## Mecanum Remote
Used to control vehicle equipped with mecanum/omni wheels, powered by ESP8266.

ESP8266 handles everything from receiving input for controls and outputs on four motor drivers.
Inputs controls are web based; ESP8266 runs a webpage with joysticks, so a smartphone is needed (alternatively a device with wifi and browser like PC). When touch input is received for joysticks, websocket will send data back to ESP8266. The data will be parsed and be used to control each motor driver.

Instructions for input controls:
1. Connect to ESP8266 access point (name and password in src\main.cpp)
2. go to www.omniwheel.com on a browser (tested on chrome)
3. Use left joystick to shift, right joystick to turn clockwise/counter-clockwise (alternatively open serial monitor to observe payload from websocket)

The logic for motor drivers definitely needs refactoring to suit your needs, it was programmed based on trial and error. Thanks to tttapa for the tutorials as a starting point for this project.