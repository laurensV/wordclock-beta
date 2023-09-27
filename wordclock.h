#ifndef wordclock_h
#define wordclock_h

// ----------------------------------------------------------------------------------
//                                   CONSTANTS
// ----------------------------------------------------------------------------------
#define PERIOD_READTIME 1000
#define PERIOD_CHECK_UPDATE 60000
#define PERIOD_STORE_COLORS 60 * 1000
#define PERIOD_NTP_UPDATE 60 * 1000
#define AP_SSID "JouwWoordklok"
#define HOSTNAME "jouwwoordklok"
#define TIMEZONE TZ_Europe_Amsterdam
#define AUTO_UPDATE true

// EEPROM to save persistent variables
#define ADR_RTC 0
#define ADR_COLOR_TIME sizeof(int)
#define ADR_COLOR_NAME (sizeof(int) + sizeof(uint32_t))
#define EEPROM_SIZE (ADR_COLOR_NAME + sizeof(uint32_t)) 

#define CLOCK_WIDTH 13
#define CLOCK_HEIGHT 13
#define NUM_PIXELS (CLOCK_WIDTH * CLOCK_HEIGHT)

#define NEOPIXEL_PIN 0 // pin to which the NeoPixels are attached

// ----------------------------------------------------------------------------------
//                              FUNCTIONS & VARS
// ----------------------------------------------------------------------------------
inline uint32_t color_TIME;
inline uint32_t color_NAME;

void print(String message, bool newline = true);
void print(int number, bool newline = true);

// ----------------------------------------------------------------------------------
//                               WORDCLOCK LAYOUT
// ----------------------------------------------------------------------------------
const String letters = "\
HETVISNEENZES\
TWEEDRIEZVIER\
VIJFZEVENACHT\
NEGEN-TIENELF\
TWAALFDERTIEN\
VEERTIENKWART\
VOOROVERTHALF\
EENXTWEENDRIE\
VIERGVIJFNZES\
KLZEVENMACHTR\
FNEGENSTIENEE\
TWAALFITOTV**\
ELFLNUURDUS**";

#endif