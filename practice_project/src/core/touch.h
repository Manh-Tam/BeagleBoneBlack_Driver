#ifndef PVZ_TOUCH_H
#define PVZ_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

#include "pos.h"

bool touch_decode_raw(const uint8_t raw[4], touch_event_t *event);

#endif
