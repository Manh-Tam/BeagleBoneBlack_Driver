#ifndef PVZ_RENDERER_H
#define PVZ_RENDERER_H

#include <stdint.h>

#include "draw.h"
#include "game.h"

typedef struct renderer renderer_t;

renderer_t *renderer_create(uint16_t width, uint16_t height);
void renderer_destroy(renderer_t *renderer);
void renderer_render(renderer_t *renderer, const game_render_view_t *view);
const framebuffer_t *renderer_get_framebuffer(const renderer_t *renderer);

#endif
