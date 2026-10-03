#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../config.h"
#include "../core/draw.h"
#include "../core/event_queue.h"
#include "../core/game.h"
#include "../core/touch.h"
#include "../core/ui.h"

static int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static game_config_t quiet_config(void)
{
    game_config_t config = game_default_config();
    config.passive_sun_interval_ms = 0U;
    config.spawning_enabled = false;
    return config;
}

static void select_plant(game_t *game, plant_type_t type)
{
    game_command_t command = {
        .type = GAME_COMMAND_SELECT_PLANT,
        .data.plant_type = type,
    };
    CHECK(game_apply_command(game, &command) == GAME_OK);
}

static game_result_t place_plant(game_t *game, uint8_t row, uint8_t column)
{
    game_command_t command = {
        .type = GAME_COMMAND_PLACE_PLANT,
        .data.cell = {row, column},
    };
    return game_apply_command(game, &command);
}

static void test_game_commands_and_resources(void)
{
    game_config_t config = quiet_config();
    game_render_view_t view;
    game_t *game = game_create(&config);

    CHECK(game != NULL);
    game_build_render_view(game, &view);
    CHECK(view.sun == 50);
    CHECK(view.plant_count == 0U);

    select_plant(game, PLANT_SUNFLOWER);
    CHECK(place_plant(game, 0U, 0U) == GAME_OK);
    CHECK(place_plant(game, 0U, 0U) == GAME_CELL_OCCUPIED);
    select_plant(game, PLANT_PEASHOOTER);
    CHECK(place_plant(game, 0U, 1U) == GAME_NOT_ENOUGH_SUN);
    CHECK(place_plant(game, PVZ_GAME_ROWS, 0U) == GAME_INVALID_CELL);

    game_update(game, config.sunflower_interval_ms);
    game_build_render_view(game, &view);
    CHECK(view.plant_count == 1U);
    CHECK(view.sun == config.sunflower_sun_amount);

    {
        game_command_t stop = {.type = GAME_COMMAND_STOP};
        CHECK(game_apply_command(game, &stop) == GAME_OK);
        CHECK(game_get_status(game) == GAME_STOPPED);
    }
    game_destroy(game);
}

static void test_spawning_capacity_and_loss(void)
{
    game_config_t config = quiet_config();
    game_render_view_t view;
    game_t *game;

    config.spawning_enabled = true;
    config.zombie_spawn_interval_ms = PVZ_GAME_TICK_MS;
    config.max_active_zombies = 1U;
    config.zombie_speed_pixels_per_tick = PVZ_TILE_WIDTH;
    game = game_create(&config);
    CHECK(game != NULL);

    for (int i = 0; i < 10 && game_get_status(game) == GAME_RUNNING; ++i) {
        game_update(game, PVZ_GAME_TICK_MS);
        game_build_render_view(game, &view);
        CHECK(view.zombie_count <= 1U);
    }
    CHECK(game_get_status(game) == GAME_LOST);
    game_destroy(game);
}

static void test_projectiles_and_pool_limit(void)
{
    game_config_t config = quiet_config();
    game_render_view_t view;
    game_t *game;
    uint8_t zombie_row;

    config.initial_sun = 1000;
    config.spawning_enabled = true;
    config.zombie_spawn_interval_ms = PVZ_GAME_TICK_MS;
    config.max_active_zombies = 1U;
    config.zombie_speed_pixels_per_tick = 0;
    config.bullet_damage = config.zombie_health;
    config.bullet_speed_pixels_per_tick = PVZ_TILE_WIDTH;
    config.peashooter_cooldown_ms = PVZ_GAME_TICK_MS;
    game = game_create(&config);
    CHECK(game != NULL);

    game_update(game, PVZ_GAME_TICK_MS);
    game_build_render_view(game, &view);
    CHECK(view.zombie_count == 1U);
    zombie_row = (uint8_t)(view.zombies[0].position.y / PVZ_TILE_HEIGHT - 1);
    select_plant(game, PLANT_PEASHOOTER);
    CHECK(place_plant(game, zombie_row, 0U) == GAME_OK);

    {
        bool observed_bullet = false;
        bool observed_dead_zombie = false;
        for (int i = 0; i < 10; ++i) {
            game_update(game, PVZ_GAME_TICK_MS);
            game_build_render_view(game, &view);
            observed_bullet = observed_bullet || view.bullet_count > 0U;
            if (view.zombie_count > 0U &&
                view.zombies[0].state == ZOMBIE_DEAD) {
                observed_dead_zombie = true;
                break;
            }
        }
        CHECK(observed_bullet);
        CHECK(observed_dead_zombie);
    }
    game_destroy(game);

    config.bullet_damage = 0;
    config.bullet_speed_pixels_per_tick = 0;
    config.zombie_health = 1000000;
    game = game_create(&config);
    CHECK(game != NULL);
    game_update(game, PVZ_GAME_TICK_MS);
    game_build_render_view(game, &view);
    zombie_row = (uint8_t)(view.zombies[0].position.y / PVZ_TILE_HEIGHT - 1);
    select_plant(game, PLANT_PEASHOOTER);
    CHECK(place_plant(game, zombie_row, 0U) == GAME_OK);
    for (size_t i = 0U; i < PVZ_MAX_BULLETS + 10U; ++i) {
        game_update(game, PVZ_GAME_TICK_MS);
    }
    game_build_render_view(game, &view);
    CHECK(view.bullet_count == PVZ_MAX_BULLETS);
    game_destroy(game);
}

static void test_zombie_plant_collision(void)
{
    game_config_t config = quiet_config();
    game_render_view_t view;
    game_t *game;
    uint8_t zombie_row;

    config.initial_sun = 1000;
    config.spawning_enabled = true;
    config.zombie_spawn_interval_ms = PVZ_GAME_TICK_MS;
    config.max_active_zombies = 1U;
    game = game_create(&config);
    CHECK(game != NULL);
    game_update(game, PVZ_GAME_TICK_MS);
    game_build_render_view(game, &view);
    zombie_row = (uint8_t)(view.zombies[0].position.y / PVZ_TILE_HEIGHT - 1);

    select_plant(game, PLANT_SUNFLOWER);
    CHECK(place_plant(game, zombie_row, 6U) == GAME_OK);
    for (int i = 0; i < 7; ++i) {
        game_update(game, PVZ_GAME_TICK_MS);
    }
    game_build_render_view(game, &view);
    CHECK(view.plant_count == 0U);
    game_destroy(game);
}

static void encode_raw(uint16_t raw_x, uint16_t raw_y, uint8_t raw[4])
{
    uint16_t encoded_x = (uint16_t)(raw_x << 3);
    uint16_t encoded_y = (uint16_t)(raw_y << 3);
    raw[0] = (uint8_t)(encoded_y >> 8);
    raw[1] = (uint8_t)encoded_y;
    raw[2] = (uint8_t)(encoded_x >> 8);
    raw[3] = (uint8_t)encoded_x;
}

static void test_touch_and_ui_mapping(void)
{
    uint8_t raw[4];
    touch_event_t event;
    game_command_t command;

    encode_raw(300U, 400U, raw);
    CHECK(touch_decode_raw(raw, &event));
    CHECK(event.x == 0U && event.y == 0U);
    CHECK(ui_command_from_touch(event, &command));
    CHECK(command.type == GAME_COMMAND_SELECT_PLANT);
    CHECK(command.data.plant_type == PLANT_SUNFLOWER);

    event = (touch_event_t){PVZ_TILE_WIDTH, 0U};
    CHECK(ui_command_from_touch(event, &command));
    CHECK(command.data.plant_type == PLANT_PEASHOOTER);

    event = (touch_event_t){100U, 20U};
    CHECK(ui_command_from_touch(event, &command));
    CHECK(command.type == GAME_COMMAND_CLEAR_SELECTION);

    encode_raw(3700U, 3700U, raw);
    CHECK(touch_decode_raw(raw, &event));
    CHECK(event.x == PVZ_SCREEN_WIDTH - 1U);
    CHECK(event.y == PVZ_SCREEN_HEIGHT - 1U);
    CHECK(ui_command_from_touch(event, &command));
    CHECK(command.type == GAME_COMMAND_PLACE_PLANT);
    CHECK(command.data.cell.row == PVZ_GAME_ROWS - 1U);
    CHECK(command.data.cell.column == PVZ_GAME_COLUMNS - 1U);
}

static void *queue_producer(void *argument)
{
    touch_queue_t *queue = argument;

    for (uint16_t i = 0U; i < PVZ_INPUT_QUEUE_CAPACITY; ++i) {
        CHECK(touch_queue_push(queue, (touch_event_t){i, (uint16_t)(i + 1U)}));
    }
    return NULL;
}

static void test_event_queue(void)
{
    touch_queue_t *queue = touch_queue_create();
    pthread_t producer;

    CHECK(queue != NULL);
    CHECK(pthread_create(&producer, NULL, queue_producer, queue) == 0);
    CHECK(pthread_join(producer, NULL) == 0);
    CHECK(touch_queue_size(queue) == PVZ_INPUT_QUEUE_CAPACITY);
    CHECK(!touch_queue_push(queue, (touch_event_t){999U, 999U}));
    CHECK(touch_queue_overflow_count(queue) == 1U);

    for (uint16_t i = 0U; i < PVZ_INPUT_QUEUE_CAPACITY; ++i) {
        touch_event_t event;
        CHECK(touch_queue_try_pop(queue, &event));
        CHECK(event.x == i);
        CHECK(event.y == i + 1U);
    }
    CHECK(touch_queue_size(queue) == 0U);
    touch_queue_destroy(queue);
}

static void test_framebuffer_clipping_and_format(void)
{
    uint8_t guarded[PVZ_FRAMEBUFFER_SIZE + 16U];
    uint8_t sprite[4] = {0xffU, 0xffU, 0x12U, 0x34U};
    framebuffer_t framebuffer;
    uint16_t color = draw_rgb555(255U, 0U, 0U);

    memset(guarded, 0xaa, sizeof(guarded));
    CHECK(framebuffer_bind(&framebuffer, guarded + 8U, PVZ_FRAMEBUFFER_SIZE,
                           PVZ_SCREEN_WIDTH, PVZ_SCREEN_HEIGHT,
                           PVZ_FRAMEBUFFER_STRIDE));
    draw_fill_rectangle(&framebuffer, -10, -10, 20, 20, color);
    CHECK(guarded[0] == 0xaaU);
    CHECK(guarded[7] == 0xaaU);
    CHECK(guarded[PVZ_FRAMEBUFFER_SIZE + 8U] == 0xaaU);
    CHECK(guarded[8] == (uint8_t)(color >> 8));
    CHECK(guarded[9] == (uint8_t)color);

    guarded[8] = 0x56U;
    guarded[9] = 0x78U;
    draw_sprite(&framebuffer, sprite, 2U, 1U, 0, 0);
    CHECK(guarded[8] == 0x56U && guarded[9] == 0x78U);
    CHECK(guarded[10] == 0x12U && guarded[11] == 0x34U);
    draw_digit(&framebuffer, PVZ_SCREEN_WIDTH - 4, PVZ_SCREEN_HEIGHT - 4,
               8, color);
    CHECK(guarded[PVZ_FRAMEBUFFER_SIZE + 8U] == 0xaaU);
}

int main(void)
{
    test_game_commands_and_resources();
    test_spawning_capacity_and_loss();
    test_projectiles_and_pool_limit();
    test_zombie_plant_collision();
    test_touch_and_ui_mapping();
    test_event_queue();
    test_framebuffer_clipping_and_format();

    if (failures != 0) {
        (void)fprintf(stderr, "%d test checks failed\n", failures);
        return 1;
    }
    (void)printf("all tests passed\n");
    return 0;
}
