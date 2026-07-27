#ifndef __TOUCH_H_
#define __TOUCH_H_
#include <stdint.h>
#include <stdio.h>

struct touch_data
{
    uint8_t y[2];
    uint8_t x[2];
};

void touch_data_to_coordinate(const struct touch_data *touch_data, uint16_t *x, uint16_t *y);

#endif