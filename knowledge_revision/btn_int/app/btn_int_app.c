#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define DEVICE      "/dev/button"

int main()
{
    int fd;
    fd = open(DEVICE, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open\n");
        return EXIT_FAILURE;
    }

    while (1)
    {
        int value;
        int read_bytes = read(fd, &value, sizeof(value));
        if (read_bytes < 0)
        {
            perror("Failed to read\n");
            break;
        }
        else
        {
            printf("GPIO value: %d\n", value);
        }

    }
    if (close(fd) < 0)
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}