#ifndef PVZ_POS_H
#define PVZ_POS_H

#include <stdint.h>

typedef struct { int32_t x; int32_t y; } screen_point_t;
typedef struct { uint8_t row; uint8_t column; } grid_cell_t;
typedef struct { uint16_t x; uint16_t y; } touch_event_t;

#endif
