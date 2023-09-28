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
#define ADR_RTC_SET 0 // bool
#define ADR_COLOR_TIME sizeof(bool) // uint32_t
#define ADR_COLOR_NAME (ADR_COLOR_TIME + sizeof(uint32_t)) // uint32_t
#define ADR_BRIGHTNESS (ADR_COLOR_NAME + sizeof(uint32_t)) // uint8_t
#define ADR_NM (ADR_BRIGHTNESS + sizeof(uint8_t)) // bool
#define ADR_NM_START_H (ADR_NM + sizeof(bool)) // uint8_t
#define ADR_NM_END_H (ADR_NM_START_H + sizeof(uint8_t)) // uint8_t
#define ADR_NM_START_M (ADR_NM_END_H + sizeof(uint8_t)) // uint8_t
#define ADR_NM_END_M (ADR_NM_START_M + sizeof(uint8_t)) // uint8_t

#define EEPROM_SIZE (ADR_NM_END_M + sizeof(uint8_t)) 

#define CLOCK_WIDTH 13
#define CLOCK_HEIGHT 13
#define NUM_PIXELS (CLOCK_WIDTH * CLOCK_HEIGHT)

#define NEOPIXEL_PIN 0 // pin to which the NeoPixels are attached

// ----------------------------------------------------------------------------------
//                              FUNCTIONS & VARS
// ----------------------------------------------------------------------------------
inline uint32_t color_TIME;
inline uint32_t color_NAME;
inline uint8_t brightness;
inline bool checkNightMode;
inline bool nightMode;
inline uint8_t nightModeStartHour;
inline uint8_t nightModeStartMin;
inline uint8_t nightModeEndHour;
inline uint8_t nightModeEndMin;

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