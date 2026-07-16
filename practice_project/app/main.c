#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/zombie_head.h"

int main()
{
    int fd = open("/dev/st7789", O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open file\n");
        return fd;
    }
    printf("Hello Wolrd\n");
    for (int i = 0; i < 10; i++)
    {
        printf("0x%u ", image_zombie_head[i]);
    }
    printf("\n");
    printf("size of array: %d", sizeof(image_zombie_head) / sizeof(image_zombie_head[0]));
    write(fd, image_zombie_head, 3200);
    close(fd);
    return 0;
}