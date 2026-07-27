#ifndef __DRAW_H__
#define __DRAW_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SCREEN_W            320
#define SCREEN_H            240
#define TILE_W              40
#define TILE_H              40

void draw_rectangle_u8(uint8_t *display_ram, int start_x, int start_y, int w, int h, uint16_t color);
void render_frame(uint8_t *display_ram);
void draw_sprite(uint8_t *display_ram, const uint8_t *sprite, int start_x, int start_y, int width, int height);

#endif