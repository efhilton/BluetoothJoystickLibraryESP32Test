#include <BLEJoystick.hpp>
#include <esp_log.h>
#include <memory>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <driver/ledc.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/gpio_num.h>

gpio_num_t LED_GPIO_NUM{GPIO_NUM_35};

gpio_num_t PWM_GPIO_NUM{GPIO_NUM_0};

void enableOutputPin(gpio_num_t pin, bool state)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin); // Select GPIO 36
    io_conf.mode = GPIO_MODE_OUTPUT; // Set as output mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE; // Disable interrupts
    gpio_config(&io_conf); // Configure GPIO
    gpio_set_level(pin, state); // Set GPIO36 low
}

void enablePwm(const bool state)
{
    if (state)
    {
        // Assuming LEDC is used for PWM on the ESP32
        ledc_timer_config_t ledc_timer = {};
        ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE; // Use low-speed mode
        ledc_timer.timer_num = LEDC_TIMER_0; // Select timer 0
        ledc_timer.duty_resolution = LEDC_TIMER_10_BIT; // Resolution: 10 bits (1024 levels)
        ledc_timer.freq_hz = 50; // Frequency: 1 kHz
        ledc_timer.clk_cfg = LEDC_AUTO_CLK; // Use automatic clock source
        ledc_timer_config(&ledc_timer);

        ledc_channel_config_t ledc_channel = {};
        ledc_channel.gpio_num = PWM_GPIO_NUM;
        ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
        ledc_channel.channel = LEDC_CHANNEL_0;
        ledc_channel.timer_sel = LEDC_TIMER_0;
        ledc_channel.duty = 512; // Set initial duty cycle to 50% (1024/2=512 for 10-bit resolution)
        ledc_channel.hpoint = 0;
        ledc_channel_config(&ledc_channel);
    }
    else
    {
        // Disable PWM by stopping the channel and timer
        ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);

        // Unconfigure the channel and timer (restore GPIO to input state)
        gpio_reset_pin(PWM_GPIO_NUM); // Resets the GPIO pin used by the PWM
    }
}

void setDutyCycleToPct(int dutyCyclePct)
{
    const int resolutionBits = 10; // Example: 10-bit resolution
    const int maxDuty = (1 << resolutionBits) - 1; // Max duty cycle = 1023 for 10 bits
    int dutyCycle = (dutyCyclePct * maxDuty) / 100; // Convert percent to raw duty
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, dutyCycle);

    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

extern "C" void app_main(void)
{
    bool sendConsoleMessages = false;
    enableOutputPin(LED_GPIO_NUM, false);
    enableOutputPin(PWM_GPIO_NUM, false);

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
    joystick.setOnConnectedCallback([] { ESP_LOGI("MAIN", "BLEJOYSTICK CONNECTED"); });
    joystick.setOnDisconnectedCallback([] { ESP_LOGI("MAIN", "BLEJOYSTICK DISCONNECTED"); });
    joystick.setOnTriggersCallback([](const efhilton::ble::BLEJoystick::Trigger& trigger)
    {
        ESP_LOGI("MAIN", "Trigger '%c%d' triggered", trigger.trigger, trigger.id);
    });
    joystick.setOnFunctionsCallback([&sendConsoleMessages](const efhilton::ble::BLEJoystick::Function& function)
    {
        ESP_LOGI("MAIN", "Function '%c%d' toggled to %d", function.function, function.id, function.state);
        if (function.id == 0)
        {
            // toggle an LED
            gpio_set_level(LED_GPIO_NUM, function.state);
        }
        else if (function.id == 1)
        {
            // transmit console messages back to the remote android device.
            sendConsoleMessages = function.state;
        }
        else if (function.id == 2)
        {
            enablePwm(function.state);
        }
        else
        {
            ESP_LOGI("MAIN", "Function '%c%d' not implemented", function.function, function.id);
        }
    });
    joystick.setOnJoysticksCallback([](const efhilton::ble::BLEJoystick::Joystick& j)
    {
        ESP_LOGI("MAIN", "Joystick '%c' moved to (%.2f, %.2f)", j.joystick, j.x, j.y);
        if (j.joystick == 'L')
        {
            setDutyCycleToPct(static_cast<int>((1 + j.x) * 50));
        }
    });

    ESP_LOGI(TAG, "Waiting for Connections.");

    int counter = 0;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (sendConsoleMessages)
        {
            const std::string message = "This is a console message with id# " + std::to_string(counter++);
            ESP_LOGI(TAG, "Queueing console message: '%s'", message.c_str());
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
    }
}
