#include <Arduino.h>
#if USE_TETRIS
#include <TetrisMatrixDraw.h>
#endif

struct Widget {
    // Configuration
    uint16_t id; // The id for the widget
    uint8_t type; // The type of widget
    std::string content; // The filename for the image, for instance
    std::string source; // The URL for get requests, for instance
    uint8_t contentType; // Extra information about the content
    std::string auth; // Authorization string for requests
    std::string cert; // Root certificate for HTTPS
    std::vector <std::string> args; // Arguments for parsing of JSON response, for instance
    uint8_t xOff; // The X offset for the widget
    uint8_t yOff; // The Y offset for the widget
    uint8_t width; // The width of the widget
    uint8_t height; // The height of the widget
    uint16_t updateFrequency; // The number of milliseconds between widget updates
    uint8_t offset; // The offset for animations, for instance
    uint16_t length; // The length of animations, for instance, or request timeout
    std::vector <uint16_t> colors; // The colors for text, for instance
    uint8_t font; // The font to use
    bool bordered; // Whether the widget has a border or not
    uint16_t borderColor; // The color of a border
    bool transparent; // Whether to fill region with backgroundColor before drawing widget or not
    uint16_t backgroundColor; // The background color if used
    bool background; // Whether the widget exists in the background or not
    // State
    time_t lastUpdate; // When the widget was last updated
    uint8_t state; // The animation frame, for instance
    uint8_t stateExtra; // An additional state value, used for colon
    bool finished; // Whether a tetris animation is completed, for instance
    bool dirty; // Whether a refresh is queued
    File file; // Open file for images to prevent file operation display flashing
    std::shared_ptr<uint8_t> cache;
#if USE_TETRIS
    std::shared_ptr<TetrisMatrixDraw> tetris;
#endif
};

struct FSImage {
    uint8_t width;
    uint8_t height;
    uint8_t length;
};

struct ProgmemImage {
    uint8_t width;
    uint8_t height;
    uint8_t length;
    uint8_t type;
    void *data;
};