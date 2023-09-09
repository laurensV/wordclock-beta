#ifndef wordclock_h
#define wordclock_h

// ----------------------------------------------------------------------------------
//                                        CONSTANTS
// ----------------------------------------------------------------------------------
#define PERIOD_READTIME 1000
#define PERIOD_CHECK_UPDATE 60000
#define PERIOD_NTP_UPDATE 60 * 1000
#define AP_SSID "JouwWoordklok"
#define HOSTNAME "jouwwoordklok"
#define TIMEZONE TZ_Europe_Amsterdam
#define AUTO_UPDATE true

#define EEPROM_SIZE 4 // size of EEPROM to save persistent variables
#define ADR_RTC 0

#define CLOCK_WIDTH 13
#define CLOCK_HEIGHT 13
#define NUM_PIXELS (CLOCK_WIDTH * CLOCK_HEIGHT)

#define NEOPIXEL_PIN 0 // pin to which the NeoPixels are attached


void print(String message, bool newline = true);

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