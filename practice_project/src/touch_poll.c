#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <poll.h>

#define DEVICE  "/dev/touch"

int main()
{
    int fd;
    int ret;
    struct pollfd pfd;
    uint8_t rx[4];

    fd = open(DEVICE, O_RDWR);
    if (fd < 0)
    {
        perror("open failed\n");
        return -1;
    }
    pfd.fd = fd;
    pfd.events = POLLIN;
    uint32_t cnt = 0;
    for (;;)
    {
        printf("Waiting for a touch : %d\n", cnt++);
        ret = poll(&pfd, 1, -1); //sleep for 10ms
        if (ret < 0)
        {
            break;
        }
        if ((pfd.revents & POLLIN) != 0)
        {
            ret = read(fd, &rx, sizeof(rx));
            if (ret < 0)
            {
                perror("read\n");
                break;
            }
            else
            {
                // uint16_t x = (value >> 3) & 0xfff;
                // x = (x * 100)/4096;
                uint16_t x = (rx[0] << 8) | rx[1];
                // printf("Touch value hex: %x\n", x);
                x = (x >> 3) & 0xfff;
                printf("x coordinate: %d\n", x);

                uint16_t y = (rx[2] << 8) | rx[3];
                y = (y >> 3) & 0xfff;
                printf("y coordinate: %d\n", y);
            }
        }
        sleep(0.1);
    }
    close(fd);
    return 0;
}