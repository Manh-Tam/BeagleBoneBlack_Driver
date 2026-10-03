#ifndef PVZ_UI_H
#define PVZ_UI_H

#include <stdbool.h>

#include "game.h"
#include "pos.h"

bool ui_command_from_touch(touch_event_t event, game_command_t *command);

#endif
