#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <ESP_DoubleResetDetector.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <map>
#include <TinyFont.h>

#include <blm.h>

#pragma region PxMatrix
//#define PxMATRIX_COLOR_DEPTH 4
#define double_buffer
#define PxMATRIX_MAX_HEIGHT 32
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

#pragma region Constants
const uint16_t RED = display.color565(255, 0, 0);
const uint16_t GREEN = display.color565(0, 255, 0);
const uint16_t BLUE = display.color565(0, 0, 255);
const uint16_t WHITE = display.color565(255, 255, 255);
const uint16_t YELLOW = display.color565(255, 255, 0);
const uint16_t CYAN = display.color565(0, 255, 255);
const uint16_t MAGENTA = display.color565(255, 0, 255);
const uint16_t BLACK = display.color565(0, 0, 0);

#define WIDGET_PROGMEM_IMAGE 0
#define WIDGET_FS_IMAGE 1
#define WIDGET_CLOCK 2
#define WIDGET_TEXT 3
#define WIDGET_TEXT_REST 4
#define BRIGHTNESS_AUTO 0
#define BRIGHTNESS_STATIC 1
#define BRIGHTNESS_TIME 2
#define BRIGHTNESS_SUN 3
#define MAX_CONFIG_FILE_SIZE 1024
#define MAX_JSON_BUFFER_SIZE 400
#pragma endregion

struct Widget {
  uint16_t id; // The id for the widget
  uint8_t type; // The type of widget
  std::string name; // The filename for the image, for instance
  uint8_t xOff; // The X offset for the widget
  uint8_t yOff; // The Y offset for the widget
  uint8_t width; // The width of the widget
  uint8_t height; // The height of the widget
  uint16_t updateFrequency; // The number of milliseconds between widget updates
  uint8_t offset; // The offset for animations, for instance
  uint8_t length; // The length of animations, for instance
  uint16_t color; // The color for text, for instance
  bool bordered; // Whether the widget has a border or not
  uint16_t borderColor; // The color of a border
  bool transparent; // Whether to fill region with backgroundColor before drawing widget or not
  uint16_t backgroundColor; // The background color if used
  std::vector<uint16_t> backgroundBuffer; // Store background data behind widget for partial re-draws
  ulong lastUpdate; // When the widget was last updated
  uint8_t state; // The animation frame, for instance
};

std::map<std::string, uint8_t*> progmemImages = {{"blm", blmAnimations}};

std::vector<Widget> widgets;
uint16_t backgroundColor;
uint8_t brightnessMode;
uint8_t brightnessUpper;
uint8_t brightnessLower;

DoubleResetDetector drd(10, 0);
ESP8266WebServer* server;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

bool needsConfig;

#pragma region Configuration
void writeDefaultConfig() {
  File file = SPIFFS.open("/config.json", "w");
  file.print("{\"widgets\":[{\"id\":0,\"type\":0,\"xOff\":0,\"yOff\":0,\"width\":64,\"height\":32,\"frequency\":0,\"name\":\"blm\"},{\"id\":1,\"type\":3,\"xOff\":5,\"yOff\":10,\"width\":53,\"height\":7,\"name\":\"ABCDEFGHIJKLM\",\"color\":34469,\"disabled\":true}],\"brightnessMode\":1,\"brightnessLower\":1,\"brightnessUpper\":100,\"backgroundColor\":45402}");
  file.close();
}

bool parseConfig(char data[]) {
  Serial.print("Attempting to parse config: ");
  Serial.println(data);

  DynamicJsonDocument doc(MAX_JSON_BUFFER_SIZE);
  DeserializationError error = deserializeJson(doc, data);
  if (error) {
    Serial.print("Failed to deserialize config file: ");
    Serial.println(error.c_str());
    return false;
  }
  
  uint16_t tempBackgroundColor = doc["backgroundColor"].isNull() ? 0 : doc["backgroundColor"];

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
  std::vector<Widget> tempWidgets;
  for (ushort i = 0; i < configWidgets.size(); i++) {
    JsonObject widget = configWidgets[i];
    if (!widget || widget["id"].isNull() || widget["type"].isNull() || widget["xOff"].isNull() || widget["yOff"].isNull() || widget["width"].isNull() || widget["height"].isNull() || !widget["name"]) {
      Serial.printf("Failed to parse widget %i in list\n", i);
      return false;
    }
    if (widget["disabled"])
      continue;
    tempWidgets.push_back({widget["id"], widget["type"], std::string(widget["name"].as<char*>()), widget["xOff"], widget["yOff"], widget["width"], widget["height"], widget["frequency"].isNull() ? 0 :widget["frequency"], 0, 1, widget["color"].isNull() ? 0 : widget["color"], true, BLUE, false, RED});
  }
  std::sort(tempWidgets.begin(), tempWidgets.end(), [](Widget w1, Widget w2){return w1.id < w2.id;});
  
  // Only load new configuration after successful completion of config parsing
  needsConfig = false;
  widgets = tempWidgets;
  brightnessLower = tempBrightnessLower;
  brightnessMode = tempBrightnessMode;
  brightnessUpper = tempBrightnessUpper;
  backgroundColor = tempBackgroundColor;

  Serial.println("Successfully loaded configuration!");
  return true;
}

bool getConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }
  
  size_t size = configFile.size();
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
void drawImageUint8(uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height, uint8_t offset, uint8_t data[], bool yields, bool progMem) {
  uint16_t line_buffer[width];
  for (uint8_t y = 0; y < height; y++) {
    if (progMem)
      memcpy_P(line_buffer, data + offset * width * height * 2 + y * width * 2, width * 2);
    else
      memcpy(line_buffer, data + offset * width * height * 2 + y * width * 2, width * 2);
    for (uint8_t x = 0; x < width; x++) 
      display.drawPixelRGB565(xOffset + x, yOffset + y, line_buffer[x]);
    if (yields)
      yield();
  }
}

void drawImageFsUint8(uint8_t xOffset, uint8_t yOffset, uint8_t width, uint8_t height, uint8_t offset, uint8_t data[], bool yields) {
  File file = SPIFFS.open("background", "r");
  char line_buffer[width * 2];
  for (uint8_t y = 0; y < height; y++) {
    file.seek(offset * width * height * 2 + y * width * 2);
    file.readBytes(line_buffer, width * 2);
    for (uint8_t x = 0; x < width; x++)
      display.drawPixelRGB565(xOffset + x, yOffset + y, line_buffer[x * 2] | line_buffer[x * 2 + 1] << 8);
    if (yields)
      yield();
  }
}

void drawWidget(Widget &widget) {
  if (!widget.transparent)
    display.fillRect(widget.xOff, widget.yOff, widget.width, widget.height, widget.backgroundColor);

  switch (widget.type) {
    case WIDGET_PROGMEM_IMAGE:
      if (progmemImages.count(widget.name) == 0) {
        Serial.print("Couldn't find PROGMEM image to draw: ");
        Serial.println(widget.name.c_str());
        return;
      }
      drawImageUint8(widget.xOff, widget.yOff, widget.width, widget.height, widget.offset, progmemImages[widget.name], true, true);
      break;
    case WIDGET_FS_IMAGE:

      break;
    case WIDGET_CLOCK:

      break;
    case WIDGET_TEXT:
      TFDrawText(display, widget.name.c_str(), widget.xOff + (widget.width - (TF_COLS + 1) * widget.name.length() + 1) / 2, widget.yOff + (widget.height - TF_ROWS) / 2, widget.color, true, 0);
      break;
    case WIDGET_TEXT_REST:

      break;
    default:
      Serial.print("Invalid widget type: ");
      Serial.println(widget.type);
      break;
    }

    if (widget.bordered)
      display.drawRect(widget.xOff, widget.yOff, widget.width, widget.height, widget.borderColor);
}

void drawScreen(bool fullUpdate) {
  if (fullUpdate)
    display.fillScreen(backgroundColor);

  for (uint i = 0; i < widgets.size(); i++) {
    Widget widget = widgets[i];
    if (!fullUpdate && (widget.updateFrequency == 0 || millis() - widget.lastUpdate < widget.updateFrequency))
      continue;
    widget.lastUpdate = millis();
    drawWidget(widget);
  }
}

void updateScreen(bool fullUpdate) {
  drawScreen(fullUpdate);
  display.showBuffer();
  drawScreen(fullUpdate);
  display.showBuffer();
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

  timeClient.setTimeOffset(0); // Todo: Handle timezones
  timeClient.setUpdateInterval(3600000);
  timeClient.begin();
  setSyncProvider([](){return (time_t)timeClient.getEpochTime();});

  updateScreen(true);
}

void loop() {
  timeClient.update();

  handleBrightness();
  updateScreen(false);

  server->handleClient();
}