#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESP_DoubleResetDetector.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <map>
#include <TinyFont.h>
#include <TinyIcons.h>
#include <JsonListener.h>
#include <OpenWeatherMapForecast.h>
#include <Timezone.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <SunMoonCalc.h>

#include <BLM.h>
#include <Taz.h>
#include <Mario.h>

#pragma region Constants

// Types
#define WIDGET_PROGMEM_IMAGE 0
#define WIDGET_FS_IMAGE 1
#define WIDGET_CLOCK 2
#define WIDGET_ANALOG_CLOCK 3
#define WIDGET_TEXT 4
#define WIDGET_TEXT_GET_JSON 5
#define WIDGET_TEXT_GET 6 //10
#define WIDGET_WEATHER_ICON 7
//#define WIDGET_TEXT_BIG 6
//#define WIDGET_CLOCK_BIG 8
//#define WIDGET_TEXT_GET_JSON_BIG 9
//#define WIDGET_TEXT_GET_BIG 11
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
#define STATE_NORMAL 0
#define STATE_UPDATING 1

// Configuration
#define MAX_CONFIG_FILE_SIZE 3000
#define CONFIG_JSON_BUFFER_SIZE 5000
#define JSON_REQUEST_BUFFER_SIZE 2000
#define MAX_IMAGE_DATA_FILE_SIZE 2000
#define IMAGE_JSON_BUFFER_SIZE 2500
#define WEATHER_UPDATE_INTERVAL 3600000 // Every Hour
#define NTP_UPDATE_INTERVAL 3700000 // Just over every hour to avoid large update delays
#define PIXEL_YIELD_THRESHOLD 100
#define SERVE_PROGMEM_IMAGES true
#define PROFILE_RENDERING false
#define BRIGHTNESS_UPDATE_INTERVAL 10
#define SUN_UPDATE_INTERVAL 7200000 // Every 2 hours, since sun won't rise within 2 hours of date change
#define BRIGHTNESS_SENSOR_PIN A0 // The pin to get photoresistor value from, if using
#define USE_BRIGHTNESS_SENSOR // Disables Vcc reading functionality to use brightness sensor
#define BRIGHTNESS_ROLLING_AVG_SIZE 10 // Higher value makes transition smoother, 4 bytes per buffer value, zero for no buffer
#define HTTPS_TRANSMIT_BUFFER 512
#define HTTPS_RECEIVE_BUFFER 1024
//#define ENABLE_ARDUINO_OTA
#define DEBUGGING

#ifndef VERSION_CODE
#define VERSION_CODE 0
#endif
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH 64
#endif
#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT 32
#endif
#ifdef ENABLE_ARDUINO_OTA
#include <ArduinoOTA.h>
#endif

#ifdef DEBUGGING
#define DEBUG(fmt, ...)  Serial.printf_P((PGM_P)PSTR( "DEBUG: " fmt), ## __VA_ARGS__)
//#define DEBUG(fmt, ...) Serial.printf("DEBUG: " fmt, ## __VA_ARGS__)
#else
#define DEBUG(...)
//#define DEBUG(...)
#endif

#pragma endregion

TimeChangeRule EDT = {"EDT", Second, Sun, Mar, 2, -240};    //Daylight time = UTC - 4 hours
TimeChangeRule EST = {"EST", First, Sun, Nov, 2, -300};     //Standard time = UTC - 5 hours
Timezone Eastern(EDT, EST);
TimeChangeRule CDT = {"CDT", Second, Sun, Mar, 2, -300};    //Daylight time = UTC - 5 hours
TimeChangeRule CST = {"CST", First, Sun, Nov, 2, -360};     //Standard time = UTC - 6 hours
Timezone Central(CDT, CST);
TimeChangeRule MDT = {"MDT", Second, Sun, Mar, 2, -360};    //Daylight time = UTC - 6 hours
TimeChangeRule MST = {"MST", First, Sun, Nov, 2, -420};     //Standard time = UTC - 7 hours
Timezone Mountain(MDT, MST);
TimeChangeRule PDT = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule PST = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 8 hours
Timezone Pacific(PDT, PST);

std::map<std::string, Timezone *> timezones = {{"eastern",  &Eastern},
                                               {"central",  &Central},
                                               {"mountain", &Mountain},
                                               {"pacific",  &Pacific}};

#pragma region PxMatrix
//#define PxMATRIX_COLOR_DEPTH 4
#define double_buffer
#define PxMATRIX_MAX_HEIGHT DISPLAY_HEIGHT
#define PxMATRIX_MAX_WIDTH DISPLAY_WIDTH

#include <PxMatrix.h>

#ifdef ESP32

#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 2
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#endif

#ifdef ESP8266

#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2

#endif

//PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);
//PxMATRIX display(32,16,P_LAT, P_OE,P_A,P_B,P_C);
PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D);
//PxMATRIX display(64,64,P_LAT, P_OE,P_A,P_B,P_C,P_D,P_E);

#ifdef ESP8266
// ISR for display refresh
void display_updater() {
  //display.displayTestPattern(70);
  display.display(70);
}
#endif

#ifdef ESP32
void IRAM_ATTR display_updater() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  //isplay.display(70);
  display.displayTestPattern(70);
  portEXIT_CRITICAL_ISR(&timerMux);
}
#endif
#pragma endregion

const uint16_t RED = display.color565(255, 0, 0);
const uint16_t GREEN = display.color565(0, 255, 0);
const uint16_t BLUE = display.color565(0, 0, 255);
const uint16_t WHITE = display.color565(255, 255, 255);
const uint16_t YELLOW = display.color565(255, 255, 0);
const uint16_t CYAN = display.color565(0, 255, 255);
const uint16_t MAGENTA = display.color565(255, 0, 255);
const uint16_t BLACK = display.color565(0, 0, 0);

class DisplayBuffer : public Adafruit_GFX {
    struct coords {
        int16_t x;
        int16_t y;
    };
    bool rotate;
    bool flip;
    std::unique_ptr <uint16_t> buffer;

public:
    DisplayBuffer(uint8_t width, uint8_t height) : Adafruit_GFX(width, height) {
        rotate = false;
        flip = false;
    }

    void setRotate(bool rotate) {
        this->rotate = rotate;
    }

    void setFlip(bool flip) {
        this->flip = flip;
    }

    coords transformCoords(int16_t x, int16_t y) {
        if (rotate) {
            int16_t temp_x = x;
            x = y;
            y = _height - 1 - temp_x;
        }
        // Todo: Verify transformations are correct
        if (flip)
            x = _width - 1 - x;

        if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) {
            DEBUG("DisplayBuffer: Transformed coordinates out of range: (%i, %i)\n", x, y);
            yield();
            return coords{0, 0};
        }
        return coords{x, y};
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        if (buffer.get() == nullptr) {
            DEBUG("DisplayBuffer: Buffer is not allocated");
            return;
        }
        //Serial.printf("Coordinates: (%i, %i)\n", x, y);
        coords c = transformCoords(x, y);
        if (c.y * _width + c.x >= _width * _height) {
            DEBUG("DisplayBuffer: index out of range");
            return;
        }
        buffer.get()[c.y * _width + c.x] = color;
    }

    void write(Adafruit_GFX &display, uint8_t xOff, uint8_t yOff, uint8_t width, uint8_t height) {
        if (buffer.get() == nullptr) {
            DEBUG("DisplayBuffer: Buffer is  not allocated");
            return;
        }

        for (uint8_t x = xOff; x < xOff + width; x++) {
            for (uint8_t y = yOff; y < yOff + height; y++) {
                if (y * _width + x >= _width * _height) {
                    DEBUG("DisplayBuffer: index out of range");
                    return;
                }
                display.drawPixel(x, y, buffer.get()[y * _width + x]);
            }
            if (width * height > PIXEL_YIELD_THRESHOLD)
                yield();
        }
    }

    bool isAllocated() {
        return buffer.get() != nullptr;
    }

    bool allocate() {
        DEBUG("Allocating display buffer...");
        buffer.reset(new uint16_t[_width * _height]);
        if (buffer.get() == nullptr) {
            Serial.println(F("Failed to allocate display buffer"));
            return false;
        }
        return true;
    }

    void close() {
        DEBUG("Freeing display buffer...");
        buffer.reset();
    }
};

struct Widget {
    uint16_t id; // The id for the widget
    uint8_t type; // The type of widget
    std::string content; // The filename for the image, for instance
    std::string source; // The URL for get requests, for instance
    uint8_t contentType; // Extra information about the content
    std::string auth; // Authorization string for requests
    std::vector <std::string> args; // Arguments for parsing of JSON response, for instance
    uint8_t xOff; // The X offset for the widget
    uint8_t yOff; // The Y offset for the widget
    uint8_t width; // The width of the widget
    uint8_t height; // The height of the widget
    uint16_t updateFrequency; // The number of milliseconds between widget updates
    uint8_t offset; // The offset for animations, for instance
    uint16_t length; // The length of animations, for instance, or request timeout
    std::vector <uint16_t> colors; // The colors for text, for instance
    bool large; // Whether text components use the large font
    bool bordered; // Whether the widget has a border or not
    uint16_t borderColor; // The color of a border
    bool transparent; // Whether to fill region with backgroundColor before drawing widget or not
    uint16_t backgroundColor; // The background color if used
    bool background; // Whether the widget exists in the background or not
    time_t lastUpdate; // When the widget was last updated
    uint8_t state; // The animation frame, for instance
    bool dirty; // Whether a refresh is queued
    File file; // Open file for images to prevent file operation display flashing
};

struct FSImage {
    uint8_t width;
    uint8_t height;
    uint8_t length;
    uint8_t type;
};

struct ProgmemImage {
    uint8_t width;
    uint8_t height;
    uint8_t length;
    uint8_t type;
    void *data;
};
std::map <std::string, ProgmemImage> progmemImages = {{"taz",  {23, 28, 1,  IMAGE_UINT16, taz}},
                                                      {"blm",  {64, 32, 36, IMAGE_UINT8,  blmAnimations}},
                                                      {"mario",  {64, 32, 1, IMAGE_UINT16,  mario}}};

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
std::string localTimezone;
bool usingWeather;
bool usingTransparency;
bool metric;
bool fastUpdate;
String weatherKey;
String weatherLocation;
bool usingSunMoon;
double longitude;
double lattitude;

// State values
uint8_t state;
time_t weatherUpdateTime;
uint16_t weatherID = 200;
bool needsConfig; // If a configuration is needed
bool needsUpdate; // If a full update is needed
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

#if BRIGHTNESS_ROLLING_AVG_SIZE > 0
#include <RunningAverage.h>
RunningAverage brightnessAverage(BRIGHTNESS_ROLLING_AVG_SIZE);
#endif

DoubleResetDetector drd(10, 0);
ESP8266WebServer *server;
ESP8266HTTPUpdateServer updateServer(true);
NTPClient *ntpClient;
OpenWeatherMapForecast weatherClient;
DisplayBuffer displayBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT);

#ifndef USE_BRIGHTNESS_SENSOR
ADC_MODE(ADC_VCC);
#endif

#pragma region Configuration

void writeDefaultConfig() {
    LittleFS.remove("/config.json");
    File file = LittleFS.open("/config.json", "w");
    file.print(F("{\"widgets\":[{\"xOff\":17,\"yOff\":11,\"type\":6,\"content\":\"Hello\",\"length\":1,\"borderColor\":\"0xFF0000\",\"backgroundColor\":\"0x00FFFF\",\"colors\":[\"0x00FF00\"],\"width\":30,\"height\":8}],\"brightnessMode\":0,\"brightnessLower\":1,\"brightnessUpper\":100,\"backgroundColor\":\"0x00FFFF\",\"timezone\":\"eastern\",\"metric\":false,\"weatherKey\":\"\",\"weatherLocation\":\"5014227\"}"));
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

bool parseConfig(char data[], String& errorString) {
    Serial.print(F("Attempting to parse config: "));
    Serial.println(data);
    
    DynamicJsonDocument doc(CONFIG_JSON_BUFFER_SIZE);
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
        errorString.concat(F("Failed to deserialize config file: "));
        errorString.concat(error.c_str());
        return false;
    }

    uint16_t tempBackgroundColor = doc["backgroundColor"].isNull() ? 0 : parseHexColorString(
            doc["backgroundColor"].as<char *>());
    String tempWeatherKey = doc["weatherKey"].isNull() ? "" : String(doc["weatherKey"].as<char *>());
    String tempWeatherLocation = doc["weatherLocation"].isNull() ? "" : String(
            doc["weatherLocation"].as<char *>());
    bool tempMetric = doc["metric"];
    bool tempTransparency = false;
    bool tempFastUpdate = doc["fastUpdate"];
    std::string tempTimezone = doc["timezone"].isNull() ? "eastern" : std::string(doc["timezone"].as<char *>());
    if (timezones.count(tempTimezone) == 0) {
        errorString.concat(F("Invalid timezone"));
        return false;
    }
    uint8_t tempBrightnessMode = doc["brightnessMode"];
    bool tempUsingSunMoon = tempBrightnessMode == BRIGHTNESS_SUN;
    uint8_t tempBrightnessDim = doc["brightnessLower"];
    uint8_t tempBrightnessBright = doc["brightnessUpper"].isNull() ? 255 : doc["brightnessUpper"];
    uint16_t tempBrightnessSensorBright = doc["sensorBright"];
    uint16_t tempBrightnessSensorDark = doc["sensorDark"];
    uint8_t tempBrightnessBrightMin;
    uint8_t tempBrightnessBrightHour;
    uint8_t tempBrightnessDimMin;
    uint8_t tempBrightnessDimHour;
    String brightTime = doc["brightTime"].isNull() ? "09:30" : String(doc["brightTime"].as<char*>());
    tempBrightnessBrightHour = brightTime.substring(0, 2).toInt();
    tempBrightnessBrightMin = brightTime.substring(3, 5).toInt();
    String darkTime = doc["darkTime"].isNull() ? "20:30" : String(doc["darkTime"].as<char*>());
    tempBrightnessDimHour = darkTime.substring(0, 2).toInt();
    tempBrightnessDimMin = darkTime.substring(3, 5).toInt();
    double tempLongitude = doc["longitude"];
    double tempLattitude = doc["lattitude"];

    bool tempUsingWeather = false;
    std::vector <Widget> tempWidgets;
    JsonArray configWidgets = doc["widgets"];
    if (configWidgets) {
        for (ushort i = 0; i < configWidgets.size(); i++) {
            JsonObject widget = configWidgets[i];
            if (!widget || widget["width"].isNull() || widget["height"].isNull()) {
                errorString.concat("Failed to parse widget: ");
                errorString.concat(i);
                return false;
            }
            if (widget[F("disabled")])
                continue;
            if (widget[F("type")] == WIDGET_WEATHER_ICON)
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
                //case WIDGET_TEXT_BIG:
                case WIDGET_TEXT:
                case WIDGET_TEXT_GET_JSON:
                //case WIDGET_TEXT_GET_JSON_BIG:
                case WIDGET_TEXT_GET:
                //case WIDGET_TEXT_GET_BIG:
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
                                   widget["auth"].isNull() ? "" : std::string(widget["auth"].as<char *>()), args,
                                   widget["xOff"], widget["yOff"], widget["width"], widget["height"],
                                   widget["frequency"], widget["offset"], widget["length"], colors,
                                   widget["large"],
                                   widget["bordered"], widget["borderColor"].isNull() ? 0 : parseHexColorString(
                            widget["borderColor"].as<char *>()),
                                   widget["transparent"],
                                   widget["backgroundColor"].isNull() ? 0 : parseHexColorString(
                                           widget["backgroundColor"].as<char *>()), widget["background"]});
        }
        std::sort(tempWidgets.begin(), tempWidgets.end(), [](Widget w1, Widget w2) { return w1.id < w2.id; });
    }

    // Only load new configuration after successful completion of config parsing
    for (uint i = 0; i < widgets.size(); i++)
        widgets[i].file.close();

    needsUpdate = true;
    needsConfig = false;
    widgets = tempWidgets;
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
    localTimezone = tempTimezone;
    usingWeather = tempUsingWeather;
    weatherLocation = tempWeatherLocation;
    weatherKey = tempWeatherKey;
    metric = tempMetric;
    fastUpdate = tempFastUpdate;
    usingTransparency = tempTransparency;
    lattitude = tempLattitude;
    longitude = tempLongitude;

    display.setFastUpdate(fastUpdate);

    weatherUpdateTime = 0;
    sunMoonTime = 0;

    // Transparency uses about an extra 4KB
    if (displayBuffer.isAllocated()) {
        if (usingTransparency)
            displayBuffer.fillScreen(0);
        else
            displayBuffer.close();
    }

    Serial.println(F("Successfully loaded configuration!"));
    Serial.printf("Memory Free: %i, Fragmentation: %i%%\n", ESP.getFreeHeap(), ESP.getHeapFragmentation());
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

    std::unique_ptr<char[]> buf(new char[size]);

    configFile.readBytes(buf.get(), size);
    configFile.close();

    String error;
    if (!parseConfig(buf.get(), error)) {
        Serial.printf("Failed to parse config: %s\n, replacing with default...\n", error.c_str());
        writeDefaultConfig();
        return false;
    }

    return true;
}

#pragma endregion

void queueFullUpdate() {
    needsUpdate = true;
}

#pragma region Web Server

void onStartAccessPoint(WiFiManager *WiFiManager) {
    Serial.println(F("Entering AP config mode"));
    display.setCursor(0, 0);
    display.println(F("Wifi"));
    display.println(F("Config"));
    display.println(F("Mode"));
    display.showBuffer();
    needsConfig = true;
}

String getContentType(String filename) {
    if (filename.endsWith(F(".html"))) return F("text/html");
    else if (filename.endsWith(F(".css"))) return F("text/css");
    else if (filename.endsWith(F(".js"))) return F("application/javascript");
    else if (filename.endsWith(F(".ico"))) return F("image/x-icon");
    else if (filename.endsWith(F(".json"))) return F("text/json");
    return F("text/plain");
}

bool sendFile(String path) {
    if (path.endsWith(F("/")))
        return false;
    String existpath = path + (LittleFS.exists(path + ".gz") ? ".gz" : "");
    Serial.printf("Serving: %s, %i\n", existpath.c_str(), LittleFS.exists(path));
    String contentType = getContentType(path);
    if (LittleFS.exists(existpath)) {
        File file = LittleFS.open(existpath, "r");
        server->streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

void serveRoot() {
    sendFile(F("/index.html"));
}

void serveConfig() {
    if (server->method() == HTTP_POST) {
        std::unique_ptr<char[]> buf(new char[MAX_CONFIG_FILE_SIZE]);

        String jsonString = server->arg(F("plain"));
        if (jsonString.length() > MAX_CONFIG_FILE_SIZE) {
            server->send(400, "text/html", F("Config file is too large"));
            return;
        }
        jsonString.toCharArray(buf.get(), MAX_CONFIG_FILE_SIZE);
        String error;
        if (parseConfig(buf.get(), error)) {
            LittleFS.remove("/config.json");
            File file = LittleFS.open("/config.json", "w");
            file.write(jsonString.c_str());
            file.close();
            Serial.println(F("Successfully updated config file"));
            server->send(200);
        } else {
            Serial.printf("Failed to update config file: %s\n", error.c_str());
            server->send(400, "text/plain", error);
        }
    } else {
        if (!sendFile(F("/config.json"))) {
            writeDefaultConfig();
            sendFile(F("/config.json"));
        }
    }
}

void serveFullUpdate() {
    queueFullUpdate();
    server->send(200);
}

void serveNotFound() {
    if (!sendFile(server->uri()))
        server->send(404, "text/html", F("Not found"));
}

void serveRestart() {
    server->send(200);
    delay(2000);
    ESP.restart();
}

void startOTA() {
    state = STATE_UPDATING;
    LittleFS.end();
    server->send(200, "text/plain", F(""));
}

void serveStats() {
    StaticJsonDocument<400> doc;
    doc["uptime"] = ntpClient->getRawTime() - bootTime;
    char reason[25];
    doc["resetReason"] = strcpy(reason, ESP.getResetReason().c_str());
    doc["version"] = VERSION_CODE;
    doc["fragmentation"] = ESP.getHeapFragmentation();
    doc["memoryFree"] = ESP.getFreeHeap();
    doc["transparencyBuffer"] = displayBuffer.isAllocated();
    FSInfo info;
    LittleFS.info(info);
    doc["filesystemUsed"] = info.usedBytes;
    doc["filesystemTotal"] = info.totalBytes;
    doc["maxOpenFiles"] = info.maxOpenFiles;
    doc["maxPathLength"] = info.maxPathLength;
    doc["width"] = DISPLAY_WIDTH;
    doc["height"] = DISPLAY_HEIGHT;
    doc["vcc"] = ESP.getVcc();
    doc["brightness"] = currentBrightness;
    doc["brightnessSensor"] = analogRead(BRIGHTNESS_SENSOR_PIN);
    std::unique_ptr<char[]> buf(new char[300]);
    serializeJson(doc, buf.get(), 300);
    server->send(200, "text/plain", buf.get());
}

/*uint8_t getDirectoryFileCount(Dir& dir) {
    uint8_t count;
    while(dir.next()) 
        count++;
    return count;
}*/

void writeDefaultImageData() {
    File dataFile = LittleFS.open(F("/imageData.json"), "w");
    dataFile.write("{}");
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
        data[entry.key().c_str()] = {entry.value()["width"], entry.value()["height"], entry.value()["length"],
                                     entry.value()["type"]};

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
        o["type"] = entry.second.type;
    }
    std::unique_ptr<char[]> buf(new char[MAX_IMAGE_DATA_FILE_SIZE]);
    serializeJson(doc, buf.get(), MAX_IMAGE_DATA_FILE_SIZE);
    dataFile.write(buf.get());
    dataFile.close();
}

Dir getImageDir() {
    if (!LittleFS.exists("/images"))
        LittleFS.mkdir("/images");
    return LittleFS.openDir("/images");
}

void serveImageData() {
    Dir imageDir = getImageDir();
    std::map <std::string, FSImage> fsImageData = getFSImageData();

    size_t size = 100 + (progmemImages.size() + fsImageData.size()) * 100;
    DynamicJsonDocument doc(size);

    if (SERVE_PROGMEM_IMAGES)
        for (auto const &entry : progmemImages) {
            //if (entry.second.type != IMAGE_GIMP)
            //    continue;
            JsonObject o = doc.createNestedObject(entry.first.c_str());
            o["width"] = entry.second.width;
            o["height"] = entry.second.height;
            o["length"] = entry.second.length;
            o["type"] = entry.second.type;
            o["progmem"] = true;
            //doc[String(entry.first.c_str()) + "_P"] = o;
            //doc.add(String(entry.first.c_str()) + "_P");
        }

    for (auto const &entry : fsImageData) {
        JsonObject o = doc.createNestedObject(entry.first.c_str());
        o["width"] = entry.second.width;
        o["height"] = entry.second.height;
        o["length"] = entry.second.length;
        o["type"] = entry.second.type;
        o["progmem"] = false;
    }

    std::unique_ptr<char[]> buf(new char[size]);
    serializeJson(doc, buf.get(), size);
    server->send(200, F("text/json"), buf.get());
}

void uploadImage() {
    for (uint i = 0; i < widgets.size(); i++)
        widgets[i].file.close();
    static File fsUploadFile;
    HTTPUpload &upload = server->upload();
    if (upload.status == UPLOAD_FILE_START) {
        String filename = "/images/" + server->arg("name");
        Serial.printf("Uploading image: %s\n", filename.c_str());
        fsUploadFile = LittleFS.open(filename, "w");
        if (!fsUploadFile) {
            Serial.println(F("Failed to open image upload file"));
            server->send(500);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (fsUploadFile)
            fsUploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile) {
            fsUploadFile.close();
            std::map <std::string, FSImage> fsImageData = getFSImageData();
            fsImageData[server->arg("name").c_str()] = {strtoul(server->arg("width").c_str(), NULL, 10),
                                                        strtoul(server->arg("height").c_str(), NULL, 10),
                                                        strtoul(server->arg("length").c_str(), NULL, 10),
                                                        strtoul(server->arg("type").c_str(), NULL, 10)};
            writeFSImageData(fsImageData);
            Serial.print("Image Size: ");
            Serial.println(upload.totalSize);
            //server->send(200);
        } else {
            String filename = "/images/" + server->arg("name");
            if (LittleFS.exists(filename))
                LittleFS.remove(filename);
            //server->send(500, "text/plain", "500: couldn't save image");
        }
    }
}

void serveImage() {
    String image = server->arg(F("image"));
    if (image.endsWith(F("_P"))) {
        std::string progmem = std::string(image.substring(0, image.length() - 2).c_str());
        if (progmemImages.count(progmem) < 1) {
            server->send(404);
            return;
        }
        uint size = (progmemImages[progmem].type == IMAGE_GIMP ? 4 : 2) * progmemImages[progmem].width *
                    progmemImages[progmem].height * progmemImages[progmem].length;
        server->sendHeader(F("Content-Description"), F("File Transfer"));
        server->sendHeader(F("Content-Transfer-Encoding"), F("binary"));
        server->send_P(200, "application/binary", (PGM_P) progmemImages[progmem].data, size);
    } else {
        if (!sendFile("/images/" + image)) {
            Serial.printf("Image doesn't exist %s\n", image.c_str());
            server->send(404);
        }
    }
}

void serveOK() {
    server->send(200);
}

void serveDeleteImage() {
    for (uint i = 0; i < widgets.size(); i++)
        widgets[i].file.close();
    std::string name = server->arg(F("name")).c_str();
    std::map <std::string, FSImage> fsImageData = getFSImageData();
    fsImageData.erase(name);
    std::string filename = "/images/" + name;
    if (LittleFS.exists(filename.c_str()))
        LittleFS.remove(filename.c_str());
    writeFSImageData(fsImageData);
    server->send(200);
}

void deleteAllImages() {
    for (uint i = 0; i < widgets.size(); i++)
        widgets[i].file.close();
    writeDefaultImageData();
    Dir dir = getImageDir();
    LittleFS.rmdir(F("/images"));
    LittleFS.mkdir(F("/images"));
    std::vector <String> files;
    while (dir.next()) {
        Serial.println(dir.fileName().c_str());
        files.push_back(dir.fileName());
    }
    for (const auto &file: files)
        LittleFS.remove("/images/" + file);
}

void serveDeleteAllImages() {
    deleteAllImages();
    server->send(200);
}

void serveResetConfiguration() {
    writeDefaultConfig();
    server->send(200);
}

void serveFactoryReset() {
    writeDefaultConfig();
    deleteAllImages();
    server->send(200);
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    delay(2000);
    ESP.restart();
}

void setupWebserver() {
    server = new ESP8266WebServer(80);
    server->on(F("/"), serveRoot);
    /*server->on("/update", HTTP_OPTIONS, []() {
        server->sendHeader("Access-Control-Max-Age", "10000");
        server->sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
        server->sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        server->send(200, "text/plain", "");
    });*/
    updateServer.setup(server);

    server->on(F("/config"), serveConfig);
    server->on(F("/beginUpdate"), startOTA);
    server->on(F("/fullRefresh"), serveFullUpdate);
    server->on(F("/restart"), serveRestart);
    server->on(F("/stats"), serveStats);
    server->on(F("/uploadImage"), HTTP_POST, serveOK, uploadImage);
    server->on(F("/image"), HTTP_GET, serveImage);
    server->on(F("/images"), serveImageData);
    server->on(F("/deleteImage"), serveDeleteImage);
    server->on(F("/deleteAllImages"), serveDeleteAllImages);
    server->on(F("/factoryReset"), serveFactoryReset);
    server->on(F("/resetConfiguration"), serveResetConfiguration);
    server->onNotFound(serveNotFound);
    Serial.println(F("Starting configuration webserver..."));
    server->begin();
}

#pragma endregion

#pragma region Rendering

void drawImageUint8(Adafruit_GFX &d, uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height, uint8_t offset,
                    uint8_t *data, bool progMem) {
    uint16_t line_buffer[width];
    for (uint8_t y = 0; y < height; y++) {
        if (progMem)
            memcpy_P(line_buffer, data + offset * width * height * 2 + y * width * 2, width * 2);
        else
            memcpy(line_buffer, data + offset * width * height * 2 + y * width * 2, width * 2);
        for (uint8_t x = 0; x < width; x++)
            d.drawPixel(xOffset + x, yOffset + y, line_buffer[x]);
        if (width * height > PIXEL_YIELD_THRESHOLD)
            yield();
    }
}

void drawImageUint16(Adafruit_GFX &d, uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height, uint8_t offset,
                     uint16_t *data, bool progMem) {
    uint16_t line_buffer[width];
    for (uint8_t y = 0; y < height; y++) {
        if (progMem)
            memcpy_P(line_buffer, data + offset * width * height + y * width, width * 2);
        else
            memcpy(line_buffer, data + offset * width * height * 2 + y * width * 2, width * 2);
        for (uint8_t x = 0; x < width; x++)
            d.drawPixel(xOffset + x, yOffset + y, line_buffer[x]);
        if (width * height > PIXEL_YIELD_THRESHOLD)
            yield();
    }
}

void drawImageFsUint8(Adafruit_GFX &d, uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height, uint8_t offset,
                      const char name[], File &file) {
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
        for (uint8_t x = 0; x < width; x++)
            d.drawPixel(xOffset + x, yOffset + y, line_buffer[x * 2] | line_buffer[x * 2 + 1] << 8);
        if (width * height > PIXEL_YIELD_THRESHOLD)
            yield();
    }
}

void
drawGimpImage(Adafruit_GFX &display, uint8_t xOff, uint8_t yOff, uint8_t width, uint8_t height, char *data) {
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
            display.drawPixel(x + xOff, y + yOff, ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
        }
        if (width * height > PIXEL_YIELD_THRESHOLD)
            yield();
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

const char *getTimeText(uint8_t type) {
    std::string text;
    uint8_t hourValue = type == CLOCK_TYPE_24 ? theHour : theHour12;
    if (hourValue < 10)
        text += "0";
    text.append(dtostrf(hourValue, hourValue < 10 ? 1 : 2, 0, " "));
    text.append(":");
    if (theMinute < 10)
        text.append("0");
    text += dtostrf(theMinute, theMinute < 10 ? 1 : 2, 0, " ");
    if (type == CLOCK_TYPE_12_PERIOD)
        text += theHour < 12 ? "AM" : "PM";
    return text.c_str();
}

void drawBigText(Adafruit_GFX &d, const char *text, uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height,
                 uint16_t color, uint8_t wrapping) {
    d.setTextColor(color);
    d.setCursor(xOffset, yOffset);
    d.print(text);
}

void drawTinyText(Adafruit_GFX &display, const char text[], uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                  uint16_t color, bool transparent, uint16_t backgroundColor, uint8_t wrapping) {
    TFDrawText(display, text, x, y, color, transparent, backgroundColor);
}

bool sendGetRequest(std::string url, std::string auth, uint16_t timeout, bool json, std::string &result) {
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
    clientPtr.reset((ssl ? new WiFiClientSecure() : new WiFiClient()));
    WiFiClient* client = clientPtr.get();
    if (ssl) {
        WiFiClientSecure* secureClient = (WiFiClientSecure*)client;
        secureClient->setInsecure();
        // Impossible to send requests without limiting buffer sizes, though this may cause issues receiving responses if server doesn't support MFLN
        secureClient->setBufferSizes(HTTPS_RECEIVE_BUFFER, HTTPS_TRANSMIT_BUFFER);
    }
    client->flush();
    client->setTimeout(timeout);
    DEBUG("Connecting to: %s\n", host.c_str());
	if (!client->connect(host.c_str(), port)){
        Serial.println(F("Failed to connect to GET address"));
        return false;
    }

    yield();

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

void updateTextGETWidgetJson(Widget &widget) {
    std::string data;
    if (!sendGetRequest(widget.source, widget.auth, widget.length, true, data))
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
        if (!widget.large)
            s.toUpperCase();
        widget.content = std::string(s.c_str());
        widget.dirty = true;
    }
}

void updateTextGETWidget(Widget &widget) {
    std::string data;
    if (!sendGetRequest(widget.source, widget.auth, widget.length, false, data))
        return;
    String s = String(data.c_str());
    if (!widget.large)
        s.toUpperCase();
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
    if (widget.state != theMinute) {
        widget.state = theMinute;
        widget.dirty = true;
        if (widget.type != WIDGET_ANALOG_CLOCK) 
            widget.content = getTimeText(widget.contentType);
    }
}

void updateWidget(Widget &widget) {
    switch (widget.type) {
        case WIDGET_PROGMEM_IMAGE:
        case WIDGET_FS_IMAGE:
            incrementWidgetState(widget);
            break;
        //case WIDGET_TEXT_GET_JSON_BIG:
        case WIDGET_TEXT_GET_JSON:
            updateTextGETWidgetJson(widget);
            break;
        case WIDGET_TEXT_GET:
        //case WIDGET_TEXT_GET_BIG:
            updateTextGETWidget(widget);
            break;
        case WIDGET_WEATHER_ICON:
            incrementWidgetState(widget, 5);
            break;
        case WIDGET_ANALOG_CLOCK:
        case WIDGET_CLOCK:
        //case WIDGET_CLOCK_BIG:
            updateClockWidget(widget);
            break;
        //case WIDGET_TEXT_BIG:
        case WIDGET_TEXT:
            break;
        default:
            Serial.print(F("Invalid widget type to update: "));
            Serial.println(widget.type);
            break;
    }

    if (widget.background)
        queueFullUpdate();
    widget.lastUpdate = millis();
}

void updateWidgets() {
    for (uint i = 0; i < widgets.size(); i++)
        if (widgets[i].updateFrequency > 0 && (millis() - widgets[i].lastUpdate > widgets[i].updateFrequency || widgets[i].lastUpdate == 0))
            updateWidget(widgets[i]);
}

void drawWidget(Adafruit_GFX &d, Widget &widget, bool buffering) {
    if (!usingTransparency || !widget.transparent) {
        if (widget.type == WIDGET_ANALOG_CLOCK)
            d.fillCircle(widget.xOff + widget.height / 2, widget.yOff + widget.height / 2, widget.height / 2,
                         widget.backgroundColor);
        else
            d.fillRect(widget.xOff, widget.yOff, widget.width, widget.height, widget.backgroundColor);
    } else if (!buffering) 
        displayBuffer.write(display, widget.xOff, widget.yOff, widget.width, widget.height);
    

    // Todo: check if background widgets change to queue a full update on the next frame
    switch (widget.type) {
        case WIDGET_PROGMEM_IMAGE:
            switch (progmemImages[widget.content].type) {
                case IMAGE_UINT8:
                    drawImageUint8(d, widget.xOff + (widget.width - progmemImages[widget.content].width) / 2,
                                   widget.yOff + (widget.height - progmemImages[widget.content].height) / 2,
                                   progmemImages[widget.content].width, progmemImages[widget.content].height,
                                   widget.state + widget.offset, (uint8_t *) progmemImages[widget.content].data, true);
                    break;
                case IMAGE_UINT16:
                    drawImageUint16(d, widget.xOff + (widget.width - progmemImages[widget.content].width) / 2,
                                    widget.yOff + (widget.height - progmemImages[widget.content].height) / 2,
                                    progmemImages[widget.content].width, progmemImages[widget.content].height,
                                    widget.state + widget.offset, (uint16_t *) progmemImages[widget.content].data,
                                    true);
                    break;
                case IMAGE_GIMP:
                    drawGimpImage(d, widget.xOff + (widget.width - progmemImages[widget.content].width) / 2,
                                  widget.yOff + (widget.height - progmemImages[widget.content].height) / 2,
                                  progmemImages[widget.content].width, progmemImages[widget.content].height,
                                  (char *) progmemImages[widget.content].data);
                    break;
                default:
                    Serial.println(F("Unknown image type"));
                    break;
            }
            break;
        case WIDGET_FS_IMAGE:
            drawImageFsUint8(d, widget.xOff, widget.yOff, widget.width, widget.height, widget.state + widget.offset,
                             ("/images/" + widget.content).c_str(), widget.file);
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
            if (widget.large)
                drawBigText(d, widget.content.c_str(),
                        widget.xOff + _max(0, (widget.width - 6 * (int) widget.content.length() + 1) / 2),
                        widget.yOff + (widget.height - 7) / 2, widget.width, widget.height, widget.colors[0],
                        widget.contentType);
            else
                drawTinyText(d, widget.content.c_str(),
                         widget.xOff + _max(0, (widget.width - (TF_COLS + 1) * (int) widget.content.length() + 1) / 2),
                         widget.yOff + (widget.height - TF_ROWS) / 2, widget.width, widget.height, widget.colors[0],
                         true, 0, widget.contentType);
            break;
        /*case WIDGET_TEXT_GET_JSON_BIG:
        case WIDGET_TEXT_GET_BIG:
        case WIDGET_TEXT_BIG:
        case WIDGET_CLOCK_BIG:
            drawBigText(d, widget.content.c_str(),
                        widget.xOff + _max(0, (widget.width - 6 * (int) widget.content.length() + 1) / 2),
                        widget.yOff + (widget.height - 7) / 2, widget.width, widget.height, widget.colors[0],
                        widget.contentType);
            break;*/
        case WIDGET_WEATHER_ICON:
            TIDrawIcon(d, weatherID, widget.xOff + _max(0, (widget.width - 10 + 1) / 2), widget.yOff + (widget.height - 5) / 2, widget.state, true, widget.transparent, widget.backgroundColor);
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
        else
            drawScreen(displayBuffer, fullUpdate, true, false);
    }
    drawScreen(display, fullUpdate, false, false);
    display.showBuffer();
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
#if BRIGHTNESS_ROLLING_AVG_SIZE > 0
            brightnessAverage.addValue(analogRead(BRIGHTNESS_SENSOR_PIN));
            avg = brightnessAverage.getAverage();
#else
            avg = analogRead(BRIGHTNESS_SENSOR_PIN);
#endif
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
            targetBrightness = now() > sunRiseTime && now() < sunSetTime ? brightnessBright : brightnessDim;
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
    if (!usingWeather || weatherUpdateTime == 0 || millis() - weatherUpdateTime < WEATHER_UPDATE_INTERVAL)
        return;
    weatherUpdateTime = millis();

    OpenWeatherMapForecastData data[1];
    weatherClient.setMetric(metric);
    weatherClient.setLanguage(F("en"));
    uint8_t allowedHours[] = {0, 12};
    weatherClient.setAllowedHours(allowedHours, 2);
    // See https://docs.thingpulse.com/how-tos/openweathermap-key/

    if (!weatherClient.updateForecastsById(data, weatherKey, weatherLocation, 1)) {
        Serial.println(F("Failed to update weather info"));
        return;
    }

    weatherID = data[0].weatherId;
}

void updateSunMoon() {
    if (!usingSunMoon || (sunMoonTime != 0 && millis() - sunMoonTime < SUN_UPDATE_INTERVAL))
        return;
    sunMoonTime = millis();
    SunMoonCalc sunMoonCalc = SunMoonCalc(now(), lattitude, longitude);
    SunMoonCalc::Sun sun = sunMoonCalc.calculateSunAndMoonData().sun;
    sunRiseTime = timezones[localTimezone]->toLocal(sun.rise);
    sunSetTime = timezones[localTimezone]->toLocal(sun.set);
}

void updateTime() {
    ntpClient->update();
    theHour12 = hourFormat12();
    theHour = hour();
    theMinute = minute();
}

void setupArduinoOTA() {
#ifdef ENABLE_ARDUINO_OTA
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
#ifdef ENABLE_ARDUINO_OTA
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
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 2000, true);
    timerAlarmEnable(timer);
#endif
    //display.setBrightness(1);
    //display.setFastUpdate(true);
    display.setTextWrap(false);
    display.fillScreen(GREEN);
    display.showBuffer();
    display.fillScreen(GREEN);

    LittleFS.begin();

    WiFiManager wifiManager;
    wifiManager.setAPCallback(onStartAccessPoint);

    if (drd.detectDoubleReset()) {
        Serial.println(F("Double reset detected, entering config mode"));
        wifiManager.startConfigPortal("LED Matrix Display");
        //needsConfig = true;
    } else {
        Serial.println(F("Attemping to connect to network..."));
        display.print(F("Connecting..."));
        display.showBuffer();
        wifiManager.autoConnect("LED Matrix Display");
    }

    drd.stop();
    Serial.print(F("Connected to network with IP: "));
    Serial.println(WiFi.localIP());

    std::string result;
    //sendGetRequest2("http://httpbin.org/get", "", 10000, result);
    //sendGetRequest2("https://www.googleapis.com/youtube/v3/channels?part=statistics&id=UCFKDEp9si4RmHFWJW1vYsMA&key=AIzaSyDKUk5KPDq7vTwrmbrFDcyJSNjYp52CJvk", "", 10000, result);
    Serial.println(result.c_str());

    // Development OTA
    setupArduinoOTA();

    // Webserver
    setupWebserver();

    // Development FS stuff
    //writeDefaultConfig();
    //LittleFS.remove("/config.json");

    ntpClient = new NTPClient("time.nist.gov", 0, NTP_UPDATE_INTERVAL);
    ntpClient->begin();
    ntpClient->forceUpdate();
    bootTime = ntpClient->getRawTime();

    if (needsConfig || !getConfig()) {
        display.fillScreen(BLUE);
        display.setCursor(0, 0);
        display.println(F("Configure Display at"));
        drawTinyText(display, WiFi.localIP().toString().c_str(), 0, 17, 0, 0, WHITE, true, 0, 0);
        display.showBuffer();
        needsConfig = true;
        while (needsConfig) {
            server->handleClient();
            updateArduinoOTA();
            yield();
        }
        Serial.println(F("Successfully configured display"));
    }

    setSyncProvider([]() { return timezones[localTimezone]->toLocal(ntpClient->getRawTime()); });
    setSyncInterval(10);
}

void loop() {
    updateTime();
    updateWeather();
    updateSunMoon();
    updateBrightness();

    switch (state) {
        default:
            Serial.printf("Unknown state: %i/n", state);
        case STATE_NORMAL:
            updateScreen(needsUpdate);
            break;
        case STATE_UPDATING:
            display.fillScreen(BLUE);
            display.setCursor(0, 0);
            display.print(F("Updating, \nDo not \npower off"));
            display.showBuffer();
            break;
    }

    server->handleClient();
    updateArduinoOTA();
}