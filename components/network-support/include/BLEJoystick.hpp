#pragma once
#include <functional>
#include <BLEManager.hpp>
#include <memory>

namespace efhilton::ble
{
    /**
     * @class BLEJoystick
     * @brief Represents a BLE joystick interface for managing communication and events.
     *
     * The BLEJoystick class provides an abstraction for handling joystick interactions over
     * Bluetooth Low Energy (BLE). It supports callbacks for joystick events, functions, and triggers,
     * as well as connection and disconnection events. The class integrates with a BLEManager
     * to manage BLE-related functionality.
     */
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
         * - function: A character representing the function type or identifier (this is always 'F').
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
         * - trigger: A character representing the type or identifier of the trigger event. This is always 'T'.
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
         * @brief Sets the callback function to handle trigger actions.
         *
         * This method allows the user to define a custom callback function that is invoked
         * whenever a specific trigger is activated. If no callback is provided (nullptr),
         * a default action will be executed, which logs the trigger information.
         *
         * @param callback A std::function that takes a const reference to a Trigger object as input.
         *                 The callback is invoked when a trigger event occurs.
         */
        void setOnTriggersCallback(const std::function<void(const Trigger&)>& callback);

        /**
         * @brief Sets the callback function for handling BLE function events.
         *
         * This method allows the user to register a callback function that will be
         * triggered when a Function event occurs in the BLEJoystick system. If a null
         * callback is provided, a default logging functionality will be applied.
         *
         * @param callback A std::function callback that takes a constant reference
         *                 to a Function object. This callback will handle the event
         *                 related to the specified function.
         */
        void setOnFunctionsCallback(const std::function<void(const Function&)>& callback);

        /**
         * @brief Sets the callback function to handle joystick input events.
         *
         * This method allows you to specify a custom callback function that will
         * be invoked whenever joystick input events are received. The callback
         * receives a reference to a Joystick object containing the details of the
         * joystick event.
         *
         * @param callback A function object that takes a const reference to a Joystick
         *                 instance and performs actions based on the joystick event.
         *                 If nullptr is passed, a default logging callback will be used.
         */
        void setOnJoysticksCallback(const std::function<void(const Joystick&)>& callback);

        /**
         * @brief Sets the callback function to be executed when the joystick is connected.
         *
         * This method allows the user to provide a custom function that will be invoked
         * upon a successful BLE connection. If no callback is provided, a default callback
         * will print a "Connected" message using ESP logging.
         *
         * @param callback A `std::function<void()>` representing the custom function to execute
         *                 when the BLE joystick is connected. If nullptr is provided, a default
         *                 implementation is used.
         */
        void setOnConnectedCallback(const std::function<void()>& callback);

        /**
         * @brief Sets the callback function to be called when the BLE device is disconnected.
         *
         * This method allows the user to specify a custom function that will be invoked
         * when the BLE connection is terminated. If no valid callback is provided, a default
         * action will be used.
         *
         * @param callback A standard function object (std::function) with no parameters that defines
         *                 the actions to be executed upon disconnection.
         */
        void setOnDisconnectedCallback(const std::function<void()>& callback);

    private:
        static constexpr auto TAG{"BLEJoystick"};

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
        std::function<void()> onConnectedCallback;
        std::function<void()> onDisconnectedCallback;

        void onRawFunctionToggled(const uint8_t* incoming_data, const int len) const;
        void onRawTriggerTriggered(const uint8_t* incoming_data, const int len) const;
        void onRawJoystickMotion(const uint8_t* incoming_data, const int len) const;
        static double normalizeJoystickInput(const int16_t value);

        BLEManager::ConnectionStatusCallback_t onConnectionChangeCallback;
        BLEManager::DataCallback_t onDataCallback;
    };
}
