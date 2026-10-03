#ifndef PVZ_UI_H
#define PVZ_UI_H

#include <stdbool.h>

#include "game.h"
#include "pos.h"

#define PVZ_REPLAY_DIALOG_X 40
#define PVZ_REPLAY_DIALOG_Y 70
#define PVZ_REPLAY_DIALOG_WIDTH 240
#define PVZ_REPLAY_DIALOG_HEIGHT 110
#define PVZ_REPLAY_YES_X 60
#define PVZ_REPLAY_YES_Y 130
#define PVZ_REPLAY_NO_X 180
#define PVZ_REPLAY_NO_Y 130
#define PVZ_REPLAY_BUTTON_WIDTH 80
#define PVZ_REPLAY_BUTTON_HEIGHT 32

bool ui_command_from_touch(touch_event_t event, game_status_t status,
                           game_command_t *command);

#endif
