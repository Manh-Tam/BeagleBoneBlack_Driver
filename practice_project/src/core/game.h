#ifndef PVZ_GAME_H
#define PVZ_GAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../config.h"
#include "pos.h"

typedef struct game game_t;
typedef enum { PLANT_NONE = 0, PLANT_SUNFLOWER, PLANT_PEASHOOTER } plant_type_t;
typedef enum { PLANT_IDLE = 0, PLANT_RECOVERING, PLANT_READY, PLANT_SHOOTING } plant_state_t;
typedef enum {
    ZOMBIE_WALKING_1 = 0, ZOMBIE_WALKING_2, ZOMBIE_HURT,
    ZOMBIE_EATING, ZOMBIE_DEAD
} zombie_state_t;
typedef enum { BULLET_FLYING = 0, BULLET_EXPLODING } bullet_state_t;
typedef enum { GAME_RUNNING = 0, GAME_LOST, GAME_STOPPED } game_status_t;
typedef enum {
    GAME_OK = 0, GAME_INVALID_ARGUMENT, GAME_INVALID_COMMAND,
    GAME_INVALID_CELL, GAME_CELL_OCCUPIED, GAME_NOT_ENOUGH_SUN,
    GAME_NOT_RUNNING
} game_result_t;
typedef enum {
    GAME_COMMAND_SELECT_PLANT = 0, GAME_COMMAND_PLACE_PLANT,
    GAME_COMMAND_CLEAR_SELECTION, GAME_COMMAND_RESTART, GAME_COMMAND_STOP
} game_command_type_t;

typedef struct {
    game_command_type_t type;
    union { plant_type_t plant_type; grid_cell_t cell; } data;
} game_command_t;

typedef struct {
    int initial_sun;
    int sunflower_price;
    int peashooter_price;
    int plant_health;
    int zombie_health;
    int zombie_damage;
    int bullet_damage;
    int zombie_speed_pixels_per_tick;
    int bullet_speed_pixels_per_tick;
    int passive_sun_amount;
    uint32_t passive_sun_interval_ms;
    int sunflower_sun_amount;
    uint32_t sunflower_interval_ms;
    uint32_t peashooter_cooldown_ms;
    uint32_t zombie_spawn_interval_ms;
    uint32_t zombie_animation_interval_ms;
    uint32_t random_seed;
    size_t max_active_zombies;
    bool spawning_enabled;
} game_config_t;

typedef struct { plant_type_t type; plant_state_t state; screen_point_t position; } render_plant_t;
typedef struct { zombie_state_t state; screen_point_t position; } render_zombie_t;
typedef struct { bullet_state_t state; screen_point_t position; } render_bullet_t;

typedef struct {
    int sun;
    int sunflower_price;
    int peashooter_price;
    plant_type_t selected_plant;
    game_status_t status;
    render_plant_t plants[PVZ_MAX_PLANTS];
    size_t plant_count;
    render_zombie_t zombies[PVZ_MAX_ZOMBIES];
    size_t zombie_count;
    render_bullet_t bullets[PVZ_MAX_BULLETS];
    size_t bullet_count;
} game_render_view_t;

game_config_t game_default_config(void);
game_t *game_create(const game_config_t *config);
void game_destroy(game_t *game);
game_result_t game_apply_command(game_t *game, const game_command_t *command);
void game_update(game_t *game, uint32_t step_ms);
game_status_t game_get_status(const game_t *game);
void game_build_render_view(const game_t *game, game_render_view_t *view);

#endif
