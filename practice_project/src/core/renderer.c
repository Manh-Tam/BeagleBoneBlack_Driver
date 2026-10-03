#include "renderer.h"

#include <stdio.h>
#include <stdlib.h>

#include "../config.h"
#include "assets.h"
#include "digit_bitmaps.h"

struct renderer {
    framebuffer_t framebuffer;
};

static void draw_asset(framebuffer_t *framebuffer, const asset_image_t *asset,
                       screen_point_t position)
{
    if (asset != NULL) {
        draw_sprite(framebuffer, asset->pixels, asset->width, asset->height,
                    position.x, position.y);
    }
}

static void draw_number(framebuffer_t *framebuffer, int x, int y, int value,
                        uint16_t color)
{
    char text[16];
    int length = snprintf(text, sizeof(text), "%d", value);

    if (length <= 0) {
        return;
    }
    for (int i = 0; i < length && text[i] != '\0'; ++i) {
        if (text[i] >= '0' && text[i] <= '9') {
            draw_digit(framebuffer, x, y, text[i] - '0', color);
            x += PVZ_DIGIT_WIDTH + 2;
        }
    }
}

static void draw_border(framebuffer_t *framebuffer, int x, int y,
                        int width, int height, uint16_t color)
{
    draw_fill_rectangle(framebuffer, x, y, width, 2, color);
    draw_fill_rectangle(framebuffer, x, y + height - 2, width, 2, color);
    draw_fill_rectangle(framebuffer, x, y, 2, height, color);
    draw_fill_rectangle(framebuffer, x + width - 2, y, 2, height, color);
}

static void draw_background(framebuffer_t *framebuffer)
{
    uint16_t menu = draw_rgb555(110U, 175U, 40U);
    uint16_t light_grass = draw_rgb555(115U, 205U, 65U);
    uint16_t dark_grass = draw_rgb555(75U, 170U, 45U);

    draw_fill_rectangle(framebuffer, 0, 0, PVZ_SCREEN_WIDTH,
                        PVZ_TILE_HEIGHT, menu);
    for (int row = 0; row < PVZ_GAME_ROWS; ++row) {
        for (int column = 0; column < PVZ_GAME_COLUMNS; ++column) {
            uint16_t color = ((row + column) % 2) == 0
                                 ? light_grass : dark_grass;
            draw_fill_rectangle(framebuffer, column * PVZ_TILE_WIDTH,
                                (row + 1) * PVZ_TILE_HEIGHT,
                                PVZ_TILE_WIDTH, PVZ_TILE_HEIGHT, color);
        }
    }
}

renderer_t *renderer_create(uint16_t width, uint16_t height)
{
    renderer_t *renderer;

    if (width != PVZ_SCREEN_WIDTH || height != PVZ_SCREEN_HEIGHT) {
        return NULL;
    }
    renderer = calloc(1U, sizeof(*renderer));
    if (renderer == NULL) {
        return NULL;
    }
    if (!framebuffer_init(&renderer->framebuffer, width, height)) {
        free(renderer);
        return NULL;
    }
    return renderer;
}

void renderer_destroy(renderer_t *renderer)
{
    if (renderer != NULL) {
        framebuffer_destroy(&renderer->framebuffer);
        free(renderer);
    }
}

void renderer_render(renderer_t *renderer, const game_render_view_t *view)
{
    framebuffer_t *framebuffer;
    const uint16_t yellow = draw_rgb555(255U, 255U, 0U);
    const uint16_t red = draw_rgb555(255U, 0U, 0U);

    if (renderer == NULL || view == NULL) {
        return;
    }
    framebuffer = &renderer->framebuffer;
    draw_background(framebuffer);

    draw_asset(framebuffer, assets_menu_item(PLANT_SUNFLOWER),
               (screen_point_t){0, 0});
    draw_asset(framebuffer, assets_menu_item(PLANT_PEASHOOTER),
               (screen_point_t){PVZ_TILE_WIDTH, 0});
    draw_number(framebuffer, 2, 26, view->sunflower_price, yellow);
    draw_number(framebuffer, 42, 26, view->peashooter_price, yellow);
    draw_number(framebuffer, 90, 12, view->sun, yellow);
    draw_number(framebuffer, 170, 12, (int)view->zombie_count, yellow);

    if (view->selected_plant == PLANT_SUNFLOWER) {
        draw_border(framebuffer, 0, 0, PVZ_TILE_WIDTH, PVZ_TILE_HEIGHT, yellow);
    } else if (view->selected_plant == PLANT_PEASHOOTER) {
        draw_border(framebuffer, PVZ_TILE_WIDTH, 0,
                    PVZ_TILE_WIDTH, PVZ_TILE_HEIGHT, yellow);
    }

    for (size_t i = 0U; i < view->plant_count; ++i) {
        draw_asset(framebuffer,
                   assets_plant(view->plants[i].type, view->plants[i].state),
                   view->plants[i].position);
    }
    for (size_t i = 0U; i < view->zombie_count; ++i) {
        draw_asset(framebuffer, assets_zombie(view->zombies[i].state),
                   view->zombies[i].position);
    }
    for (size_t i = 0U; i < view->bullet_count; ++i) {
        draw_asset(framebuffer, assets_bullet(view->bullets[i].state),
                   view->bullets[i].position);
    }
    if (view->status == GAME_LOST) {
        draw_border(framebuffer, 0, 0, PVZ_SCREEN_WIDTH,
                    PVZ_SCREEN_HEIGHT, red);
    }
}

const framebuffer_t *renderer_get_framebuffer(const renderer_t *renderer)
{
    return renderer == NULL ? NULL : &renderer->framebuffer;
}
