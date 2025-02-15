#include <BLEManager.hpp>
#include <BleJoystick.hpp>
#include <cstdint>
#include <functional>
#include <memory>


namespace efhilton::ble
{
    BLEJoystick::BLEJoystick() : bleManager(std::make_unique<BLEManager>()), onTriggersCallback(nullptr),
                                 onFunctionsCallback(nullptr), onJoysticksCallback(nullptr)
    {
        bleManager->setOnDataCallback([this](const uint8_t* incomingData, const int len)
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
        });
        bleManager->setConnectionStatusCallback([](const bool isConnected)
        {
            if (isConnected)
            {
                ESP_LOGI("MAIN", "Connected");
            }
            else
            {
                ESP_LOGI("MAIN", "Disconnected");
            }
        });
        bleManager->onInitialize();
    }

    void BLEJoystick::setOnTriggersCallback(const std::function<void(const Trigger&)>& callback)
    {
        onTriggersCallback = callback;
    }

    void BLEJoystick::setOnFunctionsCallback(const std::function<void(const Function&)>& callback)
    {
        onFunctionsCallback = callback;
    }

    void BLEJoystick::setOnJoysticksCallback(const std::function<void(const Joystick&)>& callback)
    {
        onJoysticksCallback = callback;
    }

    void BLEJoystick::onFunctionToggled(const uint8_t* incoming_data, const int len) const
    {
        if (incoming_data[0] != 'F' && len != sizeof(Function))
        {
            return;
        }
        auto f = reinterpret_cast<const Function&>(*incoming_data);
        if (onFunctionsCallback != nullptr)
        {
            onFunctionsCallback(f);
        }
    }

    void BLEJoystick::onTrigger(const uint8_t* incoming_data, const int len) const
    {
        if (incoming_data[0] != 'T' || len != sizeof(Trigger))
        {
            return;
        }
        auto t = reinterpret_cast<const Trigger&>(*incoming_data);
        if (onTriggersCallback != nullptr)
        {
            onTriggersCallback(t);
        }
    }

    double BLEJoystick::normalized(int16_t value)
    {
        return static_cast<double>(value) / 32768.0;
    }

    void BLEJoystick::onJoystick(const uint8_t* incoming_data, const int len)
    {
        if ((incoming_data[0] != 'L' && incoming_data[0] != 'R'))
        {
            ESP_LOGI("MAIN", "Unknown joystick: %d", incoming_data[0]);
            return;
        }
        if (len != sizeof(JoystickRaw))
        {
            ESP_LOGI("MAIN", "Joystick data length incorrect: %d, expected %d", len, sizeof(JoystickRaw));
            return;
        }
        JoystickRaw j = *reinterpret_cast<const JoystickRaw*>(incoming_data);
        Joystick joystick{
            .joystick = j.joystick,
            .x = normalized(j.x),
            .y = normalized(j.y)
        };
        if (onJoysticksCallback != nullptr)
        {
            onJoysticksCallback(joystick);
        }
    }
};
