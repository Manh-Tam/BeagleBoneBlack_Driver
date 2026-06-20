#include <stdio.h>
#include <fcntl.h>          // for open() and O_* flags
#include <unistd.h>         // for read(), write(), close()
#include <errno.h>          // for perror()
#include <stdlib.h>         // for EXIT_*

#define BUTTON_DEVICE   "/dev/BUTTON"

int main(void)
{
    int fd = -1;
    ssize_t bytes_read = 0;
    char value = 0;
    int ret = 0;
    fd = open(BUTTON_DEVICE, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return EXIT_FAILURE;
    }
    for(;;)
    {
        /* check the button state for every one second */
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
            printf("Button value: %c\n", value);
        }
        sleep(1);
    }
    ret = close(fd);
    if (ret != 0)
    {
        perror("close");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}