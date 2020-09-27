# ESP LED Matrix Display

## About
This project is intended to provide an easily customizeable and functional firmware to drive LED Matrix panels using 
ESP8266 based microcontrollers. It's powered by the [PxMatrix](https://github.com/2dom/PxMatrix) driver and is
inspired by the concept of [MorphingClockRemix](https://github.com/lmirel/MorphingClockRemix) and intends to greatly
expand upon it, providing an easily customizable system using a JSON based configuration system and a Vue based web
interface. This is one of my first substantial C++/HTML projcts, so some aspects are still a bit rough. The name itself
is a placeholder until something clever comes along.

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
and OTA updates. This was developed primarily on a P3 32x64 panel, but other sizes should be supported. Wiring is required to
be like [this](https://www.instructables.com/id/Morphing-Digital-Clock/). A photoresistor/resistor is supported on pin 
A0 in [this](https://www.instructables.com/id/NodeMCU-With-LDR/) configuration to control the display brightness. There 
are pre-configured Platformio environments for Wemos D1 Mini, NodeMCU V2, and the Wemos D1 Mini Lite. Pre-built update
binaries are availible for D1 Mini and NodeMCU V2. The D1 Mini Lite has 1MB of flash memory, so OTA updates are not 
supported and features like HTTPS and built-in images are disabled.

## Installation
The release binaries could be manually flashed using [esptool](https://github.com/espressif/esptool), but flashing the
filesystem binary would require board specific parameters, which would be a hassle. Instead, use Platformio or Arduino
IDE after downloading the latest [release](https://github.com/TheLogicMaster/ESP-LED-Matrix-Display/releases/latest).
The flash memory split for the program and filesystem is 50/50 for a 4MB board, where 1MB is for the program, 1MB is for
firmware OTA updates, and 2MB is for the LittleFS filesystem. For platformio, this means the `eagle.flash.4m2m.ld` ld
script. For Arduino IDE, use the `4MB (FS: 2MB)` flashing option. For 1MB boards, Platformio is probably required, as it
doesn't look like Arduino IDE has an option to not use flash for OTA. 

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

The ESP8266 board support must be installed from 
[here](https://github.com/esp8266/Arduino#installing-with-boards-manager).
The [ESP8266 FS plugin](https://github.com/earlephilhower/arduino-esp8266littlefs-plugin)
is also needed to upload the web interface. Select the correct board and flash split for your board. First upload the
code to the board, then select *FS Data Upload* under *Tools*.

### Platformio
To use [Platformio](https://docs.platformio.org/), install it and configure platformio.ini to suit the display size and
ESP8266 flash layout. If you are using one of the boards with an existing environment, just use that one, especially
since that will enable easy OTA updates using release binaries rather than manually compiling and flashing new ones. 
Build and upload to the board. There seems to be an issue using .ino files with platformio which necessitates running 
the upload task twice. Then run the *Upload Filesystem Image* task.

### Initial Setup
This project uses [WifiManager](https://github.com/tzapu/WiFiManager) to handle WiFi configuration. After successfully
connecting to a network using the created access point, the display's IP will be shown. The configuration server is
hosted on port 80, so enter the address in any browser. After saving any configuration from the *Display* page, the 
display will update and show the new content. [DoubleResetDetector](https://github.com/datacute/DoubleResetDetector) 
is used to detect two restarts within ten seconds of each other, at which point the WiFi configuration AP will start
again. The configuration screen is always displayed after setting up the WiFi connection, if you want to determine
the display's IP. Static IP configuration is not available yet. If the display isn't displaying correctly after flashing
the firmware, the default PxMatrix parameters may be incorrect for your display. In this case, either determine the IP
of the display using a serial monitor with baud rate 115200 or your router. Once connected to the display dashboard,
try changing the mux and scan patterns under *Settings*. If this doesn't fix it, the wiring may be incorrect or the 
firmware wasn't correct for your board, causing incorrect pin assignments. 

## Web Dashboard
Vue is used to create a SPA webpage that is hosted on the ESP8266. All files are compressed and it takes up about
500KB of the filesystem. The first time loading the interface takes a while, but after that the files should be cached
until the dashboard is updated. All of the resource intensive processes like image conversion take place in the browser
to leave only rendering to the display. The dashboard should be fully functional on either mobile or desktop 
environments, but the mobile part is not seamless yet. The interface is broken up into several pages based on their 
functions. The sidebar handles navigation between pages, and the page names should be pretty self-explanatory.

## OTA Updates
OTA Updates are as simple as downloading the newest release binary that matches your board configuration and selecting
it from the *OTA Update* page of the dashboard. Alternatively if you have a non-stock configuration, pull the repo
changes and build and deploy the dashboard and firmware again. In the future, automatic update checking and updating 
from the dashboard will be implemented.

## Configuration
The display configuration is entirely based on a JSON configuration file stored in the display filesystem. This is
primarily intended to be configured using the graphical interface, but the *Raw Configuration Editor* page can be used
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

## Logistics
This project is intended to handle everything in a more asynchronous way to ensure consistent rendering. Previously, I
attempted to branch MorphingClockRemix with a few more features, but it was fundamentally designed differently, where
animations would steal the main thread until they were done, which isn't really compatible with my idea for a widget
based rendering system. Thus, I started from an empty project setup for PxMatrix.

### Rendering
Depending on your configuration, up to three display buffers could be used. One or two buffers will be used by PxMatrix
to handle basic rendering, but another buffer will be allocated if any enabled widgets are using transparency.
This works by rendering all background layer widgets to this buffer and then drawing a portion of this buffer underneath
any dirty widgets. If a widget isn't using transparency, then the specified background color will be filled underneath
the widget according to its size. There is a tradeoff here: a lot more memory is required, but the need to re-draw the 
entire screen is greatly reduced, only occuring when a background layer widget changes. Widgets are rendered from lowest
layer to highest layer, but background layers should not be above any non-background layers or things will render
incorrectly.

### Widget Updating
Widgets are marked as dirty whenever their content changes. This can happen if they are a time based widget and the time
changes or if they are a dynamic widget and the specified update interval elapses. 

### Display API
There is a basic, unsecured API that the dashboard uses to configure the display. This is defined in the 
*setupWebserver* function, and functions for each route can easily be read to determine any necessary parameters. Token
based or basic auth could be a future feature.

## Firmware Customization
It's recommended to use Platformio if you are planning on customizing the firmware.
### Definitions
The definitions are all documented in the source code. The following definitions are configured in the platformio.ini 
file if using Platformio. Just change the source definitions if using Arduino IDE.
* **VERSION_CODE:** This is used to update dashboard browser caches and ensure that the dashboard version always
matches the firmware version so that hopefully no broken configurations occur. 
* **DISPLAY_WIDTH:** The width of the display. Used for buffer sizes and configuration thresholds.
* **DISPLAY_HEIGHT:** The height of the display.
* **DISPLAY_PANELS:** The total number of chained horizontal panels.
* **DEBUGGING:** Enables more verbose logging of display events to the serial port.
### Optional Features
* **Brightness Sensor:** If the brightness sensor is enabled, the Vcc measurement is disabled. If the brightness rolling
average is disabled, a bit of memory will be saved and the display will immediately respond to brightness changes. This
is enabled by default.
* **Arduino OTA:** This is really only useful for LAN based development, since it doesn't want to flash the FS binary 
directly over the existing filesystem, presumably, since it complains of not having enough space.
* **Web Server Caching:** Files are cached based on dashboard version, so if you are modifying and testing the dashboard,
you may want to disable this. This is disabled by default.
* **Double Buffering:** The PxMatrix library supports using two buffers, with one being the actively rendered buffer
and the other being the one being drawn to. This prevents partial renderings of content until the entire screen is done
drawing. The downside is that twice as much memory is used for display buffers. This is enabled by default.
### Firmware Embedded Images
Addition images can easily be added to the firmware by including the source header for either images stored as a 
uint8_t, uint16_t, or char(Gimp) array. To make the display aware of the images, simply add a new entry to the *progmemImages*
map with the correct type, dimensions, and a name. 565 color and Gimp header files are supported. Both of which can be
exported from the dashboard. The 565 format uses half the space that the Gimp format uses, so it is preferable. Unwanted
included images can also be removed in this way.


## Credits
* Small font and weather animations are repurposed from 
[MorphingClockRemix](https://github.com/lmirel/MorphingClockRemix)
* BLM and Mario images are from [PxMatrix](https://github.com/2dom/PxMatrix)
* Gimp header format information is from 
[Disk91](https://www.disk91.com/2014/technology/programming/use-create-thegimp-header-file-format/)
* 565 Image format based on this [converter](http://www.rinkydinkelectronics.com/t_imageconverter565.php)
