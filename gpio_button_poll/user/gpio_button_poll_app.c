#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#define MY_DEVICE       "/dev/gpio_button_poll"

int main()
{
    int fd;
    int ret;
    char value;
    struct pollfd pfd;

    fd = open(MY_DEVICE, O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return EXIT_FAILURE;
    }
    pfd.fd = fd;
    pfd.events = POLLIN;

    for (;;)
    {
        printf("Waiting for button ...\n");

        ret = poll(&pfd, 1, -1);
        if (ret < 0)
        {
            perror("poll");
            break;
        }

        if ((pfd.revents & POLLIN) != 0)
        {
            ret = read(fd, &value, sizeof(value));
            if (ret < 0)
            {
                perror("read\n");
                break;
            }
            else
            {
                printf("Button value: %c\n", value);
            }
        }

    }
    if (close(fd))
    {
        perror("close\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}