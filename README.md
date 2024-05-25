# PrinterServiceController

This program is written to run on a Raspberry Pi Zero inside of the housing of a 3D printer. The housing features thermometers on the inside and outside plus a controllable fan to control the temperature during the printing. The display shows the current temperature as well as the currently selected print profile, which defines a target temperature. The control panel additionally provides two buttons and a buzzer, which allow the selection of the print profile and toggling of the fan control.

To further monitor and control the print process, there is the [flutter companion app](https://github.com/AndreasUrlberger/Printer4WebFlutterApp). It connecteds to the housing and allows the user to configure new print profiles, graphically monitor the temperature of the housing and the printer, get an overview of the print progress, watch a live webcam, and preheat parts of the printer using the Prusa API. When a user connects to the housing, the webcam stream starts automatically and via the control of a Hue Smart Plug the integrated light turns on as well, giving the user a great view of the current print.

Without the great [raspberry-pi-cross-compilers](https://github.com/abhiTronix/raspberry-pi-cross-compilers), this project would have been much more annoying and time consuming.

# Libraries and Licenses

The project makes use of the following libraries:
- [Protocol Buffers](https://github.com/protocolbuffers/protobuf)
- [hueplusplus](https://github.com/enwi/hueplusplus)
- [uSockets](https://github.com/uNetworking/uSockets/tree/master)
- [libssd1306](https://github.com/stealthylabs/libssd1306)
- [pigpio](https://github.com/joan2937/pigpio)

The according licenses can be found in the linked repositories.
