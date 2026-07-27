#include "digit_bitmaps.h"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define DISPLAY_SIZE  (SCREEN_WIDTH * SCREEN_HEIGHT * 2) // 153,600 bytes

// If display_ram is declared as uint8_t in your project:
extern uint8_t display_ram[DISPLAY_SIZE];

void draw_digit_sprite(int start_x, int start_y, int digit, uint16_t color_555) {
    if (digit < 0 || digit > 9) return;
    if (start_x < 0 || start_x + DIGIT_WIDTH > SCREEN_WIDTH) return; // Edge boundary guard

    uint16_t *buffer16 = (uint16_t *)display_ram;
    const uint8_t *bitmap = digit_bitmaps[digit];

    for (int row = 0; row < DIGIT_HEIGHT; row++) {
        int py = start_y + row;
        if (py < 0 || py >= SCREEN_HEIGHT) continue;

        uint8_t line = bitmap[row];
        uint16_t *dest = &buffer16[py * SCREEN_WIDTH + start_x];

        if (line & 0x80) dest[0] = color_555;
        if (line & 0x40) dest[1] = color_555;
        if (line & 0x20) dest[2] = color_555;
        if (line & 0x10) dest[3] = color_555;
        if (line & 0x08) dest[4] = color_555;
        if (line & 0x04) dest[5] = color_555;
        if (line & 0x02) dest[6] = color_555;
        if (line & 0x01) dest[7] = color_555;
    }
}