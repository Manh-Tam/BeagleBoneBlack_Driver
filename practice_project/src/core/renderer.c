#include "renderer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../config.h"
#include "assets.h"
#include "digit_bitmaps.h"
#include "ui.h"

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

static void copy_background(framebuffer_t *framebuffer)
{
    const asset_image_t *background = assets_background();
    size_t background_size = (size_t)background->width * background->height *
                             PVZ_BYTES_PER_PIXEL;

    if (background->width == framebuffer->width &&
        background->height == framebuffer->height &&
        background_size == framebuffer->size) {
        memcpy(framebuffer->pixels, background->pixels, background_size);
    } else {
        draw_fill_rectangle(framebuffer, 0, 0, framebuffer->width,
                            framebuffer->height, draw_rgb555(0U, 0U, 0U));
    }
}

static void draw_replay_dialog(framebuffer_t *framebuffer)
{
    uint16_t black = draw_rgb555(0U, 0U, 0U);
    uint16_t white = draw_rgb555(255U, 255U, 255U);
    uint16_t yellow = draw_rgb555(255U, 255U, 0U);
    uint16_t green = draw_rgb555(35U, 210U, 70U);
    uint16_t red = draw_rgb555(220U, 45U, 45U);

    draw_fill_rectangle(framebuffer, PVZ_REPLAY_DIALOG_X,
                        PVZ_REPLAY_DIALOG_Y, PVZ_REPLAY_DIALOG_WIDTH,
                        PVZ_REPLAY_DIALOG_HEIGHT, black);
    draw_border(framebuffer, PVZ_REPLAY_DIALOG_X, PVZ_REPLAY_DIALOG_Y,
                PVZ_REPLAY_DIALOG_WIDTH, PVZ_REPLAY_DIALOG_HEIGHT, white);
    draw_text(framebuffer, 94, 90, "PLAY AGAIN?", yellow, 2U);

    draw_fill_rectangle(framebuffer, PVZ_REPLAY_YES_X, PVZ_REPLAY_YES_Y,
                        PVZ_REPLAY_BUTTON_WIDTH, PVZ_REPLAY_BUTTON_HEIGHT,
                        green);
    draw_fill_rectangle(framebuffer, PVZ_REPLAY_NO_X, PVZ_REPLAY_NO_Y,
                        PVZ_REPLAY_BUTTON_WIDTH, PVZ_REPLAY_BUTTON_HEIGHT,
                        red);
    draw_border(framebuffer, PVZ_REPLAY_YES_X, PVZ_REPLAY_YES_Y,
                PVZ_REPLAY_BUTTON_WIDTH, PVZ_REPLAY_BUTTON_HEIGHT, white);
    draw_border(framebuffer, PVZ_REPLAY_NO_X, PVZ_REPLAY_NO_Y,
                PVZ_REPLAY_BUTTON_WIDTH, PVZ_REPLAY_BUTTON_HEIGHT, white);
    draw_text(framebuffer, 82, 139, "YES", black, 2U);
    draw_text(framebuffer, 208, 139, "NO", white, 2U);
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
    copy_background(framebuffer);

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
        draw_replay_dialog(framebuffer);
    }
}

const framebuffer_t *renderer_get_framebuffer(const renderer_t *renderer)
{
    return renderer == NULL ? NULL : &renderer->framebuffer;
}
