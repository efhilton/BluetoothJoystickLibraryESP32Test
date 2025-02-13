#pragma once

#include <string>

namespace SafeEvac {
    class BluetoothManager {
    public:
        BluetoothManager();

        ~BluetoothManager() = default;

        void onInitialize();

        void onTerminate();

        [[nodiscard]] const std::string& getMacAddress() const;

        [[nodiscard]] const std::string& getDeviceName() const;

        void setDeviceName(const std::string& deviceName);

    private:
        std::string macAddress;
        std::string deviceName{};
        bool        initialized{false};
    };
}
