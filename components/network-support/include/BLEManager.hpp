#pragma once

#include <functional>
#include <cstring>
#include <string>
#include <host/ble_gap.h>

namespace efhilton
{
    class BLEManager
    {
    public:
        BLEManager() = default;

        void onInitialize();

        static void onTerminate();

        [[nodiscard]] const std::string& getMacAddress();
        void setOnDataCallback(const std::function<void(uint8_t*, int)>& callback);

    private:
        static constexpr int MIN_REQUIRED_HEAP = 1024 * 10;
        static constexpr auto TAG{"BLEManager"};
        std::string macAddress;

        typedef std::function<void(uint8_t*, int)> DataCallback_t;
        DataCallback_t onData = defaultDataOutput;

        void setMacAddress(const std::string& newMacAddress);

        static int onGapEvent(ble_gap_event* event, void* arg);
        static void bleAdvertise();
        static void hostTask(void* arg);
        static void onReset(int reason);
        static void printStats();
        static void onSync();
        static void onGattsRegister(ble_gatt_register_ctxt* ctxt, void* arg);
        static int onCharacteristicAccess(uint16_t conn_handle, uint16_t attr_handle, ble_gatt_access_ctxt* ctxt,
                                          void* arg);
        static void defaultDataOutput(uint8_t* incomingData, int len);
        int gatt_svr_init();
        static std::array<uint8_t, 16> reverseUuid(const std::array<uint8_t, 16>& uuid);
        static std::string bleUuid128ToGuid(const std::array<uint8_t, 16>& uuid);
    };
}
