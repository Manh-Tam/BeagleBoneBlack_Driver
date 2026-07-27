#include "draw.h"

void draw_rectangle_u8(uint8_t *display_ram, int start_x, int start_y, int w, int h, uint16_t color) 
{
    /* Split the 16-bit color into Big-Endian bytes */
    uint8_t hi = (color >> 8) & 0xFF;
    uint8_t lo = color & 0xFF;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int tx = start_x + x;
            int ty = start_y + y;
            
            if (tx < SCREEN_W && ty < SCREEN_H && tx >= 0 && ty >= 0) {
                /* Calculate the byte offset: 2 bytes per pixel */
                int pixel_index = (ty * SCREEN_W + tx) * 2;
                
                display_ram[pixel_index]     = hi; /* First byte */
                display_ram[pixel_index + 1] = lo; /* Second byte */
            }
        }
    }
}

void render_frame(uint8_t *display_ram) 
{
    /* 1. Draw Menu Row 0 */
    for (int col = 0; col < 8; col++) {
        draw_rectangle_u8(display_ram, col * TILE_W, 0, TILE_W, TILE_H, 0x72A4);
    }

    /* 2. Draw Lawn Rows 1 to 5 */
    for (int row = 1; row < 6; row++) {
        for (int col = 0; col < 8; col++) {
            int x = col * TILE_W;
            int y = row * TILE_H;
            
            if ((row + col) % 2 == 0) {
                draw_rectangle_u8(display_ram, x, y, TILE_W, TILE_H, 0x5E68); // Light Green
            } else {
                draw_rectangle_u8(display_ram, x, y, TILE_W, TILE_H, 0x3DC4); // Dark Green
            }
        }
    }

}

void draw_sprite(uint8_t *display_ram, const uint8_t *sprite, int start_x, int start_y, int width, int height)
{
    if ((start_x >= 320) || (start_y >= 240))
        return;
    int bpp = 2;
    for (int i = 0; i < height; i++)
    {
        uint8_t image[width * bpp];
        for (int j = 0; j < width * bpp; j++)
        {
            if (sprite[j + i * width * bpp] == 0xff)
            {
                image[j] = display_ram[bpp * start_x + 320 * bpp * (i + start_y) + j];
            }
            else
                image[j] = sprite[j + i * width * bpp];
        }
        // memcpy(display_ram + bpp * start_x + 320 * bpp * (i + start_y), sprite + width * bpp * i, width * bpp);
        memcpy(display_ram + bpp * start_x + 320 * bpp * (i + start_y), image, width * bpp);
    }
}