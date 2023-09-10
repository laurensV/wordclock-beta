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
          color = WHITE;
          break;
        case NAME:
          color = WHITE;
          break;
        case OTHER:
          color = GREEN;
          break;
      }
      uint16_t n;

      (*pixels).setPixelColor(pixelIndex(row, col), color);
    }
  }
  (*pixels).show();
}

/**
 * shows a byte buffer
 */
void LEDMatrix::print(const byte* drawing, uint8_t xpos, uint8_t ypos, enum LED_TYPE type) {
  for (int i = 0; i < FONT_HEIGHT; ++i) {
    for (int j = 0; j < FONT_WIDTH; ++j) {
      if (drawing[i] & 1 << j) {
        setPixelType(i + ypos, FONT_WIDTH - j - 1 + xpos, type);
      }
    }
  }
  // for (int y = ypos, i = 0; y < (ypos + 5); y++, i++) {
  //   for (int x = xpos, k = 2; x < (xpos + 3); x++, k--) {
  //     if ((drawing[i] >> k) & 0x1) {
  //       setPixelType(x, y, type);
  //     }
  //   }
  // }
}

uint16_t LEDMatrix::pixelIndex(uint8_t row, uint8_t col) {
  if (row % 2) {
    // Odd row, right to left
    return row * CLOCK_WIDTH + (CLOCK_WIDTH - 1 - col);
  }
  // Even row, left to right
  return row * CLOCK_WIDTH + col;
}