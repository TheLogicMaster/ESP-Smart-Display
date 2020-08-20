#include <Arduino.h>
#include <FS.h>
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
#include "OpenWeatherMapForecast.h"

#include <blm.h>

#pragma region Constants

#define WIDGET_PROGMEM_IMAGE 0
#define WIDGET_FS_IMAGE 1
#define WIDGET_CLOCK 2
#define WIDGET_ANALOG_CLOCK 3
#define WIDGET_TEXT 4
#define WIDGET_TEXT_REST 5
#define WIDGET_TEXT_BIG 6
#define WIDGET_WEATHER_ICON 7
#define WIDGET_CLOCK_BIG 8
#define BRIGHTNESS_AUTO 0
#define BRIGHTNESS_STATIC 1
#define BRIGHTNESS_TIME 2
#define BRIGHTNESS_SUN 3
#define MAX_CONFIG_FILE_SIZE 1536
#define JSON_BUFFER_SIZE 2048
#define WEATHER_UPDATE_INTERVAL 3600000
#define NTP_UPDATE_INTERVAL 3700000
#define PIXEL_YIELD_THRESHOLD 100
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#pragma endregion

#pragma region PxMatrix
//#define PxMATRIX_COLOR_DEPTH 4
#define double_buffer
#define PxMATRIX_MAX_HEIGHT DISPLAY_HEIGHT
#define PxMATRIX_MAX_WIDTH DISPLAY_WIDTH
#include <PxMatrix.h>
//#include <CustomPxMatrix.h>

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
PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);
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

struct coords {
  int16_t x;
  int16_t y;
};

class DisplayBuffer : public Adafruit_GFX {

  bool rotate;
  bool flip;
  uint16_t* buffer;

  public:
  DisplayBuffer(uint8_t width, uint8_t height) : Adafruit_GFX(width, height) {
    buffer = new uint16_t[width * height];
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
  if (rotate){
    int16_t temp_x=x;
    x=y;
    y=_height-1-temp_x;
  }
  // Todo: Verify transformations are correct
  if (flip)
    x =_width - 1 -x;
  
   if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) {
      Serial.printf("DisplayBuffer: Transformed coordinates out of range: (%i, %i)\n", x, y);
      yield();
      return coords{0, 0};
    }
    return coords{x, y};
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (buffer == nullptr) {
      Serial.println("DisplayBuffer: Buffer is null");
      return;
    }
    //Serial.printf("Coordinates: (%i, %i)\n", x, y);
    coords c = transformCoords(x, y);
    if (c.y * _width + c.x >= _width * _height) {
      Serial.println("DisplayBuffer: index out of range");
      return;
    }
    buffer[c.y * _width + c.x] = color;
  }

  void write(Adafruit_GFX& display, uint8_t xOff, uint8_t yOff, uint8_t width, uint8_t height) {
    if (buffer == nullptr) {
      Serial.println("DisplayBuffer: Buffer is null");
      return;
    }

    for (uint8_t x = xOff; x < xOff + width; x++) {
      for (uint8_t y = yOff; y < yOff + height; y++) {
        if (y * _width + x >= _width * _height) {
          Serial.println("DisplayBuffer: index out of range");
          return;
        }
        display.drawPixel(x, y, buffer[y * _width + x]);
      }
      if (width * height > PIXEL_YIELD_THRESHOLD)
        yield();
    }
  }

  void close() {
    delete(buffer);
  }
};

struct Widget {
  uint16_t id; // The id for the widget
  uint8_t type; // The type of widget
  std::string content; // The filename for the image, for instance
  uint8_t xOff; // The X offset for the widget
  uint8_t yOff; // The Y offset for the widget
  uint8_t width; // The width of the widget
  uint8_t height; // The height of the widget
  uint16_t updateFrequency; // The number of milliseconds between widget updates
  uint8_t offset; // The offset for animations, for instance
  uint8_t length; // The length of animations, for instance
  std::vector<uint16_t> colors; // The colors for text, for instance
  bool bordered; // Whether the widget has a border or not
  uint16_t borderColor; // The color of a border
  bool transparent; // Whether to fill region with backgroundColor before drawing widget or not
  uint16_t backgroundColor; // The background color if used
  bool background; // Whether the widget exists in the background or not
  ulong lastUpdate; // When the widget was last updated
  uint8_t state; // The animation frame, for instance
  bool dirty;
};

std::map<std::string, uint8_t*> progmemImages = {{"blm", blmAnimations}};

// Configuration values
std::vector<Widget> widgets;
uint16_t backgroundColor;
uint8_t brightnessMode;
uint8_t brightnessUpper;
uint8_t brightnessLower;
int timeOffset;
bool usingWeather;
bool usingTransparency;
bool metric;
String weatherKey;
String weatherLocation;

// State values
long weatherUpdateTime;
uint16_t weatherID = 200;
bool needsConfig; // If a configuration is needed
bool needsUpdate; // If a full update is needed
std::vector<uint16_t> backgroundBuffer;

DoubleResetDetector drd(10, 0);
ESP8266WebServer* server;
NTPClient* ntpClient;
OpenWeatherMapForecast client;
DisplayBuffer* displayBuffer;\

#pragma region Configuration
void writeDefaultConfig() {
  File file = SPIFFS.open("/config.json", "w");
  file.print("{\"widgets\":[{\"id\":0,\"type\":0,\"xOff\":0,\"yOff\":0,\"width\":64,\"height\":32,\"content\":\"blm\",\"disabled\":false,\"background\":true,\"frequency\":10000,\"length\":2},{\"id\":1,\"type\":4,\"xOff\":33,\"yOff\":15,\"width\":29,\"height\":7,\"content\":\"ABCDEFG\",\"colors\":[14304],\"disabled\":false,\"transparent\":false,\"backgroundColor\":63488,\"bordered\":true,\"borderColor\":63488},{\"id\":2,\"type\":7,\"xOff\":29,\"yOff\":2,\"width\":12,\"height\":7,\"disabled\":false,\"frequency\":1000,\"transparent\":false,\"backgroundColor\":0,\"bordered\":true,\"borderColor\":63488},{\"id\":3,\"type\":6,\"xOff\":1,\"yOff\":23,\"width\":61,\"height\":9,\"content\":\"ABCDEFGHIJ\",\"colors\":[14304],\"disabled\":false,\"backgroundColor\":0,\"bordered\":true,\"borderColor\":63488},{\"id\":4,\"type\":3,\"xOff\":46,\"yOff\":0,\"width\":15,\"height\":15,\"colors\":[10000, 20000],\"disabled\":false,\"frequency\":60000,\"bordered\":true,\"borderColor\":63488,\"backgroundColor\":0},{\"id\":5,\"type\":8,\"xOff\":1,\"yOff\":11,\"width\":31,\"height\":9,\"colors\":[14304],\"disabled\":false,\"frequency\":1000,\"backgroundColor\":0,\"bordered\":true,\"borderColor\":63488},{\"id\":5,\"type\":2,\"xOff\":2,\"yOff\":2,\"width\":21,\"height\":7,\"colors\":[2042],\"disabled\":false,\"frequency\":1000,\"backgroundColor\":0,\"bordered\":true,\"borderColor\":63488}],\"brightnessMode\":1,\"brightnessLower\":1,\"brightnessUpper\":100,\"backgroundColor\":0,\"timeOffset\":-4,\"metric\":false,\"weatherKey\":\"\",\"weatherLocation\":\"5014227\",\"transparency\":true}");
  file.close(); // 
}

bool parseConfig(char data[]) {
  Serial.print("Attempting to parse config: ");
  Serial.println(data);

  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, data);
  if (error) {
    Serial.print("Failed to deserialize config file: ");
    Serial.println(error.c_str());
    return false;
  }
  
  uint16_t tempBackgroundColor = doc["backgroundColor"];
  int tempTimeOffset = doc["timeOffset"];
  String tempWeatherKey = doc["weatherKey"].isNull() ? "" : String(doc["weatherKey"].as<char*>());
  String tempWeatherLocation = doc["weatherLocation"].isNull() ? "" : String(doc["weatherLocation"].as<char*>());
  bool tempMetric = doc["metric"];
  bool tempTransparency = doc["transparency"];


  if (doc["brightnessMode"].isNull() || doc["brightnessUpper"].isNull() || doc["brightnessLower"].isNull()) {
    Serial.println("Missing brightness key in config file");
    return false;
  }
  uint8_t tempBrightnessMode = doc["brightnessMode"];
  uint8_t tempBrightnessLower = doc["brightnessLower"];
  uint8_t tempBrightnessUpper = doc["brightnessUpper"];

  JsonArray configWidgets = doc["widgets"];
  if (!configWidgets) {
    Serial.println("Missing widgets key in config file");
    return false;
  }
  bool tempUsingWeather;
  std::vector<Widget> tempWidgets;
  for (ushort i = 0; i < configWidgets.size(); i++) {
    JsonObject widget = configWidgets[i];
    if (!widget || widget["id"].isNull() || widget["type"].isNull() || widget["xOff"].isNull() || widget["yOff"].isNull() || widget["width"].isNull() || widget["height"].isNull()) {
      Serial.printf("Failed to parse widget %i in list\n", i);
      return false;
    }
    if (widget["disabled"])
      continue;
    if (widget["type"] == WIDGET_WEATHER_ICON)
      tempUsingWeather = true;

    std::vector<uint16_t> colors;
    JsonArray c = widget["colors"];
    switch (widget["type"].as<uint8_t>()) {
    case WIDGET_ANALOG_CLOCK:
      if (c.size() < 2) {
        Serial.println("Analog clocks require 2 color arguments");
        return false;
      }
      break;
    case WIDGET_CLOCK:
      if (c.size() < 1) {
        Serial.println("Digital clocks require 1 color argument");
        return false;
      }
      break;
    case WIDGET_TEXT_BIG:
    case WIDGET_TEXT:
      if (c.size() < 1) {
        Serial.println("Text require 1 color argument");
        return false;
      }
      break;
    default:
      break;
    }
    for (uint i = 0; i < c.size(); i++)
      colors.push_back(c[i]);
    
    tempWidgets.push_back({widget["id"], widget["type"], widget["content"].isNull() ? "" : std::string(widget["content"].as<char*>()), widget["xOff"], widget["yOff"], widget["width"], widget["height"], widget["frequency"], widget["offset"], widget["length"], colors, widget["bordered"], widget["borderColor"], widget["transparent"], widget["backgroundColor"].as<uint16_t>(), widget["background"]});
  }
  std::sort(tempWidgets.begin(), tempWidgets.end(), [](Widget w1, Widget w2){return w1.id < w2.id;});
  
  // Only load new configuration after successful completion of config parsing
  needsUpdate = true;
  needsConfig = false;
  widgets = tempWidgets;
  brightnessLower = tempBrightnessLower;
  brightnessMode = tempBrightnessMode;
  brightnessUpper = tempBrightnessUpper;
  backgroundColor = tempBackgroundColor;
  timeOffset = tempTimeOffset;
  usingWeather = tempUsingWeather;
  weatherLocation = tempWeatherLocation;
  weatherKey = tempWeatherKey;
  metric = tempMetric;
  usingTransparency = tempTransparency;

  // Transparency uses about an extra 4KB
  if (usingTransparency && displayBuffer == nullptr) {
    Serial.println("Creating display buffer for transparency...");
    displayBuffer = new DisplayBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  } else if (!usingTransparency && displayBuffer != nullptr) {
    Serial.println("Freeing display buffer...");
    displayBuffer->close();
    delete(displayBuffer);
  }

  Serial.println("Successfully loaded configuration!");
  Serial.printf("Memory Free: %i, Fragmentation: %i%%\n", ESP.getFreeHeap(), ESP.getHeapFragmentation());
  return true;
}

bool getConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }
  
  size_t size = configFile.size();
  Serial.print("Configuration file size: ");
  Serial.println(size);
  if (size > MAX_CONFIG_FILE_SIZE) {
    Serial.println("Config file size is too large, replacing with default...");
    configFile.close();
    writeDefaultConfig();
    return false;
  }

  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);
  configFile.close();

  if (!parseConfig(buf.get())) {
    Serial.println("Failed to parse config, replacing with default...");
    writeDefaultConfig();
    return false;
  }

  return true;
}
#pragma endregion

#pragma region Web Server
void onStartAccessPoint(WiFiManager* WiFiManager) {
  Serial.println("Entering AP config mode");
  display.setCursor(0, 0);
  display.println("Wifi");
  display.println("Config");
  display.println("Mode");
  display.showBuffer();
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".json")) return "text/json";
  return "text/plain";
}

bool sendFile(String path) {
  if (path.endsWith("/"))
    return false;
  String contentType = getContentType(path);
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    server->streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void serveRoot() {
  sendFile("/index.html");
}

void serveConfig() {
  if (server->method() == HTTP_POST) {
    std::unique_ptr<char[]> buf(new char[MAX_CONFIG_FILE_SIZE]);
    String jsonString = server->arg("config");
    if (jsonString.length() > MAX_CONFIG_FILE_SIZE) {
      server->send(400, "text/html", "Config file is too large");
      return;
    }
    jsonString.toCharArray(buf.get(), MAX_CONFIG_FILE_SIZE);
    if (parseConfig(buf.get())) 
      Serial.println("Successfully updated config file");
    else
      Serial.println("Failed to update config file");
  } else {
    if (!sendFile("/config.json")) {
      writeDefaultConfig();
      sendFile("/config.json");
    }
  } 
}

void serveNotFound() {
  if (!sendFile(server->uri()))
    server->send(404, "text/html", "Not found");
}

void uploadFile() {

}
#pragma endregion

#pragma region Rendering
void drawImageUint8(Adafruit_GFX& d, uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height, uint8_t offset, const char name[], bool progMem) {
  if (progmemImages.count(name) == 0) {
    Serial.print("Couldn't find PROGMEM image to draw: ");
    Serial.println(name);
    return;
  }

  // Todo: Check for end of data...
  uint8_t* data = progmemImages[name];
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

void drawImageFsUint8(Adafruit_GFX& d, uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height, uint8_t offset, const char name[]) {
  if (!SPIFFS.exists(name)) {
    Serial.print("Couldn't find image to draw for: ");
    Serial.println(name);
    return;
  }

  File file = SPIFFS.open(name, "r");
  char line_buffer[width * 2];
  for (uint8_t y = 0; y < height; y++) {
    file.seek(offset * width * height * 2 + y * width * 2);
    file.readBytes(line_buffer, width * 2);
    for (uint8_t x = 0; x < width; x++)
      d.drawPixel(xOffset + x, yOffset + y, line_buffer[x * 2] | line_buffer[x * 2 + 1] << 8);
    if (width * height > PIXEL_YIELD_THRESHOLD)
      yield();
  }
}

void drawAnalogClock(Adafruit_GFX& d, uint8_t xOffset, uint8_t yOffset, uint8_t radius, uint16_t colorMinute, uint16_t colorHour) {
  uint h = hourFormat12();
  uint m = minute();
  d.drawLine(xOffset + radius, yOffset + radius, xOffset + radius + round(cos(2 * PI * (m / 60. - .25)) * (radius - 1)), yOffset + radius + round(sin(2 * PI * (m / 60. - .25)) * (radius - 1)), colorMinute);
  d.drawLine(xOffset + radius, yOffset + radius, xOffset + radius + round(cos(2 * PI * (h / 12. - .25)) * (radius - 3)), yOffset + radius + round(sin(2 * PI * (h / 12. - .25)) * (radius - 3)), colorHour);
}

const char* getTimeText() {
  uint h = hourFormat12();
  uint m = minute();
  std::string text;
  if (h < 10)
    text.append("0");
  text.append(dtostrf(h, h < 10 ? 1 : 2, 0, " "));
  text.append(":");
  if (m < 10)
    text.append("0");
  text.append(dtostrf(m, m < 10 ? 1 : 2, 0, " "));
  return text.c_str();
}

void drawBigText(Adafruit_GFX& d, const char* text, uint8_t xOffset, uint8_t yOffset, uint16_t color) {
  d.setTextColor(color);
  d.setCursor(xOffset, yOffset);
  d.print(text);
}

void incrementWidgetState(Widget& widget, uint8_t max) {
  if (max > 0) {
    widget.state++;
    widget.state %= max;
    widget.dirty = true;
  }
}

void incrementWidgetState(Widget& widget) {
  incrementWidgetState(widget, widget.length);
}

void updateWidget(Widget& widget) {
  switch (widget.type) {
    case WIDGET_PROGMEM_IMAGE:
    case WIDGET_FS_IMAGE:
      incrementWidgetState(widget);
      break;
    case WIDGET_TEXT_REST:

      break;
    case WIDGET_WEATHER_ICON:
      incrementWidgetState(widget, 5);
      break;
    case WIDGET_CLOCK:
    case WIDGET_CLOCK_BIG:
    case WIDGET_ANALOG_CLOCK:
    case WIDGET_TEXT_BIG:
    case WIDGET_TEXT:
      break;
    default:
      Serial.print("Invalid widget type: ");
      Serial.println(widget.type);
      break;
    }

  if (widget.background)
    needsUpdate = true;
  widget.lastUpdate = millis();
}

void updateWidgets() {
  for (uint i = 0; i < widgets.size(); i++) 
    if (widgets[i].updateFrequency > 0 && millis() - widgets[i].lastUpdate > widgets[i].updateFrequency) 
      updateWidget(widgets[i]);
}

void drawWidget(Adafruit_GFX& d, Widget &widget, bool buffering) {
  if (!usingTransparency || !widget.transparent) {
    if (widget.type == WIDGET_ANALOG_CLOCK)
      d.fillCircle(widget.xOff + widget.height / 2, widget.yOff + widget.height / 2, widget.height / 2, widget.backgroundColor);
    else
      d.fillRect(widget.xOff, widget.yOff, widget.width, widget.height, widget.backgroundColor);
  }
  else if (!buffering) {
    displayBuffer->write(display, widget.xOff, widget.yOff, widget.width, widget.height);
  }

  // Todo: check if background widgets change to queue a full update on the next frame
  switch (widget.type) {
    case WIDGET_PROGMEM_IMAGE:
      drawImageUint8(d, widget.xOff, widget.yOff, widget.width, widget.height, widget.state + widget.offset, widget.content.c_str(), true);
      break;
    case WIDGET_FS_IMAGE:
      drawImageFsUint8(d, widget.xOff, widget.yOff, widget.width, widget.height, widget.state + widget.offset, widget.content.c_str());
      break;
    case WIDGET_CLOCK:
      TFDrawText(d, getTimeText(), widget.xOff + (widget.width - (TF_COLS + 1) * 5 + 1) / 2, widget.yOff + (widget.height - TF_ROWS) / 2, widget.colors[0], true, 0);
      break;
    case WIDGET_CLOCK_BIG:
      drawBigText(d, getTimeText(), widget.xOff + (widget.width - 29) / 2, widget.yOff + (widget.height - 6) / 2, widget.colors[0]);
      break;
    case WIDGET_ANALOG_CLOCK:
      drawAnalogClock(d, widget.xOff, widget.yOff, widget.height / 2, widget.colors[0], widget.colors[1]);
      break;
    case WIDGET_TEXT:
      TFDrawText(d, widget.content.c_str(), widget.xOff + (widget.width - (TF_COLS + 1) * widget.content.length() + 1) / 2, widget.yOff + (widget.height - TF_ROWS) / 2, widget.colors[0], true, 0);
      break;
    case WIDGET_TEXT_REST:

      break;
    case WIDGET_TEXT_BIG:
      drawBigText(d, widget.content.c_str(), widget.xOff + (widget.width - 6 * widget.content.length() + 1) / 2, widget.yOff + (widget.height - 7) / 2, widget.colors[0]);
      break;
    case WIDGET_WEATHER_ICON:
      TIDrawIcon(d, weatherID, widget.xOff + 1, widget.yOff + 1, widget.state, true, widget.transparent, widget.backgroundColor);
      break;
    default:
      Serial.print("Invalid widget type: ");
      Serial.println(widget.type);
      break;
    }

    if (widget.bordered) {
      if (widget.type == WIDGET_ANALOG_CLOCK)
        d.drawCircle(widget.xOff + widget.height / 2, widget.yOff + widget.height / 2, widget.height / 2, widget.borderColor);
      else
        d.drawRect(widget.xOff, widget.yOff, widget.width, widget.height, widget.borderColor);
    }
}

void drawScreen(Adafruit_GFX& d, bool fullUpdate, bool buffering, bool finalize) {
  if (fullUpdate)
    d.fillScreen(backgroundColor);

  for (uint i = 0; i < widgets.size(); i++) {
    Widget& widget = widgets[i];
    if (buffering && !widget.background)
      return; // Background widgets should not come after foreground ones
      //continue;

    if (!fullUpdate && !widget.dirty)
      continue;
    
    drawWidget(d, widget, buffering);

    if (finalize)
      widget.dirty = false;
  }
}

void updateScreen(bool fullUpdate) {
  needsUpdate = false;
  updateWidgets();
  if (needsUpdate)
    return;
  if (usingTransparency && fullUpdate) 
    drawScreen(*displayBuffer, fullUpdate, true, false);
  drawScreen(display, fullUpdate, false, false);
  display.showBuffer();
  drawScreen(display, fullUpdate, false, true);
}
#pragma endregion

void handleBrightness() {
  switch(brightnessMode) {
    case BRIGHTNESS_AUTO:

      break;
    case BRIGHTNESS_STATIC:
      display.setBrightness(brightnessUpper);
      break;
    case BRIGHTNESS_TIME:

      break;
    case BRIGHTNESS_SUN:

      break;
    default:
      Serial.print("Invalid brightness mode: ");
      Serial.println(brightnessMode);
      break;
  }
}

void updateWeather() {
  OpenWeatherMapForecastData data[1];
  client.setMetric(metric);
  client.setLanguage("en");
  uint8_t allowedHours[] = {0, 12};
  client.setAllowedHours(allowedHours, 2);
  // See https://docs.thingpulse.com/how-tos/openweathermap-key/

  if (!client.updateForecastsById(data, weatherKey, weatherLocation, 1)) {
    Serial.println("Failed to update weather info");
    return;
  }

  weatherID = data[0].weatherId;
}

void handleWeather() {
  if (!usingWeather || millis() - weatherUpdateTime < WEATHER_UPDATE_INTERVAL)
    return;

  weatherUpdateTime = millis();
  updateWeather();
}

void setup() {
  Serial.begin(9600);
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
  display.fillScreen(GREEN);
  display.showBuffer();
  display.fillScreen(GREEN);

  SPIFFS.begin();

  WiFiManager wifiManager;
  wifiManager.setAPCallback(onStartAccessPoint);

  if (drd.detectDoubleReset()) {
    Serial.println("Double reset detected, entering config mode");
    wifiManager.startConfigPortal("LED Matrix Display");
    needsConfig = true;
  }
  else {
    Serial.println("Attemping to connect to network...");
    display.print("Connecting...");
    display.showBuffer();
    wifiManager.autoConnect("LED Matrix Display");
  }

  drd.stop();
  Serial.print("Connected to network with IP: ");
  Serial.println(WiFi.localIP());

  // Webserver
  server = new ESP8266WebServer(80);
  server->on("/", serveRoot);
  server->on("/config", serveConfig);
  //server->onFileUpload(uploadFile);
  server->onNotFound(serveNotFound);
  Serial.println("Starting configuration webserver...");
  server->begin();

  // Development FS stuff
  writeDefaultConfig();
  //SPIFFS.remove("/config.json");

  if (needsConfig || !getConfig()) {
    display.fillScreen(BLUE);
    display.setCursor(0, 0);
    display.println("Configure Display at");
    display.println(WiFi.localIP());
    display.showBuffer();
    needsConfig = true;
    while (needsConfig) {
      server->handleClient();
      yield();
    }
    Serial.println("Successfully configured display");
  }

  ntpClient = new NTPClient("time.nist.gov", 3600 * timeOffset, NTP_UPDATE_INTERVAL);
  ntpClient->begin();
  setSyncProvider([](){return (time_t)ntpClient->getRawTime();});
  setSyncInterval(10);

  updateWeather();
}

void loop() {
  ntpClient->update();
  handleWeather();
  handleBrightness();
  updateScreen(needsUpdate);
  server->handleClient();
}