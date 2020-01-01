## Mecanum Remote
Used to control vehicle equipped with mecanum/omni wheels, powered by ESP8266.

ESP8266 handles everything from receiving input for controls and outputs on four motor drivers.
Inputs controls are web based; ESP8266 runs a webpage with joysticks, so a smartphone is needed (alternatively a device with wifi and browser like PC). When touch input is received for joysticks, websocket will send data back to ESP8266. The data will be parsed and be used to control each motor driver.

## Prerequisite:
- VSCode (since we are working with PlatformIO)
- PlatformIO (abbr: PIO)

## Instruction for ESP8266:
1. Download, install and set up prerequisite software
2. Clone/Download this repository
3. VSCode -> PIO: Home -> Open Project, select repository as folder
4. PIO: Upload

## Instruction for input controls:
1. Connect to ESP8266 access point with a smartphone (name and password in src\main.cpp) 
2. go to www.omniwheel.com on a browser (tested on chrome)
3. Use left joystick to shift, right joystick to turn clockwise/counter-clockwise (alternatively open serial monitor to observe payload from websocket)

## Remarks
The logic for motor drivers definitely needs refactoring to suit your needs, it was programmed based on trial and error.

## Credits
- tttapa for providing ESP8266 tutorials as a starting point (https://tttapa.github.io/)
- SuperDroid Robots for creating this image on manipulating directions of mecanum wheels (https://www.superdroidrobots.com/images/TP/TP-950-001-C.jpg)