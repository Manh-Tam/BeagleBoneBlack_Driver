#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#define DEVICE            "/dev/myled"
#define MY_MAGIC        'L'
#define LED_ON          _IO(MY_MAGIC, 0)
#define LED_OFF         _IO(MY_MAGIC, 1)
#define LED_TOGGLE      _IO(MY_MAGIC, 2)

int main()
{
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0)
    {
        perror("open\n");
        return 1;
    }
    else
    {
        int ret = ioctl(fd, LED_TOGGLE);
        if (ret < 0)
        {
            perror("ioctl\n");
        }
    }
    close(fd);
    return 0;
}