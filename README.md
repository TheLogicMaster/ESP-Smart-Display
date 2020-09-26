# ESP LED Matrix Display

## About
This project is intended to provide an easily customizeable and functional firmware to drive LED Matrix panels using 
ESP8266 based microcontrollers. It's powered by the [PxMatrix](https://github.com/2dom/PxMatrix) driver and is
inspired by the concept of [MorphingClockRemix](https://github.com/lmirel/MorphingClockRemix) and intends to greatly
expand upon it, providing an easily customizable system using a JSON based configuration system and a Vue based web
interface. This is one of my first substantial C++/HTML projcts, so some aspects are still a bit rough.

## Features
* Self-hosted web dashboard
* Drag and drop widget based display configuration
* Built-in image editor with support for standard images and binary formats
* Several display brightness control modes
* NTP time synchronization and timezone support
* OpenWeatherMap synchronization
* OTA Updating
* Raw configuration editor
* YouTube subscriber counter

## Demo
A demo of the web dashboard is available [Here](https://thelogicmaster.github.io/ESP-LED-Matrix-Display/). Example data 
is provided in place of the normal display API calls. Aside from configuration and image saving, the dashboard should
be fully functional.

## Hardware
Currently, only ESP8266 is supported, but ESP32 support is planned. 4MB of RAM is required to use both the web dashboard
and OTA updates. This was developed primarily on a P3 panel, but other sizes should be supported. Wiring is required to
be like [this](https://www.instructables.com/id/Morphing-Digital-Clock/). A photoresistor/resistor is supported on pin 
A0 in [this](https://www.instructables.com/id/NodeMCU-With-LDR/) configuration to control the display brightness.

## Installation
The release binaries could be manually flashed using [esptool](https://github.com/espressif/esptool), but flashing the
filesystem binary would require board specific parameters, which would be a hassle. Instead, use Platformio or Arduino
IDE after downloading the latest [release](https://github.com/TheLogicMaster/ESP-LED-Matrix-Display/releases/latest).
The flash memory split for the program and filesystem is 50/50 for a 4MB board, where 1MB is for the program, 1MB is for
firmware OTA updates, and 2MB is for the LittleFS filesystem. For platformio, this means the `eagle.flash.4m2m.ld` ld
script. For Arduino, use the `4MB (FS: 2MB)` flashing option.

### Building the Web Dashboard
The dashboard is built and moved to the data directory by running the following scripts in the project directory. This
must be done before uploading the filesystem image to the ESP8266.
```shell script
# Install NPM dependencies
./install-dependencies.sh

# Build the web dashboard
./build-dashboard.sh
```

### Arduino IDE
To use Arduino IDE, the following libraries must be installed through the library manager:
* PxMatrix
* Adafruit GFX Library
* WifiManager
* ESP_DoubleResetDetector
* ArduinoJson
* Time
* WeatherStation (And dependencies)
* Timezone
* RunningAverage (If using brightness sensor "rolling" average)

The [ESP8266 FS plugin](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#uploading-files-to-file-system)
is also needed to upload the web interface. 

### Platformio
To use [Platformio](https://docs.platformio.org/), install it and configure platformio.ini to suit the display size and
ESP8266 flash layout. Build and upload to the board. There seems to be an issue using .ino files with platformio which
necessitates running the upload task twice. Then run the Upload Filesystem Image task.

### Initial Setup
This project uses [WifiManager](https://github.com/tzapu/WiFiManager) to handle WiFi configuration. After successfully
connecting to a network using the created access point, the display's IP will be shown. The configuration server is
hosted on port 80, so enter the address in any browser. After saving any configuration from the *Display* page, the 
display will update and show the new content. [DoubleResetDetector](https://github.com/datacute/DoubleResetDetector) 
is used to detect two restarts within ten seconds of each other, at which point the WiFi configuration AP will start
again. The configuration screen is always displayed after setting up the WiFi connection, if you want to determine
the display's IP. Static IP configuration is not available yet.

## OTA Updates
OTA Updates are as simple as downloading the newest release binary that matches your board configuration and selecting
it from the *OTA Update* page of the dashboard. Alternatively if you have a non-stock configuration, pull the repo
changes and build and deploy the dashboard and firmware again. In the future, autoamtic update checking and updating 
from the dashboard will be implemented.

## Configuration
The display configuration is entirely based on a JSON configuration file stored in the display filesystem. This is
primarily intented to be configured using the graphical interface, but the *Raw Configuration Editor* page can be used
to directly edit the JSON file. If the dashboard isn't required, the configuration files could manually be uploaded
using the data directory. `configuration.json` is used to store the display configuration, and `images.json` stores
the properties of custom images.
 
## Widgets

### Text Based Widgets
All text based widgets have two font sizes: normal and large. The normal font uses the TinyFont library from
MorphingClockRemix and the large font uses the default Adafruit GFX printing font. They can be freely resized and the 
text content will be centered. Supports word and character wrapping.
* **Text:** This is a basic Text widget that shows a static string.
* **Digital Clock:** This widget shows the current time in 24 hour, 12 hour, or 12 hour with period formats.
* **JSON GET Text:** This sends a GET request and parses a JSON response to display. Supports HTTP and HTTPS.
* **GET PLAIN TEXT:** Sends GET requests and shows the body of the response. Supports HTTP and HTTPS.

### Image Widgets
There are two types of images to display: custom images provided by the user and build-in images. The built-in images
are read-only at runtime, but more can be easily added by customizing the source code. The dimensions of image widgets
are fixed to the size of the image content. 

### Weather Icon Widgets
These components show a simple animation based on the current weather state from OpenWeatherMaps. The API key and
location need to be configured on the *Settings* page.

### Analog Clock Widgets
This is a scalable widget that draws a clock based on the current time.

## Firmware Customization
It's recommended to use Platformio if you are planning on customizing the firmware.
### Flags
The flags are all documented in the source code. The following flags are configured in the platformio.ini file if using
Platformio.
* **VERSION_CODE:** This is used to update dashboard browser caches and ensure that the dashboard version always
matches the firmware version so that hopefully no broken configurations occur. 
* **DISPLAY_WIDTH:** The width of the display. Used for buffer sizes and configuration thresholds.
* **DISPLAY_HEIGHT:** The height of the display.
* **DEBUGGING:** Enables more verbose logging of display events to the serial port
### Optional Features
* **Brightness Sensor:** If the brightness sensor is enabled, the Vcc measurement is disabled. If the brightness rolling
average is disabled, a bit of memory will be saved and the display will immediately respond to brightness changes.

## Credits
* Small font and weather animations are repurposed from [MorphingClockRemix](https://github.com/lmirel/MorphingClockRemix)
* BLM and Mario images are from [PxMatrix](https://github.com/2dom/PxMatrix)
