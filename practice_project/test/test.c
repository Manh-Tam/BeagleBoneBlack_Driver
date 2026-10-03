#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define TOUCH_DEVICE      "/dev/touch"

void touch_data_to_coordinate(const uint8_t *touch_data, uint16_t *x, uint16_t *y)
{
    /*calculate x pos*/
    *x = (touch_data[2] << 8) | touch_data[3];
    *x = (*x >> 3) & 0xfff;
    if (*x < 300)
        *x = 300;
    if (*x > 3700)
        *x = 3700;
    *x = (*x - 300) * 100 / (3700 - 300);
    *x = *x * (float)3.1;

    /*calculate y pos*/
    *y = (touch_data[0] << 8) | touch_data[1];
    *y = (*y >> 3) & 0xfff;
    if (*y < 400)
        *y = 400;
    if (*y > 3700)
        *y = 3700;
    *y = (*y - 400) * 100 / (3700 - 400);
    *y = *y * (float)2.3;
}

int main()
{
    int ret = 0;
    int fd = open(TOUCH_DEVICE, O_RDWR);
    if (fd < 0)
    {
        printf("Failed to open touch device\n");
        return fd;
    }
    while (1)
    {
        uint8_t data[4];
        ret = read(fd, data, sizeof(data));
        if (ret < 0)
        {
            printf("Failed to read\n");
            return ret;
        }
        else
        {
            uint16_t x, y;

            touch_data_to_coordinate(data, &x, &y);
            printf("Number of read bytes: %d\n", ret);
            printf("%u %u\n", x, y);
        }
        usleep(500000);
    }
    return 0;
}