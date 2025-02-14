#include "BLEManager.hpp"
#include "NetworkUtilities.hpp"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <array>
#include <cstring>
#include <esp_mac.h>
#include <functional>

#define DEVICE_NAME "Heltec V3 Tester"

// UUID byte arrays.
static constexpr std::array<uint8_t, 16> gatt_svr_svc_uuid_bytes{
    0xf0, 0x56, 0x78, 0x12, 0x34, 0x56, 0x78, 0x12,
    0x34, 0x56, 0x78, 0x12, 0x34, 0x56, 0x12, 0xf0
};

static constexpr std::array<uint8_t, 16> gatt_svr_chr_uuid_bytes{
    0x12, 0x90, 0x78, 0x56, 0xef, 0xcd, 0x12, 0xab,
    0x90, 0x78, 0x56, 0x34, 0x12, 0xef, 0xcd, 0xab
};

static uint8_t ownAddressType;

namespace efhilton
{
    int BLEManager::onGapEvent(ble_gap_event* event, void* arg)
    {
        switch (event->type)
        {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "connection %s; status=%d ",
                     event->connect.status == 0 ? "established" : "failed",
                     event->connect.status);
            if (event->connect.status != 0)
            {
                BLEManager::onSync();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "disconnect; reason=%d ", event->disconnect.reason);
            BLEManager::onSync();
            return 0;

        case BLE_GAP_EVENT_CONN_UPDATE:
            ESP_LOGI(TAG, "connection updated; status=%d ",
                     event->conn_update.status);
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "advertise complete; reason=%d",
                     event->adv_complete.reason);
            return 0;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG, "subscribe event; val_handle=%d conn_handle=%d reason=%d cur_notify=%d",
                     event->subscribe.attr_handle,
                     event->subscribe.conn_handle,
                     event->subscribe.reason,
                     event->subscribe.cur_notify);
            return 0;

        case BLE_GAP_EVENT_MTU:
            ESP_LOGI(TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
                     event->mtu.conn_handle,
                     event->mtu.channel_id,
                     event->mtu.value);
            return 0;
        case BLE_GAP_EVENT_CONN_UPDATE_REQ:
            ESP_LOGI(TAG, "connection update request");
            break;
        case BLE_GAP_EVENT_LINK_ESTAB:
            ESP_LOGI(TAG, "Event Link Established");
            break;
        case BLE_GAP_EVENT_DATA_LEN_CHG:
            ESP_LOGI(TAG, "Event Data Length Change");
            break;
        default:
            ESP_LOGI(TAG, "Default case: I don't know what to do here! (%d)", event->type);
            return 1;
        }
        return 0;
    }

    void BLEManager::bleAdvertise()
    {
        ble_gap_adv_params adv_params{};
        ble_hs_adv_fields fields{};
        const char* name{nullptr};
        int rc{};

        memset(&fields, 0, sizeof(fields));

        fields.flags = BLE_HS_ADV_F_DISC_GEN |
            BLE_HS_ADV_F_BREDR_UNSUP;

        fields.tx_pwr_lvl_is_present = 1;
        fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

        name = ble_svc_gap_device_name();
        fields.name = (uint8_t*)name;
        fields.name_len = strlen(name);
        fields.name_is_complete = 1;

        rc = ble_gap_adv_set_fields(&fields);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "error setting advertisement data; rc=%d", rc);
            return;
        }

        memset(&adv_params, 0, sizeof(adv_params));
        adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
        adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
        rc = ble_gap_adv_start(ownAddressType, nullptr, BLE_HS_FOREVER, &adv_params, onGapEvent, nullptr);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "error enabling advertisement; rc=%d", rc);
            return;
        }
    }

    void BLEManager::hostTask(void* arg)
    {
        ESP_LOGI(TAG, "BLE Host Task Started");

        nimble_port_run();

        nimble_port_freertos_deinit();
    }

    void BLEManager::onReset(int reason)
    {
        ESP_LOGI(TAG, "Resetting BLE State. Reason: %d", reason);
    }

    std::string BLEManager::bleUuid128ToGuid(const std::array<uint8_t, 16>& uuid)
    {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0');

        // Section 1: 8 characters
        for (int i = 0; i < 4; ++i)
        {
            ss << std::setw(2) << static_cast<int>(uuid[i]);
        }
        ss << "-";

        // Section 2: 4 characters
        for (int i = 4; i < 6; ++i)
        {
            ss << std::setw(2) << static_cast<int>(uuid[i]);
        }
        ss << "-";

        // Section 3: 4 characters
        for (int i = 6; i < 8; ++i)
        {
            ss << std::setw(2) << static_cast<int>(uuid[i]);
        }
        ss << "-";

        // Section 4: 4 characters
        for (int i = 8; i < 10; ++i)
        {
            ss << std::setw(2) << static_cast<int>(uuid[i]);
        }
        ss << "-";

        // Section 5: 12 characters
        for (int i = 10; i < 16; ++i)
        {
            ss << std::setw(2) << static_cast<int>(uuid[i]);
        }

        return ss.str();
    }

    std::array<uint8_t, 16> BLEManager::reverseUuid(const std::array<uint8_t, 16>& uuid)
    {
        auto reversedUuid = uuid;
        std::ranges::reverse(reversedUuid);
        return reversedUuid;
    }

    void BLEManager::printStats()
    {
        std::array<uint8_t, 6> addr_val{};
        ble_hs_id_copy_addr(ownAddressType, addr_val.data(), nullptr);
        const std::string serviceUUID = bleUuid128ToGuid(reverseUuid(gatt_svr_svc_uuid_bytes));
        const std::string characteristicUUID = bleUuid128ToGuid(reverseUuid(gatt_svr_chr_uuid_bytes));

        ESP_LOGI(TAG, "Device Address: %s", NetworkUtilities::macToString(addr_val.data()).c_str());
        ESP_LOGI(TAG, "Device Address Type: %d", ownAddressType);
        ESP_LOGI(TAG, "Device Name: %s", ble_svc_gap_device_name());
        ESP_LOGI(TAG, "Service UUID: %s", serviceUUID.c_str());
        ESP_LOGI(TAG, "Characteristic UUID: %s", characteristicUUID.c_str());
    }


    void BLEManager::onSync()
    {
        int rc = ble_hs_util_ensure_addr(0);
        assert(rc == 0);

        rc = ble_hs_id_infer_auto(0, &ownAddressType);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "error determining address type; rc=%d", rc);
            return;
        }

        printStats();
        bleAdvertise();
    }

    void BLEManager::onGattsRegister(ble_gatt_register_ctxt* ctxt, void* arg)
    {
        char buf[BLE_UUID_STR_LEN];

        switch (ctxt->op)
        {
        case BLE_GATT_REGISTER_OP_SVC:
            ESP_LOGI(TAG, "registered service %s with handle=%d",
                     ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                     ctxt->svc.handle);
            break;

        case BLE_GATT_REGISTER_OP_CHR:
            ESP_LOGI(TAG, "registering characteristic %s with "
                     "def_handle=%d val_handle=%d",
                     ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                     ctxt->chr.def_handle,
                     ctxt->chr.val_handle);
            break;

        case BLE_GATT_REGISTER_OP_DSC:
            ESP_LOGI(TAG, "registering descriptor %s with handle=%d",
                     ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                     ctxt->dsc.handle);
            break;

        default:
            assert(0);
            break;
        }
    }

    int BLEManager::onCharacteristicAccess(uint16_t conn_handle, uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt* ctxt, void* arg)
    {
        static constexpr int PACKET_LENGTH{20};
        static uint8_t incomingCmd[PACKET_LENGTH];

        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
        {
            // Determine the length of the incoming data
            const int len = OS_MBUF_PKTLEN(ctxt->om);

            // Ensure the buffer is large enough to hold the incoming data plus a null terminator
            if (len + 1 >= PACKET_LENGTH)
            {
                ESP_LOGE(BLEManager::TAG, "Incoming data too large for buffer");
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            // Extract the data from ctxt->om
            const int rc = ble_hs_mbuf_to_flat(ctxt->om, incomingCmd, len, nullptr);
            if (rc != 0)
            {
                ESP_LOGE(BLEManager::TAG, "Error extracting data from mbuf: %d", rc);
            }
            else
            {
                auto* bleManager = static_cast<BLEManager*>(arg);
                if (bleManager->onData != nullptr)
                {
                    bleManager->onData(incomingCmd, len);
                }
            }

            return rc;
        }
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
        {
            ESP_LOGI(TAG, "Read Characteristic Value Request");
            return os_mbuf_append(ctxt->om, incomingCmd, sizeof(incomingCmd));
        }
        ESP_LOGI(TAG, "Unknown Characteristic Access Request");
        return 0;
    }

    void BLEManager::defaultDataOutput(uint8_t* incomingData, int len)
    {
        const std::string incomingCmdString(reinterpret_cast<char*>(incomingData), len);
        ESP_LOGI(BLEManager::TAG, "Received command via Characteristic, value: %s", incomingCmdString.c_str());
    }

    int BLEManager::gatt_svr_init()
    {
        ble_svc_gap_init();
        ble_svc_gatt_init();

        static ble_uuid128_t gatt_svr_svc_uuid{};
        gatt_svr_svc_uuid.u = {
            .type = BLE_UUID_TYPE_128
        };
        memcpy(gatt_svr_svc_uuid.value, gatt_svr_svc_uuid_bytes.data(), sizeof(gatt_svr_svc_uuid.value));

        static ble_uuid128_t gatt_svr_chr_uuid{};
        gatt_svr_chr_uuid.u = {
            .type = BLE_UUID_TYPE_128
        };
        memcpy(gatt_svr_chr_uuid.value, gatt_svr_chr_uuid_bytes.data(), sizeof(gatt_svr_chr_uuid.value));

        ble_gatt_chr_def mainCharacteristic{};
        mainCharacteristic.uuid = &gatt_svr_chr_uuid.u;
        mainCharacteristic.access_cb = onCharacteristicAccess;
        mainCharacteristic.arg = this;
        mainCharacteristic.flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE;
        static ble_gatt_chr_def characteristics[] = {
            mainCharacteristic,
            {} // no more characteristics.
        };

        ble_gatt_svc_def mainService{};
        mainService.type = BLE_GATT_SVC_TYPE_PRIMARY;
        mainService.uuid = &gatt_svr_svc_uuid.u;
        mainService.characteristics = characteristics;
        static const ble_gatt_svc_def gatt_svr_svcs[] = {
            mainService,
            {}, // no more services.
        };

        int rc = ble_gatts_count_cfg(gatt_svr_svcs);
        if (rc != 0)
        {
            return rc;
        }

        rc = ble_gatts_add_svcs(gatt_svr_svcs);
        if (rc != 0)
        {
            return rc;
        }

        ESP_LOGI(TAG, "Service UUID: %s", bleUuid128ToGuid(gatt_svr_svc_uuid_bytes).c_str());
        ESP_LOGI(TAG, "Characteristic UUID: %s", bleUuid128ToGuid(gatt_svr_chr_uuid_bytes).c_str());
        return 0;
    }


    void BLEManager::onInitialize()
    {
        size_t free_heap = esp_get_free_heap_size();
        ESP_LOGI(TAG, "Available heap memory before BLE initialization: %d bytes", free_heap);

        if (free_heap < 50000)
        {
            ESP_LOGE(TAG, "Insufficient heap memory for BLE initialization: %d bytes available", free_heap);
            return;
        }

        ESP_LOGI(TAG, "Initializing NimBLE stack");
        esp_err_t ret = nimble_port_init();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "NimBLE port initialization failed: %s", esp_err_to_name(ret));
            return;
        }

        ble_hs_cfg.reset_cb = onReset;
        ble_hs_cfg.sync_cb = onSync;
        ble_hs_cfg.gatts_register_cb = onGattsRegister;
        ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
        ble_hs_cfg.sm_io_cap = 3;

        ret = gatt_svr_init();
        if (ret != 0)
        {
            ESP_LOGE(TAG, "GATT server initialization failed: %d", ret);
            return;
        }

        ret = ble_svc_gap_device_name_set(DEVICE_NAME);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Setting BLE device name failed: %s", esp_err_to_name(ret));
            return;
        }
        ESP_LOGI(TAG, "Device name set successfully.");

        nimble_port_freertos_init(hostTask);
        ESP_LOGI(TAG, "Bluetooth initialization successfully completed.");
    }

    void BLEManager::onTerminate()
    {
        esp_err_t stop_ret = nimble_port_stop();
        if (stop_ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Stopping NimBLE port failed: %s", esp_err_to_name(stop_ret));
            return;
        }

        ESP_LOGI(TAG, "NimBLE port stopped successfully.");
        esp_err_t deinit_ret = nimble_port_deinit();
        if (deinit_ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Deinitializing NimBLE port failed: %s", esp_err_to_name(deinit_ret));
            return;
        }

        ESP_LOGI(TAG, "NimBLE port deinitialized successfully.");

        esp_err_t hci_deinit_ret = esp_nimble_hci_deinit();
        if (hci_deinit_ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Deinitializing NimBLE HCI failed: %s", esp_err_to_name(hci_deinit_ret));
            return;
        }

        ESP_LOGI(TAG, "NimBLE HCI deinitialized successfully.");
        ESP_LOGI(TAG, "Bluetooth termination completed successfully.");
    }

    void BLEManager::setMacAddress(const std::string& newMacAddress)
    {
        macAddress = newMacAddress;
    }

    const std::string& BLEManager::getMacAddress()
    {
        if (macAddress.empty())
        {
            uint8_t mac[6];
            esp_read_mac(mac, ESP_MAC_BT);
            macAddress = NetworkUtilities::macToString(mac);
        }
        return macAddress;
    }

    void BLEManager::setOnDataCallback(const DataCallback_t& callback)
    {
        onData = callback;
    }
}
