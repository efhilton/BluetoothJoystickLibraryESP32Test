#include <BLEJoystick.hpp>
#include <esp_log.h>
#include <memory>
#include <nvs_flash.h>
#include <sstream>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/timer_types_legacy.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/gpio_num.h>

gpio_num_t LED_GPIO_NUM{GPIO_NUM_35};

gpio_num_t PWM_LR_GPIO_NUM{GPIO_NUM_0};
gpio_num_t PWM_UD_GPIO_NUM{GPIO_NUM_2};

void enableOutputPin(const gpio_num_t gpio, const bool state)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << gpio); // Select GPIO 36
    io_conf.mode = GPIO_MODE_OUTPUT; // Set as output mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE; // Disable interrupts
    gpio_config(&io_conf); // Configure GPIO
    gpio_set_level(gpio, state); // Set GPIO36 low
}

void enablePwm(const bool state, const gpio_num_t gpio, const ledc_timer_t timer, const ledc_channel_t channel,
               const uint32_t freqHz)
{
    if (state)
    {
        enableOutputPin(gpio, false);
        // Assuming LEDC is used for PWM on the ESP32
        ledc_timer_config_t ledc_timer = {};
        ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE; // Use low-speed mode
        ledc_timer.timer_num = timer; // Select timer 0,
        ledc_timer.duty_resolution = LEDC_TIMER_10_BIT; // Resolution: 10 bits (1024 levels)
        ledc_timer.freq_hz = freqHz; // Frequency: 1 kHz
        ledc_timer.clk_cfg = LEDC_AUTO_CLK; // Use automatic clock source
        ledc_timer_config(&ledc_timer);

        ledc_channel_config_t ledc_channel = {};
        ledc_channel.gpio_num = gpio;
        ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
        ledc_channel.channel = channel;
        ledc_channel.timer_sel = timer;
        ledc_channel.duty = 512; // Set initial duty cycle to 50% (1024/2=512 for 10-bit resolution)
        ledc_channel.hpoint = 0;
        ledc_channel_config(&ledc_channel);
    }
    else
    {
        // Stop the LEDC PWM for the given channel
        ledc_stop(LEDC_LOW_SPEED_MODE, channel, 0);

        // Uninstall any LEDC fade functionality, if applied
        ledc_fade_func_uninstall();

        // Reset the GPIO pin to default state (input)
        gpio_reset_pin(gpio);

        // Explicitly set GPIO pin as input to ensure it's cleanly reset
        gpio_set_direction(gpio, GPIO_MODE_INPUT);
    }
}

void setDutyCycleToPct(const ledc_channel_t channel, const uint32_t dutyCyclePct)
{
    constexpr uint32_t resolutionBits = 10; // Example: 10-bit resolution
    constexpr uint32_t maxDuty = (1 << resolutionBits) - 1; // Max duty cycle = 1023 for 10 bits
    const uint32_t dutyCycle = (dutyCyclePct * maxDuty) / 100; // Convert percent to raw duty
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, dutyCycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

extern "C" void app_main(void)
{
    bool sendConsoleMessages = false;
    enableOutputPin(LED_GPIO_NUM, false);

    const auto TAG = "BLE_TEST";

    ESP_LOGI(TAG, "Initializing NVS, Needed by Bluetooth");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Instantiating BLE Joystick");
    efhilton::ble::BLEJoystick joystick{"Heltec Wifi V3 Tester"};
    joystick.setOnConnectedCallback([] { ESP_LOGI("MAIN", "BLE JOYSTICK CONNECTED"); });
    joystick.setOnDisconnectedCallback([] { ESP_LOGI("MAIN", "BLE JOYSTICK DISCONNECTED"); });
    joystick.setOnTriggersCallback([](const efhilton::ble::BLEJoystick::Trigger& trigger)
    {
        ESP_LOGI("MAIN", "Trigger '%c%d' triggered", trigger.trigger, trigger.id);
    });
    joystick.setOnFunctionsCallback(
        [&sendConsoleMessages, &joystick](const efhilton::ble::BLEJoystick::Function& function)
        {
            ESP_LOGI("MAIN", "Function '%c%d' toggled to %d", function.function, function.id, function.state);
            if (function.id == 0)
            {
                std::stringstream ss{};
                ss << (function.state ? "Enabling" : "Disabling") << " LED";
                joystick.sendConsoleMessage(ss.str());
                // toggle an LED
                gpio_set_level(LED_GPIO_NUM, function.state);
            }
            else if (function.id == 1)
            {
                std::stringstream ss{};
                ss << (function.state ? "Enabling" : "Disabling") << " Periodic Console Messages (1 every 10 secs)";
                joystick.sendConsoleMessage(ss.str());
                // transmit console messages back to the remote android device.
                sendConsoleMessages = function.state;
            }
            else if (function.id == 2)
            {
                std::stringstream ss{};
                ss << (function.state ? "Enabling" : "Disabling") << " PWM Control";
                joystick.sendConsoleMessage(ss.str());
                enablePwm(function.state, PWM_LR_GPIO_NUM, LEDC_TIMER_0, LEDC_CHANNEL_0, 50);
                enablePwm(function.state, PWM_UD_GPIO_NUM, LEDC_TIMER_1, LEDC_CHANNEL_1, 50);
            }
            else
            {
                std::stringstream ss{};
                ss << "Function '"
                    << function.function << std::to_string(function.id) << "'='" << (function.state ? "true" : "false")
                    << "' has no implementation. Ignoring.";
                joystick.sendConsoleMessage(ss.str());
            }
        });
    joystick.setOnJoysticksCallback([](const efhilton::ble::BLEJoystick::Joystick& j)
    {
        ESP_LOGI("MAIN", "Joystick '%c' moved to (%.2f, %.2f)", j.joystick, j.x, j.y);
        if (j.joystick == 'L')
        {
            setDutyCycleToPct(LEDC_CHANNEL_0, static_cast<int>((j.x + 1) * 50));
            setDutyCycleToPct(LEDC_CHANNEL_1, static_cast<int>((j.y + 1) * 50));
        }
    });

    ESP_LOGI(TAG, "Waiting for Connections.");

    int counter = 0;
    while (true)
    {
        if (sendConsoleMessages)
        {
            const std::string message = "This is a console message with id# " + std::to_string(counter++);
            const size_t bytesSent = joystick.sendConsoleMessage(message, portMAX_DELAY);
            if (bytesSent == message.length())
            {
                ESP_LOGI(TAG, "...Queued console message: '%s'", message.c_str());
            }
            else
            {
                ESP_LOGE(TAG, "...Failed to queue message: '%s'", message.c_str());
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
