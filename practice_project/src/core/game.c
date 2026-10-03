#include "game.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    plant_type_t type;
    plant_state_t state;
    int health;
    uint32_t action_elapsed_ms;
} plant_slot_t;

typedef struct {
    bool active;
    zombie_state_t state;
    int32_t x;
    uint8_t row;
    int health;
    uint32_t animation_elapsed_ms;
} zombie_slot_t;

typedef struct {
    bool active;
    bullet_state_t state;
    int32_t x;
    uint8_t row;
    int damage;
} bullet_slot_t;

struct game {
    game_config_t config;
    plant_slot_t plants[PVZ_GAME_ROWS][PVZ_GAME_COLUMNS];
    zombie_slot_t zombies[PVZ_MAX_ZOMBIES];
    bullet_slot_t bullets[PVZ_MAX_BULLETS];
    plant_type_t selected_plant;
    int sun;
    uint32_t passive_sun_elapsed_ms;
    uint32_t spawn_elapsed_ms;
    uint32_t random_state;
    game_status_t status;
};

static bool valid_cell(grid_cell_t cell)
{
    return cell.row < PVZ_GAME_ROWS && cell.column < PVZ_GAME_COLUMNS;
}

static uint32_t random_next(game_t *game)
{
    uint32_t x = game->random_state;

    if (x == 0U) {
        x = UINT32_C(0x6d2b79f5);
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    game->random_state = x;
    return x;
}

static size_t active_zombie_count(const game_t *game)
{
    size_t count = 0U;

    for (size_t i = 0U; i < PVZ_MAX_ZOMBIES; ++i) {
        if (game->zombies[i].active) {
            ++count;
        }
    }
    return count;
}

static bool zombie_ahead_in_row(const game_t *game, uint8_t row, int32_t x)
{
    for (size_t i = 0U; i < PVZ_MAX_ZOMBIES; ++i) {
        const zombie_slot_t *zombie = &game->zombies[i];
        if (zombie->active && zombie->state != ZOMBIE_DEAD &&
            zombie->row == row && zombie->x >= x) {
            return true;
        }
    }
    return false;
}

static bool spawn_zombie(game_t *game)
{
    if (active_zombie_count(game) >= game->config.max_active_zombies) {
        return false;
    }

    for (size_t i = 0U; i < PVZ_MAX_ZOMBIES; ++i) {
        zombie_slot_t *zombie = &game->zombies[i];
        if (!zombie->active) {
            *zombie = (zombie_slot_t) {
                .active = true,
                .state = ZOMBIE_WALKING_1,
                .x = PVZ_SCREEN_WIDTH - PVZ_TILE_WIDTH,
                .row = (uint8_t)(random_next(game) % PVZ_GAME_ROWS),
                .health = game->config.zombie_health,
                .animation_elapsed_ms = 0U,
            };
            return true;
        }
    }
    return false;
}

static bool spawn_bullet(game_t *game, uint8_t row, uint8_t column)
{
    for (size_t i = 0U; i < PVZ_MAX_BULLETS; ++i) {
        bullet_slot_t *bullet = &game->bullets[i];
        if (!bullet->active) {
            *bullet = (bullet_slot_t) {
                .active = true,
                .state = BULLET_FLYING,
                .x = (int32_t)column * PVZ_TILE_WIDTH,
                .row = row,
                .damage = game->config.bullet_damage,
            };
            return true;
        }
    }
    return false;
}

static void update_resources(game_t *game, uint32_t step_ms)
{
    if (game->config.passive_sun_interval_ms == 0U) {
        return;
    }

    game->passive_sun_elapsed_ms += step_ms;
    while (game->passive_sun_elapsed_ms >= game->config.passive_sun_interval_ms) {
        game->passive_sun_elapsed_ms -= game->config.passive_sun_interval_ms;
        game->sun += game->config.passive_sun_amount;
    }
}

static void update_spawning(game_t *game, uint32_t step_ms)
{
    if (!game->config.spawning_enabled || game->config.zombie_spawn_interval_ms == 0U) {
        return;
    }

    game->spawn_elapsed_ms += step_ms;
    while (game->spawn_elapsed_ms >= game->config.zombie_spawn_interval_ms) {
        game->spawn_elapsed_ms -= game->config.zombie_spawn_interval_ms;
        (void)spawn_zombie(game);
    }
}

static void update_plants(game_t *game, uint32_t step_ms)
{
    for (uint8_t row = 0U; row < PVZ_GAME_ROWS; ++row) {
        for (uint8_t column = 0U; column < PVZ_GAME_COLUMNS; ++column) {
            plant_slot_t *plant = &game->plants[row][column];
            if (plant->type == PLANT_NONE) {
                continue;
            }
            if (plant->health <= 0) {
                memset(plant, 0, sizeof(*plant));
                continue;
            }

            plant->action_elapsed_ms += step_ms;
            if (plant->type == PLANT_SUNFLOWER) {
                plant->state = PLANT_RECOVERING;
                if (plant->action_elapsed_ms >= game->config.sunflower_interval_ms) {
                    plant->action_elapsed_ms %= game->config.sunflower_interval_ms;
                    game->sun += game->config.sunflower_sun_amount;
                    plant->state = PLANT_READY;
                }
            } else if (plant->type == PLANT_PEASHOOTER) {
                int32_t plant_x = (int32_t)column * PVZ_TILE_WIDTH;
                if (plant->state == PLANT_SHOOTING) {
                    plant->state = PLANT_RECOVERING;
                }
                if (plant->action_elapsed_ms >= game->config.peashooter_cooldown_ms) {
                    plant->state = PLANT_READY;
                    if (zombie_ahead_in_row(game, row, plant_x) &&
                        spawn_bullet(game, row, column)) {
                        plant->action_elapsed_ms = 0U;
                        plant->state = PLANT_SHOOTING;
                    }
                } else {
                    plant->state = PLANT_RECOVERING;
                }
            }
        }
    }
}

static plant_slot_t *colliding_plant(game_t *game, const zombie_slot_t *zombie,
                                     int32_t candidate_x)
{
    for (int column = PVZ_GAME_COLUMNS - 1; column >= 0; --column) {
        plant_slot_t *plant = &game->plants[zombie->row][column];
        int32_t plant_x = column * PVZ_TILE_WIDTH;
        if (plant->type != PLANT_NONE &&
            candidate_x < plant_x + PVZ_TILE_WIDTH &&
            candidate_x + PVZ_TILE_WIDTH > plant_x) {
            return plant;
        }
    }
    return NULL;
}

static void update_zombies(game_t *game, uint32_t step_ms)
{
    for (size_t i = 0U; i < PVZ_MAX_ZOMBIES; ++i) {
        zombie_slot_t *zombie = &game->zombies[i];
        if (!zombie->active) {
            continue;
        }
        if (zombie->state == ZOMBIE_DEAD) {
            zombie->active = false;
            continue;
        }

        int32_t candidate_x = zombie->x - game->config.zombie_speed_pixels_per_tick;
        plant_slot_t *plant = colliding_plant(game, zombie, candidate_x);
        if (plant != NULL) {
            zombie->state = ZOMBIE_EATING;
            plant->health -= game->config.zombie_damage;
        } else {
            zombie->x = candidate_x;
            zombie->animation_elapsed_ms += step_ms;
            if (zombie->state == ZOMBIE_HURT || zombie->state == ZOMBIE_EATING) {
                zombie->state = ZOMBIE_WALKING_1;
            } else if (zombie->animation_elapsed_ms >=
                       game->config.zombie_animation_interval_ms) {
                zombie->animation_elapsed_ms %=
                    game->config.zombie_animation_interval_ms;
                zombie->state = zombie->state == ZOMBIE_WALKING_1
                                    ? ZOMBIE_WALKING_2 : ZOMBIE_WALKING_1;
            }
        }

        if (zombie->x <= 0) {
            game->status = GAME_LOST;
        }
    }
}

static void update_bullets(game_t *game)
{
    for (size_t i = 0U; i < PVZ_MAX_BULLETS; ++i) {
        bullet_slot_t *bullet = &game->bullets[i];
        if (!bullet->active) {
            continue;
        }
        if (bullet->state == BULLET_EXPLODING) {
            bullet->active = false;
            continue;
        }

        int32_t old_x = bullet->x;
        bullet->x += game->config.bullet_speed_pixels_per_tick;
        for (size_t j = 0U; j < PVZ_MAX_ZOMBIES; ++j) {
            zombie_slot_t *zombie = &game->zombies[j];
            if (!zombie->active || zombie->state == ZOMBIE_DEAD ||
                zombie->row != bullet->row) {
                continue;
            }
            if (old_x <= zombie->x + PVZ_TILE_WIDTH && bullet->x >= zombie->x) {
                zombie->health -= bullet->damage;
                zombie->state = zombie->health <= 0 ? ZOMBIE_DEAD : ZOMBIE_HURT;
                bullet->state = BULLET_EXPLODING;
                break;
            }
        }
        if (bullet->x >= PVZ_SCREEN_WIDTH) {
            bullet->active = false;
        }
    }
}

game_config_t game_default_config(void)
{
    return (game_config_t) {
        .initial_sun = 50,
        .sunflower_price = 50,
        .peashooter_price = 100,
        .plant_health = 100,
        .zombie_health = 100,
        .zombie_damage = 20,
        .bullet_damage = 2,
        .zombie_speed_pixels_per_tick = 1,
        .bullet_speed_pixels_per_tick = 10,
        .passive_sun_amount = 20,
        .passive_sun_interval_ms = PVZ_GAME_TICK_MS,
        .sunflower_sun_amount = 5,
        .sunflower_interval_ms = 3U * PVZ_GAME_TICK_MS,
        .peashooter_cooldown_ms = 3U * PVZ_GAME_TICK_MS,
        .zombie_spawn_interval_ms = 100U * PVZ_GAME_TICK_MS,
        .zombie_animation_interval_ms = PVZ_GAME_TICK_MS,
        .random_seed = UINT32_C(0x12345678),
        .max_active_zombies = PVZ_MAX_ZOMBIES,
        .spawning_enabled = true,
    };
}

game_t *game_create(const game_config_t *config)
{
    game_config_t defaults = game_default_config();
    const game_config_t *chosen = config == NULL ? &defaults : config;
    game_t *game;

    if (chosen->max_active_zombies > PVZ_MAX_ZOMBIES ||
        chosen->sunflower_interval_ms == 0U ||
        chosen->peashooter_cooldown_ms == 0U ||
        chosen->zombie_animation_interval_ms == 0U) {
        return NULL;
    }

    game = calloc(1U, sizeof(*game));
    if (game == NULL) {
        return NULL;
    }
    game->config = *chosen;
    game->sun = chosen->initial_sun;
    game->selected_plant = PLANT_NONE;
    game->random_state = chosen->random_seed;
    game->status = GAME_RUNNING;
    if (chosen->spawning_enabled && chosen->zombie_spawn_interval_ms > 0U) {
        game->spawn_elapsed_ms = chosen->zombie_spawn_interval_ms;
    }
    return game;
}

void game_destroy(game_t *game)
{
    free(game);
}

game_result_t game_apply_command(game_t *game, const game_command_t *command)
{
    if (game == NULL || command == NULL) {
        return GAME_INVALID_ARGUMENT;
    }
    if (game->status != GAME_RUNNING && command->type != GAME_COMMAND_STOP) {
        return GAME_NOT_RUNNING;
    }

    switch (command->type) {
    case GAME_COMMAND_SELECT_PLANT:
        if (command->data.plant_type != PLANT_SUNFLOWER &&
            command->data.plant_type != PLANT_PEASHOOTER) {
            return GAME_INVALID_COMMAND;
        }
        game->selected_plant = command->data.plant_type;
        return GAME_OK;
    case GAME_COMMAND_CLEAR_SELECTION:
        game->selected_plant = PLANT_NONE;
        return GAME_OK;
    case GAME_COMMAND_STOP:
        game->status = GAME_STOPPED;
        return GAME_OK;
    case GAME_COMMAND_PLACE_PLANT: {
        grid_cell_t cell = command->data.cell;
        int cost;
        plant_slot_t *plant;
        if (!valid_cell(cell)) {
            return GAME_INVALID_CELL;
        }
        if (game->selected_plant == PLANT_NONE) {
            return GAME_INVALID_COMMAND;
        }
        plant = &game->plants[cell.row][cell.column];
        if (plant->type != PLANT_NONE) {
            return GAME_CELL_OCCUPIED;
        }
        cost = game->selected_plant == PLANT_SUNFLOWER
                   ? game->config.sunflower_price : game->config.peashooter_price;
        if (game->sun < cost) {
            return GAME_NOT_ENOUGH_SUN;
        }
        game->sun -= cost;
        *plant = (plant_slot_t) {
            .type = game->selected_plant,
            .state = PLANT_IDLE,
            .health = game->config.plant_health,
            .action_elapsed_ms = 0U,
        };
        return GAME_OK;
    }
    default:
        return GAME_INVALID_COMMAND;
    }
}

void game_update(game_t *game, uint32_t step_ms)
{
    if (game == NULL || game->status != GAME_RUNNING || step_ms == 0U) {
        return;
    }
    update_resources(game, step_ms);
    update_spawning(game, step_ms);
    update_plants(game, step_ms);
    update_zombies(game, step_ms);
    update_bullets(game);
}

game_status_t game_get_status(const game_t *game)
{
    return game == NULL ? GAME_STOPPED : game->status;
}

void game_build_render_view(const game_t *game, game_render_view_t *view)
{
    if (game == NULL || view == NULL) {
        return;
    }
    memset(view, 0, sizeof(*view));
    view->sun = game->sun;
    view->sunflower_price = game->config.sunflower_price;
    view->peashooter_price = game->config.peashooter_price;
    view->selected_plant = game->selected_plant;
    view->status = game->status;

    for (uint8_t row = 0U; row < PVZ_GAME_ROWS; ++row) {
        for (uint8_t column = 0U; column < PVZ_GAME_COLUMNS; ++column) {
            const plant_slot_t *plant = &game->plants[row][column];
            if (plant->type != PLANT_NONE && view->plant_count < PVZ_MAX_PLANTS) {
                render_plant_t *item = &view->plants[view->plant_count++];
                item->type = plant->type;
                item->state = plant->state;
                item->position.x = (int32_t)column * PVZ_TILE_WIDTH;
                item->position.y = (int32_t)(row + 1U) * PVZ_TILE_HEIGHT;
            }
        }
    }
    for (size_t i = 0U; i < PVZ_MAX_ZOMBIES; ++i) {
        const zombie_slot_t *zombie = &game->zombies[i];
        if (zombie->active && view->zombie_count < PVZ_MAX_ZOMBIES) {
            render_zombie_t *item = &view->zombies[view->zombie_count++];
            item->state = zombie->state;
            item->position.x = zombie->x;
            item->position.y = (int32_t)(zombie->row + 1U) * PVZ_TILE_HEIGHT;
        }
    }
    for (size_t i = 0U; i < PVZ_MAX_BULLETS; ++i) {
        const bullet_slot_t *bullet = &game->bullets[i];
        if (bullet->active && view->bullet_count < PVZ_MAX_BULLETS) {
            render_bullet_t *item = &view->bullets[view->bullet_count++];
            item->state = bullet->state;
            item->position.x = bullet->x;
            item->position.y = (int32_t)(bullet->row + 1U) * PVZ_TILE_HEIGHT;
        }
    }
}
