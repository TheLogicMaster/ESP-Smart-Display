#include <Arduino.h>

#pragma region Constants

// Types
#define WIDGET_PROGMEM_IMAGE 0
#define WIDGET_FS_IMAGE 1
#define WIDGET_CLOCK 2
#define WIDGET_ANALOG_CLOCK 3
#define WIDGET_TEXT 4
#define WIDGET_TEXT_GET_JSON 5
#define WIDGET_TEXT_GET 6
#define WIDGET_WEATHER_ICON 7
#define WIDGET_SHAPE 8
#define WIDGET_VARIABLE 9
#define VARIABLE_TEMPERATURE 0
#define VARIABLE_TEMPERATURE_PERCEIVED 1
#define VARIABLE_TEMPERATURE_HIGH 2
#define VARIABLE_TEMPERATURE_LOW 3
#define VARIABLE_VISIBILITY 4
#define VARIABLE_PRESSURE 5
#define VARIABLE_HUMIDITY 6
#define VARIABLE_WIND_SPEED 7
#define FONT_TINY 0
#define FONT_TETRIS 1
#define FONT_GFX 2
#define IMAGE_UINT8 0
#define IMAGE_UINT16 1
#define IMAGE_GIMP 2
#define TEXT_NOWRAP 0
#define TEXT_WRAPPED 1
#define TEXT_WORD_WRAPPED 2
#define BRIGHTNESS_STATIC 0
#define BRIGHTNESS_AUTO 1
#define BRIGHTNESS_TIME 2
#define BRIGHTNESS_SUN 3
#define CLOCK_TYPE_24 0
#define CLOCK_TYPE_12 1
#define CLOCK_TYPE_12_PERIOD 2
#define SHAPE_RECTANGLE 0
#define SHAPE_RECTANGLE_ROUNDED 1
#define SHAPE_CIRCLE 2
#define SHAPE_PIXEL 3
#define STATE_NORMAL 0
#define STATE_UPDATING 1

// Configuration
#define ROUNDED_RECT_RADIUS 3 // The radius to use for rounded rectangles
#define MAX_CONFIG_FILE_SIZE 3000 // Max config file size in flash
#define CONFIG_JSON_BUFFER_SIZE 5000 // The buffer for configuration file parsing
#define JSON_REQUEST_BUFFER_SIZE 2000 // The JSON buffer for parsing JSON GET requests
#define MAX_IMAGE_DATA_FILE_SIZE 2000 // The maximum size for the image data file
#define IMAGE_JSON_BUFFER_SIZE 2500 // The buffer size for image data, increase if not seeing all uploaded images
#define WEATHER_UPDATE_INTERVAL 3600000 // Every Hour update the weather if using any weather widgets
#define PIXEL_YIELD_THRESHOLD 100 // Might not be necisary, yields while drawign images larger than 10 by 10
#define BRIGHTNESS_UPDATE_INTERVAL 10 // Millis between brightness increments
#define SUN_UPDATE_INTERVAL 7200000 // Every 2 hours, since sun won't rise within 2 hours of date change
#define BRIGHTNESS_SENSOR_PIN A0 // The pin to get photoresistor value from, if using
#define BRIGHTNESS_ROLLING_AVG_SIZE 10 // Higher value makes transition smoother, 4 bytes per buffer value, zero for no buffer
#define HTTPS_TRANSMIT_BUFFER 512 // Limit HTTPS buffers on ESP8266 to prevent HTTPS GET requests all failing
#define HTTPS_RECEIVE_BUFFER 512
#define ALPHA_COLOR_DISTANCE 27 // The squared distance between image alpha colors and pixel colors to count as transparent, to account for conversion errors
#define WEATHER_BUFFER_SIZE 1000 // The size of the buffer for parsing weather API responses
#define WEATHER_TIMEOUT 10000 // The timeout for weather API requests in ms

// Configuration Values
#ifndef USE_BRIGHTNESS_SENSOR
#define USE_BRIGHTNESS_SENSOR true // Disables Vcc reading functionality to use brightness sensor
#endif
#ifndef PROFILE_RENDERING
#define PROFILE_RENDERING false // Enable to log rendering times
#endif
#ifndef ENABLE_ARDUINO_OTA
#define ENABLE_ARDUINO_OTA false // LAN based OTA, but doesn't work for FS updates
#endif
#ifndef USE_DOUBLE_BUFFERING
#define USE_DOUBLE_BUFFERING true // Only disable if pressed for memory, will cause visual glitches/partial rendering
#endif
#ifndef CACHE_DASHBOARD
#define CACHE_DASHBOARD true // Disable for dashboard development, I guess
#endif
#ifndef USE_SUNRISE
#define USE_SUNRISE true // Disable to save space if Sunrise brightness mode isn't needed
#endif
#ifndef USE_NTP
#define USE_NTP true // Disable to save space if OTA updating isn't needed
#endif
#ifndef USE_HTTPS
#define USE_HTTPS true // Disable if only http GET requests are needed space
#endif
#ifndef DELETE_TRANSPARENCY_BUFFER_ON_HTTPS
#define DELETE_TRANSPARENCY_BUFFER_ON_HTTPS false // Delete transparency buffer when making HTTPS requests if they are failing due to not enough memory
#endif
#ifndef USE_PROGMEM_IMAGES
#define USE_PROGMEM_IMAGES true // Disable if firmware images aren't needed to save space
#endif
#ifndef USE_WEATHER
#define USE_WEATHER true // Disable to save space if weather widgets aren't needed
#endif
#ifndef USE_TETRIS
#define USE_TETRIS false
#endif
#ifndef VERSION_CODE
#define VERSION_CODE 0
#endif
#ifndef BOARD_NAME
#ifdef ESP32
#define BOARD_NAME "generic-esp32" // Used to identify OTA update binaries
#else
#define BOARD_NAME "generic-esp8266" // Used to identify OTA update binaries
#endif
#endif
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH 64 // The width of the display
#endif
#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT 32 // The height of the display
#endif
#ifndef DISPLAY_PANELS
#define DISPLAY_PANELS 1 // The number of total panels being horizontally chained
#endif
#ifndef DISPLAY_ROW_PATTERN
#define DISPLAY_ROW_PATTERN 16 // The row pattern for the display
#endif
#ifndef DEBUGGING
#define DEBUGGING false
#endif
#ifndef DEBUG_TRANSPARENCY
#define DEBUG_TRANSPARENCY false // Save the transparency buffer as an read-only image
#endif
#if ENABLE_ARDUINO_OTA
#include <ArduinoOTA.h>
#endif
#if DEBUGGING
#define DEBUG(fmt, ...)  Serial.printf_P((PGM_P)PSTR( "DEBUG: " fmt), ## __VA_ARGS__)
#else
#define DEBUG(...)
#endif
#if USE_DOUBLE_BUFFERING
#define double_buffer
#endif
#define PxMATRIX_MAX_HEIGHT DISPLAY_HEIGHT
#define PxMATRIX_MAX_WIDTH DISPLAY_WIDTH

#pragma endregion

// Source files
#ifdef ESP32 // ESP32 Specific
#include <Update.h>
#include <AsyncTCP.h>
#include <esp_wifi.h>
#include <FS.h>
#include <SPIFFS.h>
#define LittleFS SPIFFS
#define Dir File
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WiFiMulti.h>
//#define sint16_t signed short
//#define sint32_t signed long
#else // ESP8266 Specific
#include <ESPAsyncTCP.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif

// Common Libraries
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <PxMatrix.h>
#define ESP_DRD_USE_EEPROM true
#include <ESP_DoubleResetDetector.h>
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>
#if USE_NTP
#include <ezTime.h>
#endif
#include <map>
#if USE_WEATHER
#include "TinyIcons.h"
#endif
#if USE_SUNRISE
#include <SunMoonCalc.h>
#endif
#if USE_TETRIS
#include <TetrisMatrixDraw.h>
#endif
#include "TinyFont.h"
#include "Structures.h"

// Images
#if USE_PROGMEM_IMAGES
#include "Taz.h"
#include "Mario.h"
#include "Youtube.h"
#endif                                            

struct rgbColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class DisplayBuffer : public Adafruit_GFX {
    bool _rotate;
    std::unique_ptr <uint16_t> buffer;

public:
    DisplayBuffer() : Adafruit_GFX(DISPLAY_WIDTH, DISPLAY_HEIGHT) {
        _rotate = false;
    }

    void setRotate(bool rotate) {
        _rotate = rotate;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        if (y * _width + x >= _width * _height || !buffer) 
            return;
        buffer.get()[y * _width + x] = color;
    }

    void write(Adafruit_GFX &display, uint8_t xOff, uint8_t yOff, uint8_t width, uint8_t height) {
        for (uint8_t x = xOff; x < xOff + width; x++) {
            for (uint8_t y = yOff; y < yOff + height; y++) {
                if (y * _width + x >= _width * _height || !buffer) 
                    return;
                display.drawPixel(x, y, buffer.get()[y * _width + x]);
            }
#if defined(ESP8266)
        if (width * height > PIXEL_YIELD_THRESHOLD)
            yield();
#endif
        }
    }

    uint16_t getPixel(uint8_t x, uint8_t y) {
        if (y * _width + x >= _width * _height || !buffer) 
            return 0;
        return buffer.get()[y * _width + x];
    }

    uint16_t* getBuffer() {
        return buffer.get();
    }

    bool isAllocated() {
        return (bool)buffer;
    }

    bool allocate() {
        DEBUG("Allocating display buffer...\n");
        buffer.reset(new uint16_t[_width * _height] {0});
        if (!buffer) {
            Serial.println(F("Failed to allocate display buffer"));
            return false;
        }
        return true;
    }

    void close() {
        DEBUG("Freeing display buffer...\n");
        buffer.reset();
    }
};

std::map <std::string, ProgmemImage> progmemImages = {
#if USE_PROGMEM_IMAGES
                                                     {"taz",  {23, 28, 1,  IMAGE_UINT16, taz}},
                                                     {"mario",  {64, 32, 1, IMAGE_UINT16,  mario}},
                                                     {"youtube", {21, 16, 1, IMAGE_UINT16, youtube}}
#endif
                                                     };

// Configuration values
std::vector <Widget> widgets;
uint16_t backgroundColor;
uint8_t brightnessMode;
uint8_t brightnessBright;
uint16_t brightnessSensorDark;
uint16_t brightnessSensorBright;
uint8_t brightnessDim;
uint8_t brightnessBrightMin;
uint8_t brightnessBrightHour;
uint8_t brightnessDimMin;
uint8_t brightnessDimHour;
bool usingWeather;
bool usingTransparency;
bool metric;
bool fastUpdate;
bool vertical;
String weatherKey;
String weatherLocation;
String location;
bool usingSunMoon;
double longitude;
double lattitude;

// State values
bool disableDisplay;
uint8_t dashboardVersion;
uint8_t state;
time_t weatherUpdateTime;
uint16_t weatherID = 200;
uint16_t temperature;
uint16_t temperaturePerceived;
uint16_t temperatureMax;
uint16_t temperatureMin;
uint16_t pressure;
uint8_t humidity;
uint16_t visibility;
uint8_t windSpeed;
bool needsConfig; // If a configuration is needed
bool needsUpdate; // If a full update is needed
bool needsRestart; // If a system reboot is needed
bool needsTimezone; // If the timezone needs to be set still
uint8_t theHour12;
uint8_t theHour;
uint8_t theMinute;
time_t bootTime;
time_t brightnessTime;
time_t profileTime;
time_t sunMoonTime;
uint8_t targetBrightness;
uint8_t currentBrightness = 100;
time_t sunRiseTime;
time_t sunSetTime;
uint8_t scanPattern;
uint8_t muxPattern;
uint8_t muxDelay;

#ifdef ESP32
#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 26 // Use 26 instead of 2 since pin isn't broken out on NodeMCU 32s
TaskHandle_t displayUpdateTaskHandle = NULL;
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#else
#include <Ticker.h>
Ticker display_ticker;
#define P_LAT D0
#define P_A D1
#define P_B D2
#define P_C D8
#define P_D D6
#define P_E D3
#define P_OE D4
#endif

PxMATRIX display(DISPLAY_WIDTH, DISPLAY_HEIGHT, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

#ifdef ESP8266
void display_updater() {
    if (!disableDisplay)
        display.display(70);
}
#else
void IRAM_ATTR display_updater(){
  portENTER_CRITICAL_ISR(&timerMux);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(displayUpdateTaskHandle, &xHigherPriorityTaskWoken );
  if(xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR( );
  portEXIT_CRITICAL_ISR(&timerMux);
}
 
void displayUpdateTask(void *) {
  for(;;)
    if (ulTaskNotifyTake( pdTRUE, portMAX_DELAY) && !disableDisplay)
        display.display(70);
}

#endif

const uint16_t RED = display.color565(255, 0, 0);
const uint16_t GREEN = display.color565(0, 255, 0);
const uint16_t BLUE = display.color565(0, 0, 255);
const uint16_t WHITE = display.color565(255, 255, 255);
const uint16_t YELLOW = display.color565(255, 255, 0);
const uint16_t CYAN = display.color565(0, 255, 255);
const uint16_t MAGENTA = display.color565(255, 0, 255);

#if BRIGHTNESS_ROLLING_AVG_SIZE > 0
#include <RunningAverage.h>
RunningAverage brightnessAverage(BRIGHTNESS_ROLLING_AVG_SIZE);
#endif

DoubleResetDetector drd(10, 0);
AsyncWebServer server(80);
DNSServer dnsServer;

#if USE_NTP
Timezone timezone;
#endif

DisplayBuffer displayBuffer;

#if !USE_BRIGHTNESS_SENSOR
ADC_MODE(ADC_VCC);
#endif

#pragma region Configuration

void writeDefaultConfig() {
    LittleFS.remove("/config.json");
    File file = LittleFS.open("/config.json", "w");
    file.print(F("{\"widgets\":[{\"xOff\":17,\"yOff\":11,\"type\":4,\"transparent\":false,\"font\":2,\"content\":\"Hello\",\"colors\":[\"0x000000\"],\"width\":30,\"height\":8,\"backgroundColor\":\"0x00FFFF\"}],\"brightnessMode\":0,\"brightnessLower\":1,\"brightnessUpper\":100,\"backgroundColor\":\"0x00FFFF\",\"timezone\":\"America/Detroit\",\"metric\":false,\"weatherKey\":\"\",\"weatherLocation\":\"5014227\"}"));
    file.close();
}

uint16_t parseHexColorString(std::string s) {
    uint offset = s.substr(0, 2) == "0x" ? 2 : 0;
    if (s.length() - offset < 6) {
        Serial.print(F("Invalid HEX format: "));
        Serial.println(s.c_str());
        return 0;
    }
    return display.color565(strtoul(s.substr(offset, 2).c_str(), NULL, 16),
                            strtoul(s.substr(offset + 2, 2).c_str(), NULL, 16),
                            strtoul(s.substr(offset + 4, 2).c_str(), NULL, 16));
}

void closeFiles() {
    for (uint i = 0; i < widgets.size(); i++)
        widgets[i].file.close();
}

bool parseConfig(JsonVariant& json, String& errorString) {
    if (DEBUGGING) {
        DEBUG("Attempting to parse config: \n");
        serializeJsonPretty(json, Serial);
        Serial.println();
    }
    if (!json) {
        errorString.concat("Invalid Config");
        return false;
    }
    uint16_t tempBackgroundColor = json["backgroundColor"].isNull() ? 0 : parseHexColorString(
            json["backgroundColor"].as<char *>());
    String tempWeatherKey = json["weatherKey"].isNull() ? "" : String(json["weatherKey"].as<char *>());
    String tempWeatherLocation = json["weatherLocation"].isNull() ? "" : String(
            json["weatherLocation"].as<char *>());
    bool tempMetric = json["metric"];
    bool tempVertical = json["vertical"];
    bool tempTransparency = false;
    bool tempFastUpdate = json["fastUpdate"];
    String tempLocation = json["timezone"].isNull() ? "America/Detroit" : String(json["timezone"].as<char *>());

    uint8_t tempBrightnessMode = json["brightnessMode"];
    bool tempUsingSunMoon = tempBrightnessMode == BRIGHTNESS_SUN;
    uint8_t tempBrightnessDim = json["brightnessLower"];
    uint8_t tempBrightnessBright = json["brightnessUpper"].isNull() ? 255 : json["brightnessUpper"];
    uint16_t tempBrightnessSensorBright = json["sensorBright"];
    uint16_t tempBrightnessSensorDark = json["sensorDark"];
    uint8_t tempBrightnessBrightMin;
    uint8_t tempBrightnessBrightHour;
    uint8_t tempBrightnessDimMin;
    uint8_t tempBrightnessDimHour;
    String brightTime = json["brightTime"].isNull() ? "09:30" : String(json["brightTime"].as<char*>());
    tempBrightnessBrightHour = brightTime.substring(0, 2).toInt();
    tempBrightnessBrightMin = brightTime.substring(3, 5).toInt();
    String darkTime = json["darkTime"].isNull() ? "20:30" : String(json["darkTime"].as<char*>());
    tempBrightnessDimHour = darkTime.substring(0, 2).toInt();
    tempBrightnessDimMin = darkTime.substring(3, 5).toInt();
    double tempLongitude = json["longitude"];
    double tempLattitude = json["lattitude"];
    uint8_t tempScanPattern = json["scanPattern"];
    uint8_t tempMuxPattern = json["muxPattern"];
    uint8_t tempMuxDelay = json["muxDelay"];

    bool tempUsingWeather = false;
    std::vector <Widget> tempWidgets;
    JsonArray configWidgets = json["widgets"];
    if (configWidgets) {
        for (ushort i = 0; i < configWidgets.size(); i++) {
            JsonObject widget = configWidgets[i];
            if (!widget || widget["width"].isNull() || widget["height"].isNull()) {
                errorString.concat("Failed to parse widget: ");
                errorString.concat(i);
                return false;
            }
            if (widget["disabled"])
                continue;

            if (widget["type"] == WIDGET_WEATHER_ICON || (widget["type"] == WIDGET_VARIABLE && widget["offset"].as<uint8_t>() <= VARIABLE_WIND_SPEED))
                tempUsingWeather = true;

            if (widget["transparent"])
                tempTransparency = true;

            JsonArray a = widget["args"];
            std::vector <std::string> args;
            for (uint i = 0; i < a.size(); i++)
                args.push_back(a[i].isNull() ? "" : a[i].as<char *>());

            std::vector <uint16_t> colors;
            JsonArray c = widget[F("colors")];
            switch (widget["type"].as<uint8_t>()) {
                case WIDGET_ANALOG_CLOCK:
                    if (c.size() < 3) {
                        errorString.concat(F("Analog clocks require 2 color arguments"));
                        return false;
                    }
                    break;
                case WIDGET_CLOCK:
                    if (c.size() < 1) {
                        errorString.concat(F("Digital clocks require 1 color argument"));
                        return false;
                    }
                    break;
                case WIDGET_TEXT:
                case WIDGET_TEXT_GET_JSON:
                case WIDGET_TEXT_GET:
                    if (c.size() < 1) {
                        errorString.concat(F("Text require 1 color argument"));
                        return false;
                    }
                    break;
                default:
                    break;
            }
            for (uint i = 0; i < c.size(); i++)
                colors.push_back(parseHexColorString(std::string(c[i].as<char *>())));

            tempWidgets.push_back({widget["id"], widget["type"],
                                   widget["content"].isNull() ? "" : std::string(widget["content"].as<char *>()),
                                   widget["source"].isNull() ? "" : std::string(widget["source"].as<char *>()),
                                   widget["contentType"],
                                   widget["auth"].isNull() ? "" : std::string(widget["auth"].as<char *>()), 
                                   widget["cert"].isNull() ? "" : std::string(widget["cert"].as<char *>()), args,
                                   widget["xOff"], widget["yOff"], widget["width"], widget["height"],
                                   widget["frequency"], widget["offset"], widget["length"], colors,
                                   widget["font"],
                                   widget["bordered"], widget["borderColor"].isNull() ? (uint16_t)0 : parseHexColorString(
                                   widget["borderColor"].as<char *>()),
                                   widget["transparent"],
                                   widget["backgroundColor"].isNull() ? (uint16_t)0 : parseHexColorString(
                                   widget["backgroundColor"].as<char *>()), widget["background"]});
        }
        std::sort(tempWidgets.begin(), tempWidgets.end(), [](Widget w1, Widget w2) { return w1.id < w2.id; });
    }

    // Only load new configuration after successful completion of config parsing

    for (uint i = 0; i < widgets.size(); i++) {
        widgets[i].file.close();
        widgets[i].dirty = true; // Start dirty to ensure widgets like clocks always have content
    }

    needsUpdate = true;
    needsConfig = false;
    widgets = tempWidgets;
    vertical = tempVertical;
    brightnessDim = tempBrightnessDim;
    brightnessMode = tempBrightnessMode;
    brightnessBright = tempBrightnessBright;
    brightnessBrightHour = tempBrightnessBrightHour;
    brightnessBrightMin = tempBrightnessBrightMin;
    brightnessDimHour = tempBrightnessDimHour;
    brightnessDimMin = tempBrightnessDimMin;
    brightnessSensorBright = tempBrightnessSensorBright;
    brightnessSensorDark = tempBrightnessSensorDark;
    usingSunMoon = tempUsingSunMoon;
    backgroundColor = tempBackgroundColor;
    usingWeather = tempUsingWeather;
    weatherLocation = tempWeatherLocation;
    weatherKey = tempWeatherKey;
    metric = tempMetric;
    fastUpdate = tempFastUpdate;
    usingTransparency = tempTransparency;
    lattitude = tempLattitude;
    longitude = tempLongitude;
    muxPattern = tempMuxPattern;
    scanPattern = tempScanPattern;
    muxDelay = tempMuxDelay;
    if (location != tempLocation)
        needsTimezone = true;
    location = tempLocation;

    display.setFastUpdate(fastUpdate);
    display.setMuxPattern(static_cast<mux_patterns>(muxPattern));
    display.setScanPattern(static_cast<scan_patterns>(scanPattern));
    display.setMuxDelay(muxDelay, muxDelay, muxDelay, muxDelay, muxDelay);
    display.setRotate(vertical);
    display.setRotation(vertical);
    displayBuffer.setRotate(vertical);
    displayBuffer.setRotation(vertical);

    weatherUpdateTime = 0;
    sunMoonTime = 0;

    Serial.println(F("Successfully loaded configuration!"));
#ifdef ESP8266
    Serial.printf("Memory Free: %i, Fragmentation: %i%%\n", ESP.getFreeHeap(), ESP.getHeapFragmentation());
#endif
    return true;
}

bool getConfig() {
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println(F("Failed to open config file"));
        return false;
    }

    size_t size = configFile.size();
    Serial.print(F("Configuration file size: "));
    Serial.println(size);
    if (size > MAX_CONFIG_FILE_SIZE) {
        Serial.println(F("Config file size is too large, replacing with default..."));
        configFile.close();
        writeDefaultConfig();
        return false;
    }

    DynamicJsonDocument doc(CONFIG_JSON_BUFFER_SIZE);
    DeserializationError deserializeError = deserializeJson(doc, configFile);
    configFile.close();
    if (deserializeError) {
        Serial.printf("Failed to parse config: %s\n, replacing with default...\n", deserializeError.c_str());
        writeDefaultConfig();
        return false;
    }
    
    String error;
    JsonVariant json = doc.as<JsonVariant>();
    if (!parseConfig(json, error)) {
        Serial.printf("Failed to parse config: %s\n, replacing with default...\n", error.c_str());
        writeDefaultConfig();
        return false;
    }

    return true;
}

#pragma endregion


#pragma region Web Server

void closeConnection(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200);
    response->addHeader("Connection", "close");
    request->send(response);
}

void onStartAccessPoint(AsyncWiFiManager *wiFiManager) {
    Serial.println(F("Entering AP config mode"));
    display.setCursor(0, 0);
    display.println(F("Wifi"));
    display.println(F("Config"));
    display.println(F("Mode"));
    display.showBuffer();
    needsConfig = true;
}

String getContentType(String filename) {
    if (filename.endsWith(F(".html"))) 
        return F("text/html");
    else if (filename.endsWith(F(".css"))) 
        return F("text/css");
    else if (filename.endsWith(F(".js"))) 
        return F("application/javascript");
    else if (filename.endsWith(F(".ico"))) 
        return F("image/x-icon");
    else if (filename.endsWith(F(".json"))) 
        return F("text/json");
    return F("text/plain");
}

int getHeaderCacheVersion(AsyncWebServerRequest *request) {
    if (!request->hasHeader(F("If-None-Match")))
        return -1;
    for (int i = 0; i < request->headers(); i++ )
        if (request->headerName(i).equals(F("If-None-Match")))
            return request->header(i).toInt();
    return -1;
}

bool sendFile(AsyncWebServerRequest *request, String path, bool cache = true) {
    if (path.endsWith(F("/")))
        return false;
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, path, getContentType(path));
    if (!response)
        return false;
    request->send(response);
    return true;
}

static const char noDashboardIndex[] PROGMEM = R"(
  <!DOCTYPE html>
  <html>
  <body>
  <h1>Dashboard is not installed</h1>
  <a href="https://github.com/TheLogicMaster/ESP-LED-Matrix-Display/releases/latest" target="_blank">Get Dashboard</a>
  <a href="/recovery">Install Dashboard</a>
  </body>
  </html>
)";

void serveRoot(AsyncWebServerRequest *request) {
    if (!sendFile(request, F("/index.html")))
        request->send(200, "text/html", noDashboardIndex);
}

void serveSaveConfig(AsyncWebServerRequest *request, JsonVariant &json) {
    // Todo: Ensure not closed while rendering or crash
    //if (displayBuffer.isAllocated())
    //    displayBuffer.close();
    for (uint i = 0; i < widgets.size(); i++) {
        widgets[i].file.close();
#if USE_TETRIS
        widgets[i].tetris.reset();
#endif
    }
        needsUpdate = true;

        String error;
        if (parseConfig(json, error)) {
            LittleFS.remove("/config.json");
            File file = LittleFS.open("/config.json", "w");
            serializeJson(json, file);
            file.close();
            Serial.println(F("Successfully updated config file"));
            request->send(200);
        } else {
            Serial.printf("Failed to update config file: %s\n", error.c_str());
            request->send(400, F("text/plain"), error);
        }
}

void serveConfig(AsyncWebServerRequest *request) {
    if (!sendFile(request, F("/config.json"), false)) {
        writeDefaultConfig();
        if (!sendFile(request, F("/config.json"), false))
            request->send(500, F("text/plain"), F("Config file error"));
    }
}

void serveFullUpdate(AsyncWebServerRequest *request) {
    needsUpdate = true;
    request->send(200);
}

void serveForceUpdate(AsyncWebServerRequest *request) {
    for (uint i = 0; i < widgets.size(); i++)
        widgets[i].lastUpdate = 0;
    request->send(200);
}

void serveWeatherUpdate(AsyncWebServerRequest *request) {
    weatherUpdateTime = 0;
    request->send(200);
}

void serveSunMoonUpdate(AsyncWebServerRequest *request) {
    sunMoonTime = 0;
    request->send(200);
}

void serveTimezoneUpdate(AsyncWebServerRequest *request) {
    needsTimezone = true;
    request->send(200);
}

void serveTimeUpdate(AsyncWebServerRequest *request) {
    timezone.setEvent(updateNTP);
    setTime(0);
    request->send(200);
}

void serveNotFound(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
        request->send(200);
        return;
    }
  
    DEBUG("Not found: %s %s %s\n", request->url().c_str(), request->contentType().c_str(), request->methodToString());
    if (!sendFile(request, request->url()))
        request->send(404, "text/html", F("Not found"));
}

void serveRestart(AsyncWebServerRequest *request) {
    closeConnection(request);
    needsRestart = true;
}

// Todo: Replace start and abort OTA functions with direct update functions
void startOTA(AsyncWebServerRequest *request) {
    state = STATE_UPDATING;
    disableDisplay = true;
    LittleFS.end();
    request->send(200);
}

void abortOTA(AsyncWebServerRequest *request) {
    state = STATE_NORMAL;
    disableDisplay = false;
    LittleFS.begin();
    request->send(200);
    needsUpdate = true;
}

void serveStats(AsyncWebServerRequest *request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonVariant& json = response->getRoot();
#ifdef ESP32
    uint features = 1;
#else
    uint features = 0;
#endif
    features |= USE_BRIGHTNESS_SENSOR << 1;
    features |= USE_DOUBLE_BUFFERING << 2;
    features |= USE_HTTPS << 3;
    features |= USE_NTP << 4;
    features |= USE_PROGMEM_IMAGES << 5;
    features |= USE_SUNRISE << 6;
    features |= USE_TETRIS << 7;
    features |= USE_WEATHER << 8;
    json["features"] = features;
#if USE_NTP
    json["uptime"] = timezone.tzTime(TIME_NOW, UTC_TIME) - bootTime;
#endif
    json["version"] = VERSION_CODE;
#ifdef ESP8266
    char reason[25];
    json["resetReason"] = strcpy(reason, ESP.getResetReason().c_str());
    json["fragmentation"] = ESP.getHeapFragmentation();
    FSInfo info;
    LittleFS.info(info);
    json["filesystemUsed"] = info.usedBytes;
    json["filesystemTotal"] = info.totalBytes;
    json["maxOpenFiles"] = info.maxOpenFiles;
    json["maxPathLength"] = info.maxPathLength;
    json["vcc"] = ESP.getVcc();
    uint16_t totalMemory;
    ESP.getHeapStats(nullptr, &totalMemory, nullptr);
    json["memoryTotal"] = totalMemory;
#else
    // Todo: Improve fragmentation calculation
    json["fragmentation"] = (uint)((ESP.getFreeHeap() - ESP.getMaxAllocHeap()) / (double)ESP.getFreeHeap() * 100);
    json["memoryTotal"] = ESP.getHeapSize();
    json["lowestMemory"] = ESP.getMinFreeHeap();
    json["filesystemUsed"] = LittleFS.usedBytes();
    json["filesystemTotal"] = LittleFS.totalBytes();
    json["maxPathLength"] = 31;
#endif
    json["memoryFree"] = ESP.getFreeHeap();
    json["frequency"] = ESP.getCpuFreqMHz();
    json["transparencyBuffer"] = displayBuffer.isAllocated();
    json["platform"] = BOARD_NAME;
    json["width"] = vertical ? DISPLAY_HEIGHT : DISPLAY_WIDTH;
    json["height"] = vertical ? DISPLAY_WIDTH : DISPLAY_HEIGHT;
    json["brightness"] = currentBrightness;
    json["brightnessSensor"] = analogRead(BRIGHTNESS_SENSOR_PIN);
    response->setLength();
    request->send(response);
}

void writeDefaultImageData() {
    File dataFile = LittleFS.open(F("/imageData.json"), "w");
    dataFile.print("{}");
    dataFile.close();
}

std::map <std::string, FSImage> getFSImageData() {
    std::map <std::string, FSImage> data;

    File dataFile = LittleFS.open("/imageData.json", "r");
    if (!dataFile) {
        Serial.println(F("Failed to open image data file, writing default"));
        writeDefaultImageData();
        dataFile = LittleFS.open("/imageData.json", "r");
    }

    size_t size = dataFile.size();
    if (size > MAX_IMAGE_DATA_FILE_SIZE) {
        Serial.printf("Image data file size is too large(%i), replacing with default...\n", size);
        dataFile.close();
        writeDefaultImageData();
        dataFile = LittleFS.open("/imageData.json", "r");
    }

    std::unique_ptr<char[]> buf(new char[size]);

    dataFile.readBytes(buf.get(), size);
    dataFile.close();

    DynamicJsonDocument doc(IMAGE_JSON_BUFFER_SIZE);
    deserializeJson(doc, buf.get());
    for (auto const &entry : doc.as<JsonObject>())
        data[entry.key().c_str()] = {entry.value()["width"], entry.value()["height"], entry.value()["length"]};

    return data;
}

void writeFSImageData(std::map <std::string, FSImage> data) {
    File dataFile = LittleFS.open("/imageData.json", "w");
    if (!dataFile) {
        Serial.println(F("Failed to open Image Data file"));
        return;
    }
    DynamicJsonDocument doc(IMAGE_JSON_BUFFER_SIZE);
    for (auto const &entry : data) {
        JsonObject o = doc.createNestedObject(entry.first.c_str());
        o["width"] = entry.second.width;
        o["height"] = entry.second.height;
        o["length"] = entry.second.length;
    }
    std::unique_ptr<char[]> buf(new char[MAX_IMAGE_DATA_FILE_SIZE]);
    serializeJson(doc, buf.get(), MAX_IMAGE_DATA_FILE_SIZE);
    dataFile.print(buf.get());
    dataFile.close();
}

Dir getImageDir() {
    if (!LittleFS.exists("/images"))
        LittleFS.mkdir("/images");
#ifdef ESP32
    return LittleFS.open("/images");
#else
    return LittleFS.openDir("/images");
#endif
}

void serveImageData(AsyncWebServerRequest *request) {
    Dir imageDir = getImageDir();
    std::map <std::string, FSImage> fsImageData = getFSImageData();

    size_t size = 100 + (progmemImages.size() + fsImageData.size()) * 100;
    DynamicJsonDocument doc(size);

#if DEBUG_TRANSPARENCY
    {
        JsonObject o = doc.createNestedObject("Transparency Buffer");
        o["width"] = DISPLAY_WIDTH;
        o["height"] = DISPLAY_HEIGHT;
        o["length"] = 1;
        o["type"] = IMAGE_UINT8;
        o["progmem"] = false;
    }
#endif

    for (auto const &entry : progmemImages) {
        JsonObject o = doc.createNestedObject(entry.first.c_str());
        o["width"] = entry.second.width;
        o["height"] = entry.second.height;
        o["length"] = entry.second.length;
        o["type"] = entry.second.type;
        o["progmem"] = true;
        }

    for (auto const &entry : fsImageData) {
        JsonObject o = doc.createNestedObject(entry.first.c_str());
        o["width"] = entry.second.width;
        o["height"] = entry.second.height;
        o["length"] = entry.second.length;
        o["progmem"] = false;
    }

    std::unique_ptr<char[]> buf(new char[size]);
    serializeJson(doc, buf.get(), size);
    request->send(200, F("text/json"), buf.get());
}

void serveUploadImage(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    for (uint i = 0; i < widgets.size(); i++)
        widgets[i].file.close();
    String imageFilename = "/images/" + request->arg("name");

    if (!index) {
        DEBUG("Uploading Image: %s\n", request->arg("name").c_str());
        request->_tempFile = LittleFS.open(imageFilename, "w");
    }

    if (request->_tempFile && request->_tempFile.write(data, len) != len) {
        Serial.printf("Error saving image: %i\n", request->_tempFile.getWriteError());
        request->_tempFile.close();
    }

    if (final) {
        if (request->_tempFile) {
            std::map <std::string, FSImage> fsImageData = getFSImageData();
            fsImageData[request->arg(F("name")).c_str()] = {(uint8_t)strtoul(request->arg(F("width")).c_str(), NULL, 10),
                                                        (uint8_t)strtoul(request->arg(F("height")).c_str(), NULL, 10),
                                                        (uint8_t)strtoul(request->arg(F("length")).c_str(), NULL, 10)};
            writeFSImageData(fsImageData);
            needsUpdate = true;
        } else {
            request->send(400, F("text/plain"), F("Failed to save image"));
            if (LittleFS.exists(imageFilename))
                LittleFS.remove(imageFilename);
        }
    }
}

void serveImage(AsyncWebServerRequest *request) {
    String image = request->arg(F("image"));
    if (image.endsWith(F("_P"))) {
        std::string progmem = std::string(image.substring(0, image.length() - 2).c_str());
        if (progmemImages.count(progmem) < 1) {
            request->send(404);
            return;
        }
        size_t size = (progmemImages[progmem].type == IMAGE_GIMP ? 4 : 2) * progmemImages[progmem].width *
                    progmemImages[progmem].height * progmemImages[progmem].length;
        request->send_P(200, F("application/binary"), (uint8_t*) progmemImages[progmem].data, size);
    } else {
        if (!sendFile(request, "/images/" + image, false)) {
            DEBUG("Image doesn't exist: %s\n", image.c_str());
            request->send(404);
        }
    }
}

void serveRenameImage(AsyncWebServerRequest *request) {
    String name = request->arg(F("name"));
    String newName = request->arg(F("newName"));
    if (!LittleFS.exists("/images/" + name)) {
        DEBUG("Image to rename doesn't exist: %s\n", name.c_str());
        request->send(404);
    } else {
        if (LittleFS.exists("/images/" + newName))
            LittleFS.remove("/images/" + newName);
        LittleFS.rename("/images/" + name, "/images/" + newName);
        std::map <std::string, FSImage> fsImageData = getFSImageData();
        fsImageData[newName.c_str()] = fsImageData[name.c_str()];
        fsImageData.erase(name.c_str());
        writeFSImageData(fsImageData);
        request->send(200);
    }
}

void serveDeleteImage(AsyncWebServerRequest *request) {
    for (uint i = 0; i < widgets.size(); i++)
        widgets[i].file.close();
    std::string name = request->arg(F("name")).c_str();
    std::map <std::string, FSImage> fsImageData = getFSImageData();
    fsImageData.erase(name);
    std::string filename = "/images/" + name;
    if (LittleFS.exists(filename.c_str()))
        LittleFS.remove(filename.c_str());
    writeFSImageData(fsImageData);
    request->send(200);
}

void deleteAllImages() {
    for (uint i = 0; i < widgets.size(); i++)
        widgets[i].file.close();
    writeDefaultImageData();
    Dir dir = getImageDir();
    LittleFS.rmdir(F("/images"));
    LittleFS.mkdir(F("/images"));
#ifdef ESP8266
    std::vector <String> files;
    while (dir.next()) {
        Serial.println(dir.fileName().c_str());
        files.push_back(dir.fileName());
    }
    for (const auto &file: files)
        LittleFS.remove("/images/" + file);
#endif
}

void serveDeleteAllImages(AsyncWebServerRequest *request) {
    deleteAllImages();
    request->send(200);
}

void serveResetConfiguration(AsyncWebServerRequest *request) {
    writeDefaultConfig();
    closeConnection(request);
    needsRestart = true;
}

void serveOK(AsyncWebServerRequest *request) {
    request->send(200);
}

void serveFactoryReset(AsyncWebServerRequest *request) {
    writeDefaultConfig();
    deleteAllImages();
    AsyncWiFiManager wifiManager(&server, &dnsServer);
    wifiManager.resetSettings();
    needsRestart = true;
    closeConnection(request);
}

void serveOTA(AsyncWebServerRequest *request) {
    needsRestart = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", needsRestart ? "OK" : String(Update.getError(), 10));
    response->addHeader("Connection", "close");
    request->send(response);
}

void serveOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
#ifdef ESP8266
        Update.runAsync(true);
#endif
        if (filename.startsWith("firmware")) {
            Serial.println("Starting Firmware update");
            if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
                Update.printError(Serial);
        } else {
            Serial.println("Starting Filesystem update");
            LittleFS.end();
#ifdef ESP8266
            if (!Update.begin((size_t) &_FS_end - (size_t) &_FS_start, U_FS))
#else
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS))
#endif
                Update.printError(Serial);
        }
    }
    if (!Update.hasError() && Update.write(data, len) != len)
      Update.printError(Serial);
    if (final) {
      if(Update.end(true))
        Serial.printf("Update Success: %uB\n", index+len);
      else
        Update.printError(Serial);
    }
}

static const char recoveryIndex[] PROGMEM = R"(<!DOCTYPE html>
     <html lang='en'>
     <head>
         <meta charset='utf-8'>
         <meta name='viewport' content='width=device-width,initial-scale=1'/>
     </head>
     <body>
     <h1>Display Recovery</h1>
     <form method='POST' action='/update' enctype='multipart/form-data'>
         Firmware:<br>
         <input type='file' accept='.bin,.bin.gz' name='firmware'>
         <input type='submit' value='Install Firmware'>
     </form>
     <form method='POST' action='/update' enctype='multipart/form-data'>
         Dashboard:<br>
         <input type='file' accept='.bin,.bin.gz' name='filesystem'>
         <input type='submit' value='Install Dashboard'>
     </form>
     </body>
     </html>)";

void serveRecovery(AsyncWebServerRequest *request) {
    request->send_P(200, F("text/html"), recoveryIndex);
}

void setupWebserver() {
    server.on("/", serveRoot);
    server.on("/index.html", serveRoot);
    server.on("/config", HTTP_GET, serveConfig);
    server.addHandler(new AsyncCallbackJsonWebHandler("/config", serveSaveConfig, MAX_CONFIG_FILE_SIZE));
    server.on("/update", HTTP_POST, serveOTA, serveOTAUpload);
    server.on("/recovery", HTTP_GET, serveRecovery);
    server.on("/abortUpdate", abortOTA);
    server.on("/beginUpdate", startOTA);
    server.on("/fullRefresh", serveFullUpdate);
    server.on("/forceUpdate", serveForceUpdate);
    server.on("/refreshWeather", serveWeatherUpdate);
    server.on("/refreshSunMoon", serveSunMoonUpdate);
    server.on("/refreshTimezone", serveTimezoneUpdate);
    server.on("/refreshTime", serveTimeUpdate);
    server.on("/restart", serveRestart);
    server.on("/stats", serveStats);
    server.on("/uploadImage", HTTP_POST, serveOK, serveUploadImage);
    server.on("/image", serveImage);
    server.on("/images", serveImageData);
    server.on("/deleteImage", serveDeleteImage);
    server.on("/deleteAllImages", serveDeleteAllImages);
    server.on("/renameImage", serveRenameImage);
    server.on("/factoryReset", serveFactoryReset);
    server.on("/resetConfiguration", serveResetConfiguration);
    server.serveStatic("/", LittleFS, "/", CACHE_DASHBOARD ? "no-cache" : NULL);
    server.onNotFound(serveNotFound);
    Serial.println(F("Starting configuration webserver..."));
    server.begin();
}

#pragma endregion

#pragma region Rendering

rgbColor unpack565Color(uint16_t color) {
    return {
        (uint8_t)((color & 0xF800) >> 11), 
        (uint8_t)((color & 0x7E0) >> 5), 
        (uint8_t)(color & 0x1F)
    };
}

bool checkAlphaColors(std::vector<uint16_t> alphaColors, uint16_t color) {
    for (uint i = 0; i < alphaColors.size(); i++) {
        if (alphaColors[i] == color)
            return true;
        rgbColor c0 = unpack565Color(alphaColors[i]);
        rgbColor c1 = unpack565Color(color);
        if (pow(c0.r - c1.r, 2) + pow(c0.g - c1.g, 2) + pow(c0.b - c1.b, 2) < ALPHA_COLOR_DISTANCE)
            return true;
    }
    return false;
}

void drawImage(Adafruit_GFX &d, uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height, uint8_t offset,
                    uint8_t *data, std::vector<uint16_t> alphaColors, bool transparent, bool uInt16) {
    uint16_t line_buffer[width];
    for (uint8_t y = 0; y < height; y++) {
        memcpy_P(line_buffer, data + (offset * width * height + y * width) * 2, width * 2);
        for (uint8_t x = 0; x < width; x++) {
            if (transparent && checkAlphaColors(alphaColors, line_buffer[x]))
                continue;
            d.drawPixel(xOffset + x, yOffset + y, line_buffer[x]);
        }
#if defined(ESP8266)
        if (width * height > PIXEL_YIELD_THRESHOLD)
            yield();
#endif
    }
}

void drawImageFs(Adafruit_GFX &d, uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height, uint8_t offset,
                      const char name[], std::vector<uint16_t> alphaColors, bool transparent, File &file) {
    if (!file) {
        if (!LittleFS.exists(name)) {
            Serial.print(F("Couldn't find image to draw for: "));
            Serial.println(name);
            return;
        }
        Serial.printf("Opening File: %s\n", name);
        file = LittleFS.open(name, "r");
        if (!file) {
            Serial.printf("Failed to open image: %s\n", name);
            return;
        }
    }

    char line_buffer[width * 2];
    file.seek(offset * width * height * 2);
    for (uint8_t y = 0; y < height; y++) {
        file.readBytes(line_buffer, width * 2);
        for (uint8_t x = 0; x < width; x++) {
            uint16_t color = line_buffer[x * 2] | line_buffer[x * 2 + 1] << 8;
            if (transparent && checkAlphaColors(alphaColors, color))
                continue;
            d.drawPixel(xOffset + x, yOffset + y, color);
        }
#if defined(ESP8266)
        if (width * height > PIXEL_YIELD_THRESHOLD)
            yield();
#endif
    }
}

void
drawGimpImage(Adafruit_GFX &display, uint8_t xOff, uint8_t yOff, uint8_t width, uint8_t height, std::vector<uint16_t> alphaColors, bool transparent, char *data) {
    for (uint y = 0; y < height; y++) {
        for (uint x = 0; x < width; x++) {
            uint offset = (x + y * width) * 4;
            char c0 = data[offset];
            char c1 = data[offset + 1];
            char c2 = data[offset + 2];
            char c3 = data[offset + 3];
            uint8_t r = (((c0 - 33) << 2) | ((c1 - 33) >> 4));
            uint8_t g = ((((c1 - 33) & 0xF) << 4) | ((c2 - 33) >> 2));
            uint8_t b = ((((c2 - 33) & 0x3) << 6) | ((c3 - 33)));
            uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            if (transparent && checkAlphaColors(alphaColors, color))
                continue;
            display.drawPixel(x + xOff, y + yOff, color);
        }
#if defined(ESP8266)
        if (width * height > PIXEL_YIELD_THRESHOLD)
            yield();
#endif
    }
}

void drawAnalogClock(Adafruit_GFX &d, uint8_t xOffset, uint8_t yOffset, uint8_t radius, uint16_t colorMinute,
                     uint16_t colorHour, uint16_t colorMarking) {
    double minAngle = 2 * PI * (theMinute / 60. - .25);
    double hourAngle = 2 * PI * (theHour12 / 12. + theMinute / 720. - .25);
    for (uint i = 0; i < 12; i++) {
        double angle = 2 * PI * i / 12.;
        d.drawPixel(xOffset + round(cos(angle) * radius), yOffset + round(sin(angle) * radius), colorMarking);
    }
    d.drawLine(xOffset, yOffset,
               xOffset + round(cos(minAngle) * radius * .9),
               yOffset + round(sin(minAngle) * radius * .9), colorMinute);
    d.drawLine(xOffset, yOffset,
               xOffset + round(cos(hourAngle) * radius * .5),
               yOffset + round(sin(hourAngle) * radius * .5), colorHour);
}

String getTimeText(uint8_t type) {
    String text;
    uint8_t hourValue = type == CLOCK_TYPE_24 ? theHour : theHour12;
    if (hourValue < 10)
        text += "0";
    text += hourValue;
    text += ":";
    if (theMinute < 10)
        text += "0";
    text += theMinute;
    if (type == CLOCK_TYPE_12_PERIOD)
        text += theHour < 12 ? "AM" : "PM";
    return text;
}

void drawText(Adafruit_GFX &d, const char *text, uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height,
                 uint16_t color, uint8_t wrapping, uint8_t size) {
    d.setTextSize(size);
    d.setTextColor(color);
    d.setCursor(xOffset, yOffset);
    d.print(text);
}

void drawTinyText(Adafruit_GFX &display, const char text[], uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                  uint16_t color, bool transparent, uint16_t backgroundColor, uint8_t wrapping) {
    TFDrawText(display, text, x, y, color, transparent, backgroundColor);
}

void drawTextWidget(Adafruit_GFX &d, Widget &widget) {
    if (widget.font >= FONT_GFX)
                drawText(d, widget.content.c_str(),
                        widget.xOff + _max(0, (widget.width - 6 * (int) widget.content.length() + 1) / 2),
                        widget.yOff + (widget.height - 7) / 2, widget.width, widget.height, widget.colors[0],
                        widget.contentType, widget.font - FONT_GFX + 1);
            else if (widget.font == FONT_TINY)
                drawTinyText(d, widget.content.c_str(),
                         widget.xOff + _max(0, (widget.width - (TF_COLS + 1) * (int) widget.content.length() + 1) / 2),
                         widget.yOff + (widget.height - TF_ROWS) / 2, widget.width, widget.height, widget.colors[0],
                         true, 0, widget.contentType);
#if USE_TETRIS
            else if (widget.tetris) {
                widget.tetris->setText(widget.content.c_str());
                uint x = widget.xOff + _max(0, (widget.width - (2 + TETRIS_DISTANCE_BETWEEN_DIGITS * widget.content.length())) / 2);
                uint y = widget.yOff + _max(0, (widget.height - 20) / 2);
                widget.finished = widget.tetris->drawText(x, y + 20);
                if (widget.type == WIDGET_CLOCK && widget.stateExtra) {
                    d.drawRect(x + 16, y + 13, 2, 2, widget.colors[0]);
                    d.drawRect(x + 16, y + 17, 2, 2, widget.colors[0]);
                }
            }
#endif
}

bool sendGetRequest3(std::string url, std::string auth, uint16_t timeout, bool json, std::string &cert, std::string &result) {
    HTTPClient httpClient;
    Serial.println(cert.c_str());
    httpClient.begin(url.c_str());
    //httpClient.begin(url.c_str(), cert.length() == 0 ? NULL : cert.c_str());
    httpClient.GET();
    Serial.println(httpClient.getString());
    httpClient.end();
    return false;
}

bool sendGetRequest(std::string url, std::string auth, uint16_t timeout, bool json, std::string &cert, std::string &result) {
    if (!String(url.c_str()).startsWith(F("http"))) {
        Serial.println("Invalid GET protocol");
        return false;
    }
    bool ssl = String(url.c_str()).startsWith(F("https"));
    uint port = ssl ? 443 : 80;
    std::string address = ssl ? url.substr(8) : url.substr(7);
    uint addressEnd = address.find_first_of('/');
    std::string hostPort = address.substr(0, addressEnd);
    uint hostEnd = address.find_first_of(':');
    std::string host = hostPort.substr(0, hostEnd);
    if (hostEnd < hostPort.length())
        port = strtoul(hostPort.substr(hostEnd + 1).c_str(), NULL, 10);

    std::unique_ptr<WiFiClient> clientPtr;

    if (DELETE_TRANSPARENCY_BUFFER_ON_HTTPS && ssl)
        displayBuffer.close();

#if USE_HTTPS
    clientPtr.reset(ssl ? new WiFiClientSecure() : new WiFiClient());
#else
    if (ssl)
        return false;
    clientPtr.reset(new WiFiClient());
#endif
    WiFiClient* client = clientPtr.get();
    if (ssl) {
        WiFiClientSecure* secureClient = (WiFiClientSecure*)client;
#ifdef ESP8266
        if (cert.size() == 0) 
            secureClient->setInsecure();
        else  // Todo: Test ESP8266 certificate handling
            secureClient->setCACert((const uint8_t*)cert.c_str(), cert.size());        
        // Impossible to send requests without limiting buffer sizes, though this may cause issues receiving responses if server doesn't support MFLN
        secureClient->setBufferSizes(HTTPS_RECEIVE_BUFFER, HTTPS_TRANSMIT_BUFFER);
        client->setTimeout(timeout);
#else
        if (cert.size() > 0) 
            secureClient->setCACert(cert.c_str());
        client->setTimeout(timeout / 1000);
#endif
    }
    //client->flush();
    DEBUG("Connecting to: %s\n", host.c_str());
#ifdef ESP32 // Display task causes TLS handshake failure
    disableDisplay = true;
#endif
	if (!client->connect(host.c_str(), port)) {
        Serial.println(F("Failed to connect to GET address"));
        disableDisplay = false;
        return false;
    }
    disableDisplay = false;

    #if defined(ESP8266)
        yield();
    #endif

    client->print(F("GET "));
    client->print(address.substr(addressEnd).c_str());
    client->println(F(" HTTP/1.1"));
    client->print(F("Host: "));
    client->println(host.c_str());
    if (auth.length() > 0)
        client->println(("Authorization: " + auth).c_str());
    // client->println(F("Cache-Control: no-cache")); // Needed for such a request?
    if (client->println() == 0) {
        Serial.println(F("Failed to send request"));
        return false;
    }

    // Get status code
    int statusCode = -1;
    if(client->find("HTTP/")) {
        client->parseFloat();
        statusCode = client->parseInt();
    }
    if (statusCode != 200) {
        Serial.print(F("Unexpected HTTP status code: "));
        Serial.println(statusCode);
        return false;
    }

    // Skip headers
    bool body = false;
    while (client->available()) {
        String line = client->readStringUntil('\n');
        if (line.equals("\r")) {
            body = true;
            break;
        }
    }
    if (!body) {
        Serial.println("Failed to read GET response body");
        return false;
    }

    while (json && client->available() && client->peek() != '{') {
        char c = 0;
        client->readBytes(&c, 1);
        DEBUG("Unexpected body character: %c\n", c);
    }

    result += client->readString().c_str();
    DEBUG("GET Response Body: %s\n", result.c_str());
    return true;
}

const char *jsonVariantToString(JsonVariant &variant) {
    if (variant.is<char*>())
        return variant.as<char *>();
    else if (variant.is<bool>())
        return variant.as<bool>() ? "true" : "false";
    else if (variant.is<long>())
        return String(variant.as<long>(), 10).c_str();
        else if (variant.is<float>())
        return String(variant.as<float>(), 2).c_str();
    else {
        Serial.println("Unknown JSON type");
        return "";
    }
}

void ensureTetrisAllocated(Widget &widget) {
#if USE_TETRIS
    if (!widget.tetris) {
        widget.tetris.reset(new TetrisMatrixDraw(display));
        DEBUG("Allocating Tetris object\n");
    }
    if (!widget.tetris)
        DEBUG("Failed to create Tetris object\n");
#endif
}

void updateTextGETWidgetJson(Widget &widget) {
    std::string data;
    if (!sendGetRequest(widget.source, widget.auth, widget.length, true, widget.cert, data))
        return;

    DynamicJsonDocument doc(JSON_REQUEST_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
        Serial.print(F("Failed to deserialize response data: "));
        Serial.println(error.c_str());
        widget.content = "ERROR";
    } else {
        JsonVariant current;
        if (doc.is<JsonArray>())
            current = doc.as<JsonArray>();
        else
            current = doc.as<JsonObject>();
        if (!current) {
            Serial.println("Failed to parse JSON request root");
            return;
        }
        for (uint i = 0; i < widget.args.size(); i++) {
            if (current.is<JsonArray>())
                current = current[strtol(widget.args[i].c_str(), NULL, 10)];
            else
                current = current[widget.args[i].c_str()];
            if (!current) {
                Serial.printf("Failed to get JSON response value, key: %s\n", widget.args[i].c_str());
                return;
            }
        }
        String s = String(jsonVariantToString(current));
        if (widget.font == FONT_TINY)
            s.toUpperCase();
        if (widget.font == FONT_TETRIS)
            ensureTetrisAllocated(widget);
        widget.content = std::string(s.c_str());
        widget.dirty = true;
    }
}

void updateTextGETWidget(Widget &widget) {
    std::string data;
    if (!sendGetRequest(widget.source, widget.auth, widget.length, false, widget.cert, data))
        return;
    String s = String(data.c_str());
    if (widget.font == FONT_TINY)
        s.toUpperCase();
    if (widget.font == FONT_TETRIS)
            ensureTetrisAllocated(widget);
    widget.content = std::string(s.c_str());
    widget.dirty = true;
}

void incrementWidgetState(Widget &widget, uint8_t max) {
    if (max > 0) {
        widget.state++;
        widget.state %= max;
        widget.dirty = true;
    }
}

void incrementWidgetState(Widget &widget) {
    incrementWidgetState(widget, widget.length);
}

void updateClockWidget(Widget &widget) {
    if (widget.state != theMinute || widget.dirty) {
        widget.state = theMinute;
        widget.dirty = true;
        widget.finished = false;
        if (widget.type != WIDGET_ANALOG_CLOCK)
            widget.content = getTimeText(widget.contentType).c_str();
    }

    time_t t = millis();
    if (widget.stateExtra != (bool)(t & 1024)) {
        widget.stateExtra = (bool)(t & 1024);
        widget.dirty = true;
        String temp = String(widget.content.c_str());
        if (widget.stateExtra || widget.font == FONT_TETRIS)
            temp.replace(':', ' ');
        else
            temp.replace(' ', ':');
        widget.content = temp.c_str();
    }

#if USE_TETRIS
    if (widget.font == FONT_TETRIS) {
        ensureTetrisAllocated(widget);
        if (widget.tetris) {
            if (!widget.finished)
                widget.dirty = true;
        }
    }
#endif
}

void updateVariableWidget(Widget &widget) {
    String content;
    switch (widget.offset) {
        case VARIABLE_TEMPERATURE:
            content = temperature;
            break;
        case VARIABLE_TEMPERATURE_PERCEIVED:
            content = temperaturePerceived;
            break;
        case VARIABLE_TEMPERATURE_HIGH:
            content = temperatureMax;
            break;
        case VARIABLE_TEMPERATURE_LOW:
            content = temperatureMin;
            break;
        case VARIABLE_HUMIDITY:
            content = humidity;
            break;
        case VARIABLE_PRESSURE:
            content = pressure;
            break;
        case VARIABLE_VISIBILITY:
            content = visibility;
            break;
        case VARIABLE_WIND_SPEED:
            content = windSpeed;
            break;
        default:
            Serial.printf("Invalid variable id: %i\n", widget.offset);
            break;
    }
    if (widget.length)
        switch (widget.offset) {
            case VARIABLE_TEMPERATURE:
            case VARIABLE_TEMPERATURE_PERCEIVED:
            case VARIABLE_TEMPERATURE_HIGH:
            case VARIABLE_TEMPERATURE_LOW:
                content.concat(metric ? 'C' : 'F');
                break;
            case VARIABLE_HUMIDITY:
                content.concat('%');
                break;
            case VARIABLE_WIND_SPEED:
                content.concat(metric ? "KPH" : "MPH");
                break;
            case VARIABLE_PRESSURE:
                content.concat("mBar");
                break;
            case VARIABLE_VISIBILITY:
                content.concat("m");
                break;
            default:
                break;
        }
    if (widget.font == FONT_TINY)
        content.toUpperCase();
    if (content != widget.content.c_str()) {
        widget.dirty = true;
        widget.content = content.c_str();
    }
}

void updateWidget(Widget &widget) {
    switch (widget.type) {
        case WIDGET_PROGMEM_IMAGE:
        case WIDGET_FS_IMAGE:
            incrementWidgetState(widget);
            break;
        case WIDGET_TEXT_GET_JSON:
            updateTextGETWidgetJson(widget);
            break;
        case WIDGET_TEXT_GET:
            updateTextGETWidget(widget);
            break;
        case WIDGET_WEATHER_ICON:
            incrementWidgetState(widget, 5);
            break;
        case WIDGET_ANALOG_CLOCK:
        case WIDGET_CLOCK:
            updateClockWidget(widget);
            break;
        case WIDGET_TEXT:
        if (widget.font == FONT_TETRIS) {
            ensureTetrisAllocated(widget);
            if (!widget.finished)
                widget.dirty = true;
        }
            break;
        case WIDGET_VARIABLE:
            updateVariableWidget(widget);
            break;
        default:
            Serial.print(F("Invalid widget type to update: "));
            Serial.println(widget.type);
            break;
    }

    if (widget.background)
        needsUpdate = true;
    widget.lastUpdate = millis();
}

void updateWidgets() {
    for (uint i = 0; i < widgets.size(); i++)
        if (widgets[i].lastUpdate == 0 || (widgets[i].updateFrequency > 0 && millis() - widgets[i].lastUpdate > widgets[i].updateFrequency))
            updateWidget(widgets[i]);
}

void drawWidget(Adafruit_GFX &d, Widget &widget, bool buffering) {
    if (!usingTransparency || !widget.transparent) {
        if (widget.type == WIDGET_ANALOG_CLOCK)
            d.fillCircle(widget.xOff + widget.height / 2, widget.yOff + widget.height / 2, widget.height / 2,
                         widget.backgroundColor);
        else if (widget.type == WIDGET_SHAPE) {
            switch (widget.contentType) {
                default:
                    Serial.printf("Unknown shape: %i\n", widget.contentType);
                case SHAPE_PIXEL:
                case SHAPE_RECTANGLE:
                    d.fillRect(widget.xOff, widget.yOff, widget.width, widget.height, widget.backgroundColor);
                    break;
                case SHAPE_RECTANGLE_ROUNDED:
                    d.fillRoundRect(widget.xOff, widget.yOff, widget.width, widget.height, ROUNDED_RECT_RADIUS, widget.backgroundColor);
                    break;
                case SHAPE_CIRCLE:
                    d.fillCircle(widget.xOff + widget.height / 2, widget.yOff + widget.height / 2, widget.height / 2,
                         widget.backgroundColor);
                    break;
            }
        } else
            d.fillRect(widget.xOff, widget.yOff, widget.width, widget.height, widget.backgroundColor);
    } else if (!buffering)
        displayBuffer.write(display, widget.xOff, widget.yOff, widget.width, widget.height);


    switch (widget.type) {
        case WIDGET_PROGMEM_IMAGE:
            switch (progmemImages[widget.content].type) {
                case IMAGE_UINT8:
                case IMAGE_UINT16:
                    drawImage(d, widget.xOff + (widget.width - progmemImages[widget.content].width) / 2,
                                    widget.yOff + (widget.height - progmemImages[widget.content].height) / 2,
                                    progmemImages[widget.content].width, progmemImages[widget.content].height,
                                    widget.state + widget.offset, (uint8_t *) progmemImages[widget.content].data,
                                    widget.colors, widget.transparent, progmemImages[widget.content].type == IMAGE_UINT16);
                    break;
                case IMAGE_GIMP:
                    drawGimpImage(d, widget.xOff + (widget.width - progmemImages[widget.content].width) / 2,
                                  widget.yOff + (widget.height - progmemImages[widget.content].height) / 2,
                                  progmemImages[widget.content].width, progmemImages[widget.content].height,
                                  widget.colors, widget.transparent, (char *) progmemImages[widget.content].data);
                    break;
                default:
                    Serial.println(F("Unknown image type"));
                    break;
            }
            break;
        case WIDGET_FS_IMAGE:
            drawImageFs(d, widget.xOff, widget.yOff, widget.width, widget.height, widget.state + widget.offset,
                             ("/images/" + widget.content).c_str(), widget.colors, widget.transparent, widget.file);
            break;
        case WIDGET_ANALOG_CLOCK:
            drawAnalogClock(d, widget.xOff + widget.height / 2, widget.yOff + widget.height / 2,
                            widget.height / 2 - (widget.bordered ? 1 : 0), widget.colors[0], widget.colors[1],
                            widget.colors[2]);
            break;
        case WIDGET_TEXT_GET_JSON:
        case WIDGET_TEXT_GET:
        case WIDGET_TEXT:
        case WIDGET_CLOCK:
        case WIDGET_VARIABLE:
            drawTextWidget(d, widget);
            break;
        case WIDGET_WEATHER_ICON:
#if USE_WEATHER
            TIDrawIcon(d, weatherID, widget.xOff + _max(0, (widget.width - 10 + 1) / 2), widget.yOff + (widget.height - 5) / 2, widget.state, true, widget.transparent, widget.backgroundColor);
#endif
            break;
        case WIDGET_SHAPE:
            switch(widget.contentType) {
                default:
                case SHAPE_RECTANGLE:
                    d.drawRect(widget.xOff, widget.yOff, widget.width, widget.height, widget.colors[0]);
                    break;
                case SHAPE_RECTANGLE_ROUNDED:
                    d.drawRoundRect(widget.xOff, widget.yOff, widget.width, widget.height, ROUNDED_RECT_RADIUS, widget.colors[0]);
                    break;
                case SHAPE_CIRCLE:
                    d.drawCircle(widget.xOff + widget.height / 2, widget.yOff + widget.height / 2, widget.height / 2, widget.colors[0]);
                    break;
                case SHAPE_PIXEL:
                    break;
            }
            break;
        default:
            Serial.print(F("Invalid widget type: "));
            Serial.println(widget.type);
            break;
    }

    if (widget.bordered) {
        if (widget.type == WIDGET_ANALOG_CLOCK)
            d.drawCircle(widget.xOff + widget.height / 2, widget.yOff + widget.height / 2, widget.height / 2,
                         widget.borderColor);
        else
            d.drawRect(widget.xOff, widget.yOff, widget.width, widget.height, widget.borderColor);
    }
}

void drawScreen(Adafruit_GFX &d, bool fullUpdate, bool buffering, bool finalize) {
    if (fullUpdate)
        d.fillScreen(backgroundColor);

    for (uint i = 0; i < widgets.size(); i++) {
        Widget &widget = widgets[i];
        if (buffering && !widget.background)
            continue;

        if (!fullUpdate && !widget.dirty)
            continue;

        profileTime = millis();
        drawWidget(d, widget, buffering);
        if (PROFILE_RENDERING)
            Serial.printf("Render time for %i: %lu\n", widget.id, millis() - profileTime);

        if (finalize)
            widget.dirty = false;
    }
}

void updateScreen(bool fullUpdate) {
    needsUpdate = false;
    updateWidgets();
    if (needsUpdate)
        return;
    if (usingTransparency && fullUpdate) {
        if (!displayBuffer.isAllocated() && !displayBuffer.allocate())
            Serial.println(F("Can't draw screen on null buffer"));
        else {
            drawScreen(displayBuffer, fullUpdate, true, false);
#if DEBUG_TRANSPARENCY
            File f = LittleFS.open("/images/Transparency Buffer", "w");
            if (vertical)
                for (uint y = 0; y < DISPLAY_HEIGHT; y++)
                    for (uint x = 0; x < DISPLAY_WIDTH; x++) 
                        f.write((uint8_t*)displayBuffer.getBuffer() + (x * DISPLAY_HEIGHT + DISPLAY_HEIGHT - 1 - y) * 2, 2);
            else
                f.write((uint8_t*)displayBuffer.getBuffer(), DISPLAY_HEIGHT * DISPLAY_WIDTH * 2);
            f.close();
#endif
        }
    }
    if (USE_DOUBLE_BUFFERING) {
        drawScreen(display, fullUpdate, false, false);
        display.showBuffer();
    }
    drawScreen(display, fullUpdate, false, true);
}

#pragma endregion

// Takes total minutes in day as values
bool isTimeInRange(uint16_t startMins, uint16_t endMins, uint16_t testMins) {
    if (startMins > endMins)
        return testMins >= startMins || testMins < endMins;
    else
        return testMins >= startMins && testMins < endMins;
}

void updateBrightness() {
    uint16_t avg, constrained;
    switch (brightnessMode) {
        case BRIGHTNESS_AUTO:
            if(BRIGHTNESS_ROLLING_AVG_SIZE > 0) {
                brightnessAverage.addValue(analogRead(BRIGHTNESS_SENSOR_PIN));
                avg = brightnessAverage.getAverage();
            }
            else
                avg = analogRead(BRIGHTNESS_SENSOR_PIN);
            constrained = _min((uint16_t)_max(brightnessSensorDark, avg), brightnessSensorBright) - brightnessSensorDark;
            targetBrightness = constrained / ((float)_max(1, brightnessSensorBright - brightnessSensorDark)) * (float)(brightnessBright - brightnessDim) + brightnessDim;
            break;
        case BRIGHTNESS_STATIC:
            targetBrightness = brightnessBright;
            break;
        case BRIGHTNESS_TIME:
            targetBrightness = isTimeInRange(brightnessBrightHour * 60 + brightnessBrightMin, brightnessDimHour * 60 + brightnessDimMin, theHour * 60 + theMinute) ? brightnessBright : brightnessDim;
            break;
        case BRIGHTNESS_SUN:
#if USE_NTP
            targetBrightness = now() > sunRiseTime && now() < sunSetTime ? brightnessBright : brightnessDim;
#endif
            break;
        default:
            Serial.print(F("Invalid brightness mode: "));
            Serial.println(brightnessMode);
            break;
    }
    if (currentBrightness != targetBrightness && millis() - brightnessTime > BRIGHTNESS_UPDATE_INTERVAL) {
        currentBrightness += targetBrightness > currentBrightness ? 1 : -1;
        display.setBrightness(currentBrightness);
        brightnessTime = millis();
    }
}

void updateWeather() {
    if (!usingWeather || (weatherUpdateTime != 0 && millis() - weatherUpdateTime < WEATHER_UPDATE_INTERVAL))
        return;
    weatherUpdateTime = millis();

#if USE_WEATHER
    std::string data, cert;
    std::string address = "http://api.openweathermap.org/data/2.5/weather?id=";
    address.append(weatherLocation.c_str());
    address.append("&appid=");
    address.append(weatherKey.c_str());
    address.append("&units=");
    address.append(metric ? "metric" : "imperial");
    Serial.println(address.c_str());
    if (!sendGetRequest(address, "", WEATHER_TIMEOUT, true, cert, data))
        return;

    StaticJsonDocument<WEATHER_BUFFER_SIZE> doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
        Serial.print(F("Failed to deserialize response data: "));
        Serial.println(error.c_str());
    } else {
        if (doc["weather"][0].isNull())
            Serial.println("Weather response JSON was null");
        else 
            weatherID = doc["weather"][0]["id"];
        temperature = doc["main"]["temp"];
        temperaturePerceived = doc["main"]["feels_like"];
        temperatureMin = doc["main"]["temp_min"];
        temperatureMax = doc["main"]["temp_max"];
        pressure = doc["main"]["pressure"];
        humidity = doc["main"]["humidity"];
        visibility = doc["visibility"];
        windSpeed = doc["wind"]["speed"];
    }
#endif
}

void updateSunMoon() {
    if (!usingSunMoon || (sunMoonTime != 0 && millis() - sunMoonTime < SUN_UPDATE_INTERVAL))
        return;
    #if USE_SUNRISE && USE_NTP
    sunMoonTime = millis();
    SunMoonCalc sunMoonCalc = SunMoonCalc(now(), lattitude, longitude);
    SunMoonCalc::Sun sun = sunMoonCalc.calculateSunAndMoonData().sun;
    sunRiseTime = timezone.tzTime(sun.rise, UTC_TIME);
    sunSetTime = timezone.tzTime(sun.set, UTC_TIME);
    #endif
}

void updateTime() {
#if USE_NTP
    if (needsTimezone) {
        if (timezone.setLocation(location))
            DEBUG("Set timezone\n");
        else
            DEBUG("Failed to set timezone\n");
    }
        needsTimezone = false;
    if (!bootTime && timeStatus() == timeSet) 
        bootTime = timezone.tzTime(TIME_NOW, UTC_TIME);
    events(); // Handle NTP events
    theHour12 = hourFormat12();
    theHour = hour();
    theMinute = minute();
#endif
}

void setupArduinoOTA() {
#if ENABLE_ARDUINO_OTA
    Serial.println("Starting Development OTA Server");
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
      LittleFS.end();
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
#endif
}

void updateArduinoOTA() {
#if ENABLE_ARDUINO_OTA
    ArduinoOTA.handle();
#endif
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    display.begin(16);
#ifdef ESP8266
    display_ticker.attach(0.002, display_updater);
#endif
#ifdef ESP32
    xTaskCreatePinnedToCore(displayUpdateTask, "displayUpdateTask", 2048, NULL, 4, &displayUpdateTaskHandle, 1);
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 2000, true);
    timerAlarmEnable(timer);;
#endif
    display.setPanelsWidth(DISPLAY_PANELS);
    display.fillScreen(GREEN);
    display.showBuffer();
    display.fillScreen(GREEN);

    if (!LittleFS.begin() && !LittleFS.begin()) {
        display.fillScreen(BLUE);
        display.println(F("FS ERROR"));
        Serial.println(F("Failed to open filesystem"));
        display.showBuffer();
        delay(3000);
        display.fillScreen(BLUE);
        display.setCursor(0, 0);
        display.println(F("Formatting"));
        display.println(F("Display..."));
        Serial.println(F("Formatting Display"));
        display.showBuffer();
        delay(3000);
        display.fillScreen(0);
        display.showBuffer();
        LittleFS.format();
        LittleFS.begin();
        needsConfig = true;
    }
    File versionFile = LittleFS.open("/version.txt", "r");
    if (versionFile) {
        dashboardVersion = versionFile.parseInt();
       versionFile.close();
    }
    else
        Serial.println(F("Failed to read version file."));

    AsyncWiFiManager wifiManager(&server, &dnsServer);
    wifiManager.setAPCallback(onStartAccessPoint);

    if (drd.detectDoubleReset()) {
        Serial.println(F("Double reset detected, entering config mode"));
        wifiManager.startConfigPortal("LED Matrix Display");
    } else {
        Serial.println(F("Attemping to connect to network..."));
        display.fillScreen(BLUE);
        display.setCursor(0, 0);
        display.print(F("Connecting..."));
        display.showBuffer();
        wifiManager.autoConnect("LED Matrix Display", NULL, 5);
    }

    drd.stop();
    server.reset();
    Serial.print(F("Connected to network with IP: "));
    Serial.println(WiFi.localIP());

    // Development OTA
    setupArduinoOTA();

    // Webserver
    setupWebserver();

    // Development FS stuff
    //writeDefaultConfig();
    //LittleFS.remove("/config.json");

#if USE_NTP
    timezone.setDefault();
#endif

    if (needsConfig || !getConfig()) {
        display.fillScreen(BLUE);
        display.setCursor(0, 0);
        display.println(F("Configure Display at"));
        drawTinyText(display, WiFi.localIP().toString().c_str(), 0, 17, 0, 0, WHITE, true, 0, 0);
        display.showBuffer();
        needsConfig = true;
        while (needsConfig) {
            updateArduinoOTA();
            updateTime();
            yield();
        }
        Serial.println(F("Successfully configured display"));
    }

    display.setTextWrap(false);
    display.fillScreen(BLUE);
    display.setCursor(0, 0);
    display.println(F("Loading"));
    display.showBuffer();
}

void loop() {
    if (needsRestart)
        ESP.restart();

    switch (state) {
        default:
            Serial.printf("Unknown state: %i/n", state);
        case STATE_UPDATING:
            break;
        case STATE_NORMAL:
            updateTime();
            updateWeather();
            updateSunMoon();
            updateBrightness();
            updateScreen(needsUpdate);
            updateArduinoOTA();
            break;
    }
}
