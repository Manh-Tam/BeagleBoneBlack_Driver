#include "ui.h"

#include "../config.h"

static bool point_in_rectangle(touch_event_t event, int x, int y,
                               int width, int height)
{
    return event.x >= x && event.x < x + width &&
           event.y >= y && event.y < y + height;
}

bool ui_command_from_touch(touch_event_t event, game_status_t status,
                           game_command_t *command)
{
    uint16_t column;

    if (command == NULL || event.x >= PVZ_SCREEN_WIDTH ||
        event.y >= PVZ_SCREEN_HEIGHT) {
        return false;
    }

    if (status == GAME_LOST) {
        if (point_in_rectangle(event, PVZ_REPLAY_YES_X, PVZ_REPLAY_YES_Y,
                               PVZ_REPLAY_BUTTON_WIDTH,
                               PVZ_REPLAY_BUTTON_HEIGHT)) {
            command->type = GAME_COMMAND_RESTART;
            return true;
        }
        if (point_in_rectangle(event, PVZ_REPLAY_NO_X, PVZ_REPLAY_NO_Y,
                               PVZ_REPLAY_BUTTON_WIDTH,
                               PVZ_REPLAY_BUTTON_HEIGHT)) {
            command->type = GAME_COMMAND_STOP;
            return true;
        }
        return false;
    }
    if (status != GAME_RUNNING) {
        return false;
    }

    column = (uint16_t)(event.x / PVZ_TILE_WIDTH);
    if (event.y < PVZ_TILE_HEIGHT) {
        if (column == 0U) {
            command->type = GAME_COMMAND_SELECT_PLANT;
            command->data.plant_type = PLANT_SUNFLOWER;
        } else if (column == 1U) {
            command->type = GAME_COMMAND_SELECT_PLANT;
            command->data.plant_type = PLANT_PEASHOOTER;
        } else {
            command->type = GAME_COMMAND_CLEAR_SELECTION;
        }
        return true;
    }

    command->type = GAME_COMMAND_PLACE_PLANT;
    command->data.cell.row =
        (uint8_t)((event.y - PVZ_TILE_HEIGHT) / PVZ_TILE_HEIGHT);
    command->data.cell.column = (uint8_t)column;
    return command->data.cell.row < PVZ_GAME_ROWS &&
           command->data.cell.column < PVZ_GAME_COLUMNS;
}
