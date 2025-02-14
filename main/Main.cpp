#include <BLEManager.hpp>
#include <esp_log.h>
#include <nvs_flash.h>
#include <driver/gpio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/gpio_num.h>

gpio_num_t LED_GPIO_NUM {GPIO_NUM_35};

struct Function
{
    char function;
    int8_t id;
    bool state;
};

struct Trigger
{
    char trigger;
    int8_t id;
};

struct Joystick
{
    char joystick;
    int16_t x;
    int16_t y;
} __attribute__((packed));

void onFunctionToggled(const uint8_t* incoming_data, const int len)
{
    if (incoming_data[0] != 'F' && len != sizeof(Function))
    {
        return;
    }
    auto f = reinterpret_cast<const Function&>(*incoming_data);
    ESP_LOGI("MAIN", "Function '%c%d' toggled to %d", f.function, f.id, f.state);
    if (f.id == 0)
    {
        gpio_set_level(LED_GPIO_NUM, f.state);
    }
}

void onTrigger(const uint8_t* incoming_data, const int len)
{
    if (incoming_data[0] != 'T' || len != sizeof(Trigger))
    {
        return;
    }
    auto t = reinterpret_cast<const Trigger&>(*incoming_data);
    ESP_LOGI("MAIN", "Trigger '%c%d' triggered", t.trigger, t.id);
}

double normalized(int16_t value)
{
    return static_cast<double>(value) / 32768.0;
}

void onJoystick(const uint8_t* incoming_data, const int len)
{
    if ((incoming_data[0] != 'L' && incoming_data[0] != 'R'))
    {
        ESP_LOGI("MAIN", "Unknown joystick: %d", incoming_data[0]);
        return;
    }
    if (len != sizeof(Joystick))
    {
        ESP_LOGI("MAIN", "Joystick data length incorrect: %d, expected %d", len, sizeof(Joystick));
        return;
    }
    Joystick j = *reinterpret_cast<const Joystick*>(incoming_data);
    ESP_LOGI("MAIN", "Joystick '%c' moved to (%d, %d) => (%.2f, %.2f)", j.joystick, j.x, j.y, normalized(j.x),
             normalized(j.y));
}

std::function<void(uint8_t*, int)> onDataCallback = [](const uint8_t* incomingData, const int len)
{
    switch (incomingData[0])
    {
    case 'F':
        onFunctionToggled(incomingData, len);
        break;
    case 'T':
        onTrigger(incomingData, len);
        break;
    case 'L':
    case 'R':
        onJoystick(incomingData, len);
        break;
    default:
        ESP_LOGI("MAIN", "Unknown command: %d", incomingData[0]);
    }
};

extern "C" void app_main(void)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << LED_GPIO_NUM); // Select GPIO 36
    io_conf.mode = GPIO_MODE_OUTPUT; // Set as output mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE; // Disable interrupts
    gpio_config(&io_conf); // Configure GPIO
    gpio_set_level(LED_GPIO_NUM, 0); // Set GPIO36 low

    const auto TAG = "BLE_TEST";
    ESP_LOGI(TAG, "Initializing BLE test app");

    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Instantiating BLE Manager");
    efhilton::BLEManager bleManager;

    bleManager.setOnDataCallback(onDataCallback);

    ESP_LOGI(TAG, "Initializing BLE Manager");
    bleManager.onInitialize();

    ESP_LOGI(TAG, "Waiting for events");
    vTaskDelay(portMAX_DELAY);
}
