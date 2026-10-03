#include "draw.h"

#include <stdlib.h>
#include <string.h>

#include "digit_bitmaps.h"

static void put_pixel(framebuffer_t *framebuffer, int x, int y, uint16_t color)
{
    size_t offset;

    if (framebuffer == NULL || framebuffer->pixels == NULL ||
        x < 0 || y < 0 || x >= framebuffer->width || y >= framebuffer->height) {
        return;
    }
    offset = (size_t)y * framebuffer->stride + (size_t)x * 2U;
    framebuffer->pixels[offset] = (uint8_t)(color >> 8);
    framebuffer->pixels[offset + 1U] = (uint8_t)(color & UINT16_C(0x00ff));
}

uint16_t draw_rgb555(uint8_t red, uint8_t green, uint8_t blue)
{
    return (uint16_t)(((uint16_t)(red >> 3) << 10) |
                      ((uint16_t)(green >> 3) << 5) |
                      (uint16_t)(blue >> 3));
}

bool framebuffer_bind(framebuffer_t *framebuffer, uint8_t *pixels, size_t size,
                      uint16_t width, uint16_t height, size_t stride)
{
    if (framebuffer == NULL || pixels == NULL || width == 0U || height == 0U ||
        stride < (size_t)width * 2U || size < stride * height) {
        return false;
    }
    *framebuffer = (framebuffer_t) {
        .pixels = pixels,
        .size = size,
        .width = width,
        .height = height,
        .stride = stride,
        .owns_pixels = false,
    };
    return true;
}

bool framebuffer_init(framebuffer_t *framebuffer, uint16_t width, uint16_t height)
{
    size_t stride;
    size_t size;
    uint8_t *pixels;

    if (framebuffer == NULL || width == 0U || height == 0U) {
        return false;
    }
    stride = (size_t)width * 2U;
    size = stride * height;
    pixels = calloc(size, 1U);
    if (pixels == NULL || !framebuffer_bind(framebuffer, pixels, size, width,
                                             height, stride)) {
        free(pixels);
        return false;
    }
    framebuffer->owns_pixels = true;
    return true;
}

void framebuffer_destroy(framebuffer_t *framebuffer)
{
    if (framebuffer != NULL) {
        if (framebuffer->owns_pixels) {
            free(framebuffer->pixels);
        }
        memset(framebuffer, 0, sizeof(*framebuffer));
    }
}

void draw_fill_rectangle(framebuffer_t *framebuffer, int x, int y,
                         int width, int height, uint16_t color)
{
    int start_x;
    int start_y;
    int end_x;
    int end_y;

    if (framebuffer == NULL || framebuffer->pixels == NULL ||
        width <= 0 || height <= 0) {
        return;
    }
    start_x = x < 0 ? 0 : x;
    start_y = y < 0 ? 0 : y;
    end_x = x + width;
    end_y = y + height;
    if (end_x > framebuffer->width) {
        end_x = framebuffer->width;
    }
    if (end_y > framebuffer->height) {
        end_y = framebuffer->height;
    }

    for (int py = start_y; py < end_y; ++py) {
        for (int px = start_x; px < end_x; ++px) {
            put_pixel(framebuffer, px, py, color);
        }
    }
}

void draw_sprite(framebuffer_t *framebuffer, const uint8_t *sprite,
                 uint16_t sprite_width, uint16_t sprite_height, int x, int y)
{
    if (framebuffer == NULL || framebuffer->pixels == NULL || sprite == NULL) {
        return;
    }

    for (uint16_t source_y = 0U; source_y < sprite_height; ++source_y) {
        int destination_y = y + source_y;
        if (destination_y < 0 || destination_y >= framebuffer->height) {
            continue;
        }
        for (uint16_t source_x = 0U; source_x < sprite_width; ++source_x) {
            int destination_x = x + source_x;
            size_t source_offset;
            size_t destination_offset;
            if (destination_x < 0 || destination_x >= framebuffer->width) {
                continue;
            }
            source_offset = ((size_t)source_y * sprite_width + source_x) * 2U;
            if (sprite[source_offset] == UINT8_C(0xff) &&
                sprite[source_offset + 1U] == UINT8_C(0xff)) {
                continue;
            }
            destination_offset = (size_t)destination_y * framebuffer->stride +
                                 (size_t)destination_x * 2U;
            framebuffer->pixels[destination_offset] = sprite[source_offset];
            framebuffer->pixels[destination_offset + 1U] =
                sprite[source_offset + 1U];
        }
    }
}

void draw_digit(framebuffer_t *framebuffer, int x, int y, int digit,
                uint16_t color)
{
    if (digit < 0 || digit > 9) {
        return;
    }

    for (int row = 0; row < PVZ_DIGIT_HEIGHT; ++row) {
        uint8_t bits = pvz_digit_bitmaps[digit][row];
        for (int column = 0; column < PVZ_DIGIT_WIDTH; ++column) {
            if ((bits & (uint8_t)(UINT8_C(0x80) >> column)) != 0U) {
                put_pixel(framebuffer, x + column, y + row, color);
            }
        }
    }
}
