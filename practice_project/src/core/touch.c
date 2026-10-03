#include "touch.h"

#include "../config.h"

static uint16_t scale_axis(uint16_t raw, uint16_t minimum, uint16_t maximum,
                           uint16_t screen_maximum)
{
    if (raw < minimum) {
        raw = minimum;
    } else if (raw > maximum) {
        raw = maximum;
    }
    return (uint16_t)(((uint32_t)(raw - minimum) * screen_maximum) /
                      (uint32_t)(maximum - minimum));
}

bool touch_decode_raw(const uint8_t raw[4], touch_event_t *event)
{
    uint16_t raw_x;
    uint16_t raw_y;

    if (raw == NULL || event == NULL) {
        return false;
    }

    raw_y = (uint16_t)(((uint16_t)raw[0] << 8) | raw[1]);
    raw_x = (uint16_t)(((uint16_t)raw[2] << 8) | raw[3]);
    raw_x = (uint16_t)((raw_x >> 3) & UINT16_C(0x0fff));
    raw_y = (uint16_t)((raw_y >> 3) & UINT16_C(0x0fff));

    event->x = scale_axis(raw_x, 300U, 3700U, PVZ_SCREEN_WIDTH - 1U);
    event->y = scale_axis(raw_y, 400U, 3700U, PVZ_SCREEN_HEIGHT - 1U);
    return true;
}
