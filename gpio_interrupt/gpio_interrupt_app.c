#include <stdio.h>
#include <stdlib.h>         // for EXIT_*
#include <fcntl.h>          // for open() + O_* flags
#include <unistd.h>         // for read(), write()
#include <errno.h>          // for perror()

#define MY_DEVICE       "/dev/gpio_interrupt"

int main(void)
{
    int fd = -1;
    ssize_t bytes_read = 0;
    char value = '\0';
    int ret = 0;
    fd = open(MY_DEVICE, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return EXIT_FAILURE;
    }
    for (;;)
    {
        bytes_read = read(fd, &value, sizeof(value));
        if (bytes_read < 0)
        {
            perror("read");
            break;
        }
        else if (bytes_read == 0)
        {
            printf("EOF\n");
            break;
        }
        else
        {
            printf("Button value: %d\n", value);
        }
    }
    ret = close(fd);
    if (ret != 0)
    {
        perror("close");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}