#include "esp_bt.h"
#include <esp_mac.h>
#include <esp_log.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include "BluetoothManager.hpp"
#include "NetworkUtilities.hpp"
#include "sdkconfig.h"

namespace SafeEvac {
    static constexpr auto TAG{"SAFEEVAC_BLUETOOTH_MANAGER"};
    static constexpr auto signName{"SafeEvac Sign"};

    void BluetoothManager::onTerminate() {
        initialized = false;
    }

    static std::string initializeMacAddress() {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_BT);
        return NetworkUtilities::macToString(mac);
    }

    BluetoothManager::BluetoothManager() : macAddress(initializeMacAddress()) {}

    void BluetoothManager::onInitialize() {
        ESP_LOGI(TAG, "Initializing BLE...");

        // Release classic Bluetooth memory (if not needed)
        esp_err_t ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to release classic Bluetooth memory: %s", esp_err_to_name(ret));
            return;
        }

        // Initialize the Bluetooth controller for BLE
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        ret                               = esp_bt_controller_init(&bt_cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize Bluetooth controller: %s", esp_err_to_name(ret));
            return;
        }

        // Enable BLE mode
        ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enable BLE mode: %s", esp_err_to_name(ret));
            return;
        }

        // Initialize the BLE stack
        esp_bluedroid_config_t cfg{};
        cfg.ssp_en = false;
        ret        = esp_bluedroid_init_with_cfg(&cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize Bluedroid: %s", esp_err_to_name(ret));
            return;
        }

        // Enable the BLE stack
        ret = esp_bluedroid_enable();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enable Bluedroid: %s", esp_err_to_name(ret));
            return;
        }

        // Set the local Bluetooth device name for BLE
        initialized = true;
        setDeviceName(signName);

        // Configure our advertising data. We want our name to be shown, so that it will be easier to find our signs later.
        esp_ble_adv_data_t adv_data  = {};
        adv_data.flag                = 0;
        adv_data.include_name        = true; // Include device name in advertising data
        adv_data.manufacturer_len    = 0;
        adv_data.p_manufacturer_data = nullptr;
        adv_data.p_service_data      = nullptr;
        adv_data.p_service_uuid      = nullptr;
        adv_data.service_data_len    = 0;
        adv_data.service_uuid_len    = 0;
        adv_data.set_scan_rsp        = false;

        ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set advertising data: %s", esp_err_to_name(ret));
            return;
        }

        // Set BLE parameters for advertising
        esp_ble_adv_params_t adv_params = {};
        adv_params.adv_filter_policy    = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
        adv_params.adv_int_max          = 0x40; // Maximum advertising interval, units of 0.625 ms (e.g., 40ms)
        adv_params.adv_int_min          = 0x20; // Minimum advertising interval, units of 0.625 ms (e.g., 20ms)
        adv_params.adv_type             = ADV_TYPE_NONCONN_IND;
        adv_params.channel_map          = ADV_CHNL_ALL;
        adv_params.own_addr_type        = BLE_ADDR_TYPE_PUBLIC;

        // Lower all power levels for transmit, advertise, etc. to the lowest value.
        constexpr esp_power_level_t level = ESP_PWR_LVL_N12;  // set to -12dBm (Decibel-milliwatts)
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, level);
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, level);
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, level);

        // Start BLE advertising
        ret = esp_ble_gap_start_advertising(&adv_params);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start BLE advertising: %s", esp_err_to_name(ret));
            return;
        }

        ESP_LOGI(TAG, "BLE initialized and advertising as %s...", macAddress.c_str());
    }

    const std::string& BluetoothManager::getMacAddress() const {
        return macAddress;
    }

    const std::string& BluetoothManager::getDeviceName() const {
        return deviceName;
    }

    void BluetoothManager::setDeviceName(const std::string& uncontrolledName) {
        constexpr uint8_t MAX_LEN{30}; // Bluetooth spec limits the advertisement package to 6-37 bytes TOTAL.
        const std::string cleanedName = uncontrolledName.size() > MAX_LEN ? uncontrolledName.substr(0, MAX_LEN) : uncontrolledName;

        if (initialized) {
            const esp_err_t ret = esp_ble_gap_set_device_name(cleanedName.c_str());
            if (ret == ESP_OK) {
                deviceName = cleanedName;
                return;
            }
            ESP_LOGE(TAG, "Failed to set device name: %s", esp_err_to_name(ret));
        }

        deviceName = cleanedName;
    }
}
