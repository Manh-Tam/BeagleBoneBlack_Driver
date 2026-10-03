#ifndef PVZ_GAME_H
#define PVZ_GAME_H

#include <stdbool.h>
#include <stdint.h>

typedef struct game game_t;

typedef enum {
    GAME_RUNNING,
    GAME_WON,
    GAME_LOST,
    GAME_STOPPED
} game_status_t;

typedef enum {
    GAME_OK,
    GAME_INVALID_ARGUMENT,
    GAME_INVALID_POSITION,
    GAME_CELL_OCCUPIED,
    GAME_NOT_ENOUGH_SUN
} game_result_t;

game_t *game_create(void);
void game_destroy(game_t *game);

game_result_t game_apply_command(
    game_t *game,
    const struct game_command *command);

void game_update(game_t *game, uint32_t elapsed_ms);

game_status_t game_get_status(const game_t *game);

void renderer_render(
    renderer_t *renderer,
    const game_t *game);

int display_present(
    display_t *display,
    const framebuffer_t *framebuffer);

#endif