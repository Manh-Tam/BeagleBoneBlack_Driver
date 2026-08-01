#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define DEVICE      "/dev/button"

int main()
{
    int fd;
    int value;
    fd = open(DEVICE, O_RDONLY);
    if (fd < 0)
    {
        perror("Failed to open device\n");
        return EXIT_FAILURE;
    }
    while(1)
    {
        int read_bytes = read(fd, &value, sizeof(value));
        if (read_bytes < 0)
        {
            perror("Failed to read device\n");
        }
        else
        {
            printf("GPIO value: %d\n", value);
        }
        sleep(0.5);
    }

    if (close(fd))
    {
        perror("Failed to close device\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
