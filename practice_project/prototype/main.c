#include "game.h"

int main()
{
    game_t *game = game_create();
    renderer_t *renderer = renderer_create(320, 240);
    display_t *display = display_open("/dev/lcd");
    input_t *input = input_open("/dev/touch");

    
    while (game_get_status(game) == GAME_RUNNING) {
        game_command_t command;

        if (input_read_command(&input, &command, 50) > 0) {
            game_apply_command(game, &command);
        }

        game_update(game, 50);
        renderer_render(&renderer, game);
        display_present(&display, renderer_framebuffer(&renderer));
    }

    game_destroy(game);
}