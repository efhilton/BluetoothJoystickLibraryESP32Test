#pragma once
#include <functional>
#include <BLEManager.hpp>
#include <memory>

namespace efhilton::ble
{
    class BLEJoystick
    {
    public:
        /**
         * @struct Function
         * @brief Represents a Function structure used in BLE communication.
         *
         * This structure contains information about a function's identifier,
         * its character representation, and its state, which might act as a trigger
         * or toggle in the BLEJoystick system.
         *
         * Members:
         * - function: A character representing the function type or identifier (e.g., 'F').
         * - id: An 8-bit integer representing the unique ID of the function.
         * - state: A boolean indicating the current state of the function,
         *          such as ON (true) or OFF (false).
         */
        struct Function
        {
            char function;
            int8_t id;
            bool state;
        };

        /**
         * @struct Trigger
         * @brief Represents a trigger event in the BLEJoystick system.
         *
         * This structure defines the essential components of a trigger,
         * including its type and identifier, primarily used for event communication
         * and handling within the BLEJoystick functionalities.
         *
         * Members:
         * - trigger: A character representing the type or identifier of the trigger event.
         * - id: An 8-bit integer denoting the unique ID associated with the trigger.
         */
        struct Trigger
        {
            char trigger;
            int8_t id;
        };

        /**
         * @struct Joystick
         * @brief Represents a joystick control used in BLE communication.
         *
         * This structure contains data about the joystick's identifier and its position
         * as determined by the X and Y axes. It is used to track movement and interactions
         * within the BLEJoystick system.
         *
         * Members:
         * - joystick: A character representing the joystick's identifier, either L or R for Left or Right, respectively.
         * - x: A double representing the X-axis position of the joystick, [-1.0, 1.0]
         * - y: A double representing the Y-axis position of the joystick, [-1.0, 1.0]
         */
        struct Joystick
        {
            char joystick;
            double x;
            double y;
        };

        /**
         * @brief Constructs a BLEJoystick object and initializes BLE communication and callbacks.
         *
         * The constructor initializes the BLEManager instance and sets up data and connection
         * status callbacks for handling incoming BLE data and device connection changes.
         *
         * @return An instance of BLEJoystick with its necessary BLE-related components initialized.
         */
        BLEJoystick();

        /**
         * @brief Sets the callback function for handling trigger events.
         *
         * This method sets a callback function that will be invoked when
         * a trigger event occurs in the BLEJoystick system. The callback
         * allows users to handle trigger-specific logic.
         *
         * @param callback A std::function taking a const Trigger reference
         *                 as an argument. It defines the logic to execute
         *                 when a trigger event is detected.
         */
        void setOnTriggersCallback(const std::function<void(const Trigger&)>& callback);

        /**
         * @brief Sets the callback function to be invoked when a function is triggered or updated.
         *
         * This method assigns a user-defined callback function. The callback is executed
         * whenever a function-related event occurs within the BLEJoystick system, passing the
         * corresponding function data as an argument.
         *
         * @param callback A std::function that takes a single parameter of type `const Function&`.
         *                 It defines the operation to perform when the event is triggered.
         */
        void setOnFunctionsCallback(const std::function<void(const Function&)>& callback);

        /**
         * @brief Sets a callback function for joystick events.
         *
         * This method allows users to define a callback function that will be invoked
         * whenever a joystick event occurs. The provided callback receives a reference
         * to a `Joystick` object containing information about the event.
         *
         * @param callback A `std::function` object that takes a `const Joystick&` as
         *                 an argument. This function is called during joystick events.
         */
        void setOnJoysticksCallback(const std::function<void(const Joystick&)>& callback);

    private:
        struct JoystickRaw
        {
            char joystick;
            int16_t x;
            int16_t y;
        } __attribute__((packed));

        std::unique_ptr<BLEManager> bleManager{};
        std::function<void(Trigger&)> onTriggersCallback;
        std::function<void(Function&)> onFunctionsCallback;
        std::function<void(Joystick&)> onJoysticksCallback;

        void onFunctionToggled(const uint8_t* incoming_data, const int len) const;

        void onTrigger(const uint8_t* incoming_data, const int len) const;

        static double normalized(int16_t value);

        void onJoystick(const uint8_t* incoming_data, const int len);
        BLEManager::ConnectionStatusCallback_t onConnectionChangeCallback;
        BLEManager::DataCallback_t onDataCallback;
    };
}
