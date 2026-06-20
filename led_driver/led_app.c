#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MY_MAGIC        'L'
#define LED_ON          _IO(MY_MAGIC, 0)
#define LED_OFF         _IO(MY_MAGIC, 1)
#define LED_TOGGLE      _IO(MY_MAGIC, 2)

int main(void)
{
    int fd;
    fd = open("/dev/myled", O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    int ret = ioctl(fd, LED_TOGGLE);
    if (ret < 0)
    {
        perror("ioctl");
    }
    close(fd);
    return 0;
}
