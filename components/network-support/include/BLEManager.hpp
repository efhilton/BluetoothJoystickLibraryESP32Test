#pragma once

#include <string>

namespace SafeEvac {
    class BLEManager {
    public:
        BLEManager() = default;

        void onInitialize();

        void onTerminate();

        [[nodiscard]] const std::string& getMacAddress();

        void setMacAddress(const std::string& newMacAddress);

        static void printStats();
        static void onSync();

    private:
        std::string macAddress;
    };
}
