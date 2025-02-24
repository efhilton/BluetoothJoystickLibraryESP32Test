# BLE Joystick Library Tester for ESP32 on Heltec Lora Wifi V3

# Overview

This project sets up basic BLE functionality in a Heltec Lora Wifi ESP32-S3 device I had lying around. It is primarily intended to
test a separately developed BLE Joystick library for my ESP32. Specifically, a remote device -- aka an Android application -- can enable/disable functions, trigger events, send joystick events to this embedded device.  Additionally, the remote device -- aka the Android application -- can also receive console messages from this embedded device -- all via Bluetooth Low Energy.

This code is intended to work in conjunction with:
- the [Android App](https://github.com/efhilton/BluetoothJoystick).  I wrote this to work on my Pixel 8 with the latest APIs.
- the [BLE ESP-IDF Joystick](https://github.com/efhilton/BluetoothJoystickESP32) Library itself, which should be added as a git submodule under `components/ble-joystick`

As of the time of this writing, this code presently implements the following Functions:

- [x] F1: It enables/disables the (very bright) LED that is onboard the Heltec on GPIO 35
- [x] F2: It enables/disables the ability to transmit Console messages to the remote Android app.
- [x] F3: Enable/disable Left Joystick control (Left Right, Up Down), putting out PWM output on GPIO 0 and 2, respectively.
- [ ] F4:
- [ ] F5
- [ ] F6:
- [ ] F6:
- [ ] F7:
- [ ] F8:
- [ ] F9:
- [ ] FA:
- [ ] FB:
- [ ] FC:
- [ ] FD:
- [ ] FE:
- [ ] FF:

The following triggers:
- [ ] T1
- [ ] T2
- [ ] T3
- [ ] T4
- [ ] T5
- [ ] T6
- [ ] T7
- [ ] T8
- [ ] T9
- [ ] TA
- [ ] TB
- [ ] TC
- [ ] TD
- [ ] TE
- [ ] TF

It uses the following Joystick features:
- [x] Joystick Left: horizontal axis on GPIO 0
- [x] Joystick Left: vertical axis on GPIO 2
- [ ] Joystick Right: horizontal axis
- [ ] Joystick Right: vertical axis

More features shall be added as I continue to implement more features on my electronics device.

# Questions?

Please contact [Edgar Hilton](mailto://edgar.hilton@gmail.com) if you have any queestions.
