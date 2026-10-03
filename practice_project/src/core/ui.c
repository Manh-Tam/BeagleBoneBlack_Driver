#include "ui.h"

#include "../config.h"

bool ui_command_from_touch(touch_event_t event, game_command_t *command)
{
    uint16_t column;

    if (command == NULL || event.x >= PVZ_SCREEN_WIDTH ||
        event.y >= PVZ_SCREEN_HEIGHT) {
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
