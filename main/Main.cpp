#include <BLEJoystick.hpp>
#include <esp_log.h>
#include <memory>
#include <nvs_flash.h>
#include <driver/gpio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/gpio_num.h>

gpio_num_t LED_GPIO_NUM{GPIO_NUM_35};

void configureLEDPinForOutput()
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << LED_GPIO_NUM); // Select GPIO 36
    io_conf.mode = GPIO_MODE_OUTPUT; // Set as output mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE; // Disable interrupts
    gpio_config(&io_conf); // Configure GPIO
    gpio_set_level(LED_GPIO_NUM, 0); // Set GPIO36 low
}

extern "C" void app_main(void)
{
    bool sendConsoleMessages = false;
    configureLEDPinForOutput();

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
    joystick.setOnConnectedCallback([] { ESP_LOGI("MAIN", "Connected"); });
    joystick.setOnDisconnectedCallback([] { ESP_LOGI("MAIN", "Disconnected"); });
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
        if (function.id == 1)
        {
            // transmit console messages back to the remote android device.
            sendConsoleMessages = function.state;
        }
    });
    joystick.setOnJoysticksCallback([](const efhilton::ble::BLEJoystick::Joystick& j)
    {
        ESP_LOGI("MAIN", "Joystick '%c' moved to (%.2f, %.2f)", j.joystick, j.x, j.y);
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
            } else {
                ESP_LOGE(TAG, "...Failed to queue message: '%s'", message.c_str());
            }
        }
    }
}
