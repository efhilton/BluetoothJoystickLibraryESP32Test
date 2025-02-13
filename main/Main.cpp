#include <BLEManager.hpp>
#include <esp_log.h>
#include <nvs_flash.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern "C" void app_main(void) {
    const auto TAG = "BLE_TEST";
    ESP_LOGI(TAG, "Initializing BLE test app");

    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Instantiating BLE Manager");
    SafeEvac::BLEManager bleManager;

    ESP_LOGI(TAG, "Initializing BLE Manager");
    bleManager.onInitialize();

    ESP_LOGI(TAG, "Waiting for events");
    vTaskDelay(portMAX_DELAY);
}
