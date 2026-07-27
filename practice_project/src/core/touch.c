#include "touch.h"

void touch_data_to_coordinate(const struct touch_data *touch_data, uint16_t *x, uint16_t *y)
{
    /*calculate x pos*/
    *x = (touch_data->x[0] << 8) | touch_data->x[1];
    *x = (*x >> 3) & 0xfff;
    if (*x < 300)
        *x = 300;
    if (*x > 3700)
        *x = 3700;
    *x = (*x - 300) * 100 / (3700 - 300);
    *x = *x * (float)3.1;

    /*calculate y pos*/
    *y = (touch_data->y[0] << 8) | touch_data->y[1];
    *y = (*y >> 3) & 0xfff;
    if (*y < 400)
        *y = 400;
    if (*y > 3700)
        *y = 3700;
    *y = (*y - 400) * 100 / (3700 - 400);
    *y = *y * (float)2.3;
}