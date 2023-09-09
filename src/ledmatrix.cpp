#include "ledmatrix.h"

/**
 * @brief Construct a new LEDMatrix::LEDMatrix object
 *
 * @param neopixels pointer to Adafruit_NeoPixel object
 */
LEDMatrix::LEDMatrix(Adafruit_NeoPixel* neopixels) {
  pixels = neopixels;
}

/**
 * @brief Set a pixel in led matrix to a color type
 *
 * @param x x-position of pixel
 * @param y y-position of pixel
 * @param type type of pixel
 */
void LEDMatrix::setPixelType(uint8_t x, uint8_t y, enum LED_TYPE type) {
  // limit ranges of x and y
  if (x >= 0 && x < CLOCK_WIDTH && y >= 0 && y < CLOCK_HEIGHT) {
    leds[x][y] = type;
  }
}

/**
 * @brief "Deactivates" all pixels in matrix
 *
 */
void LEDMatrix::clear(void) {
  // set a zero to each pixel
  for (uint8_t i = 0; i < CLOCK_WIDTH; i++) {
    for (uint8_t j = 0; j < CLOCK_HEIGHT; j++) {
      leds[i][j] = OFF;
    }
  }
}

/**
 * @brief Draws the led matrix to the neopixels
 */
void LEDMatrix::draw() {
  // loop over all leds in matrix
  for (int row = 0; row < CLOCK_WIDTH; row++) {
    for (int col = 0; col < CLOCK_HEIGHT; col++) {
      uint32_t color;
      switch (leds[row][col]) {
        case OFF:
          color = BLACK;
          break;
        case TIME:
          color = BLUE;
          break;
        case NAME:
          color = RED;
          break;
        default:
          color = WHITE;
          break;
      }
      uint16_t n;
      if (row % 2) {
        // Odd row, right to left
        n = row * CLOCK_WIDTH + (CLOCK_WIDTH - 1 - col);
      } else {
        // Even row, left to right
        n = row * CLOCK_WIDTH + col;
      }
      (*pixels).setPixelColor(n, color);
    }
  }
  (*pixels).show();
}

/**
 * shows a byte buffer
 */
void LEDMatrix::print(const byte* ledbuffer, uint8_t xpos, uint8_t ypos, uint8_t width, uint8_t height, enum LED_TYPE type) {

}
