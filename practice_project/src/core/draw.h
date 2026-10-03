#ifndef PVZ_DRAW_H
#define PVZ_DRAW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *pixels;
    size_t size;
    uint16_t width;
    uint16_t height;
    size_t stride;
    bool owns_pixels;
} framebuffer_t;

uint16_t draw_rgb555(uint8_t red, uint8_t green, uint8_t blue);
bool framebuffer_init(framebuffer_t *framebuffer, uint16_t width, uint16_t height);
bool framebuffer_bind(framebuffer_t *framebuffer, uint8_t *pixels, size_t size,
                      uint16_t width, uint16_t height, size_t stride);
void framebuffer_destroy(framebuffer_t *framebuffer);
void draw_fill_rectangle(framebuffer_t *framebuffer, int x, int y,
                         int width, int height, uint16_t color);
void draw_sprite(framebuffer_t *framebuffer, const uint8_t *sprite,
                 uint16_t sprite_width, uint16_t sprite_height, int x, int y);
void draw_digit(framebuffer_t *framebuffer, int x, int y, int digit,
                uint16_t color);

#endif
