#ifndef _RENDERER_H_
#define _RENDERER_H_

typedef struct renderer renderer_t;

renderer_t *renderer_create(
    uint16_t width,
    uint16_t height);

void renderer_render(
    renderer_t *renderer,
    const game_t *game);

const framebuffer_t *renderer_get_framebuffer(
    const renderer_t *renderer);

void renderer_destroy(renderer_t *renderer);



#endif