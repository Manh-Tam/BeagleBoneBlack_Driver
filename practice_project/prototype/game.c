#include "game.h"
#include "plant.h"
#include "zombie.h"

#define GAME_ROWS      5
#define GAME_COLUMNS   8
#define MAX_ZOMBIES      100
#define MAX_BULLETS      100
#define SCREEN_WIDTH     320
#define SCREEN_HEIGHT   240
#define BYTE_PER_PIXEL  2
#define BUFFER_SIZE     (SCREEN_WITH * SCREEN_HEIGHT * BYTE_PER_PIXEL)

struct game {
    struct plant plants[GAME_ROWS][GAME_COLUMNS];
    struct zombie zombies[MAX_ZOMBIES];
    struct bullet bullets[MAX_BULLETS];

    plant_type_t selected_plant;
    int sun;

    uint32_t zombie_spawn_timer_ms;
    uint64_t elapsed_ms;

    game_status_t status;
};

game_t *game_create(void);
void game_destroy(game_t *game)
{

}

game_result_t game_apply_command(
    game_t *game,
    const struct game_command *command)
{

}

void game_update(game_t *game, uint32_t elapsed_ms)
{

}

game_status_t game_get_status(const game_t *game)
{

}

void renderer_render(
    renderer_t *renderer,
    const game_t *game)
{

}

int display_present(
    display_t *display,
    const framebuffer_t *framebuffer)
{
    
}