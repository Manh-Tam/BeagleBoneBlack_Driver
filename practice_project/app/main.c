#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/zombie_head.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/zombie_head_attacked.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/bullet.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/move1.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/move2.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/move3.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/peashooter1.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/peashooter2.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/move4.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/shot1.h"
#include <stdint.h>
#include <stdlib.h>

struct pos
{
    int x;
    int y;
};

struct bullet
{
    struct pos pos;
    int speed;
};

struct bullet* bullet_list[100] = {NULL};

struct zombie
{
    struct pos pos;
    int speed;
    int health;
};

struct peashooter
{
    struct pos pos;
    int attack_speed;
    int timeout;
};

typedef enum 
{
    SUN_FLOWER,
    PEASHOOTER,
}plant_type;

struct plant
{
    plant_type type;
    void *plant;
};

struct plant plants[32] = {0};

/*diplay size in bytes*/
#define DISPLAY_SIZE    (320*240*2)

uint8_t display_ram[DISPLAY_SIZE];

void draw_sprite(const uint8_t *sprite, int start_x, int start_y, int width, int height)
{
    if ((start_x >= 320) || (start_y >= 240))
        return;
    int bpp = 2;
    for (int i = 0; i < height; i++)
    {
        uint8_t image[width * bpp];
        for (int j = 0; j < width * bpp; j++)
        {
            if (sprite[j + i * width * bpp] == 0xff)
                image[j] = 0x8E;
            else
                image[j] = sprite[j + i * width * bpp];
        }
        // memcpy(display_ram + bpp * start_x + 320 * bpp * (i + start_y), sprite + width * bpp * i, width * bpp);
        memcpy(display_ram + bpp * start_x + 320 * bpp * (i + start_y), image, width * bpp);
    }
}

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
    printf("size of array: %d\n", sizeof(image_zombie_head) / sizeof(image_zombie_head[0]));
    // memset(image_zombie_head, 0, 100);
    // write(fd, image_zombie_head, 3200);

    struct bullet *bullet;
    bullet = malloc(sizeof(*bullet));
    bullet->pos.x = 0;
    bullet->pos.y = 0;
    bullet->speed = 1;

    struct zombie *zombie;
    zombie = malloc(sizeof(*zombie));
    zombie->pos.x = 280;
    zombie->pos.y = 0;
    zombie->speed = 10;
    zombie->health = 50;

    struct peashooter *peashooter;
    peashooter = malloc(sizeof(*peashooter));
    peashooter->pos.x = 0;
    peashooter->pos.y = 0;
    peashooter->attack_speed = 3;
    peashooter->timeout = peashooter->attack_speed;

    for (;;)
    {
        /*1. Update the grass */
        memset(display_ram, 0x8E, DISPLAY_SIZE);
        /*2. prepare display RAM */

        if (peashooter->timeout == 0)
        {
            /*1. create a bullet */
            struct bullet *bullet = malloc(sizeof(struct bullet));
            bullet->pos.x = 40;
            bullet->pos.y = 0;
            bullet->speed = 40;
            /*2. push it into the bullet list */
            for (int i = 0; i < 100; i++)
            {
                if (bullet_list[i] == NULL) 
                {
                    bullet_list[i] = bullet;
                    printf("I'm here\n");
                    break;
                }
            }

            /*3. reload the timeout*/
            peashooter->timeout = peashooter->attack_speed;
        }
        
        // draw_sprite(image_move1, zombie->pos.x, zombie->pos.y, 40, 40);
        // if (bullet->pos.x + 40 == zombie->pos.x)
        // {
        //     draw_sprite(image_move3, zombie->pos.x, zombie->pos.y, 40, 40);
        //     write(fd, display_ram, DISPLAY_SIZE);
        //     break;
        // }
        if (peashooter->timeout == 1)
            draw_sprite(image_peashooter2, 0, 0, 40, 40);
        else
            draw_sprite(image_peashooter1, 0, 0, 40, 40);

        if (zombie != NULL)
        {
            draw_sprite(image_move1, zombie->pos.x, 0, 40, 40);
        }
        
        /* prepare the bullet */
        for (int i = 0; i < 100; i++)
        {
            if (bullet_list[i] != NULL)
            {
                /*handle collisions*/
                /*if the bullet coordinate reaches the zomebie coordinate*/
                /*calculate zombie health*/
                /*delete the bullet*/
                if (bullet_list[i]->pos.x  + 40 >= zombie->pos.x)
                {
                    /*decreate zombie health*/
                    zombie->health -= 10;
                    /*draw animation*/
                    if (zombie->health <= 0)
                    {
                        draw_sprite(image_move4, zombie->pos.x, 0, 40, 40);
                        write(fd, display_ram, DISPLAY_SIZE);
                        free(zombie);
                        zombie = NULL;
                        return 0;
                    }
                    draw_sprite(image_move3, zombie->pos.x, 0, 40, 40);
                    free(bullet_list[i]);
                    bullet_list[i] = NULL;
                }
                else
                {
                    draw_sprite(image_shot1, bullet_list[i]->pos.x, bullet_list[i]->pos.y, 40, 40);
                    bullet_list[i]->pos.x += bullet_list[i]->speed;
                }
            }
        }
        /*3. flush display RAM into the Kernel */
        write(fd, display_ram, DISPLAY_SIZE);
        peashooter->timeout--;
        zombie->pos.x -= zombie->speed;

        sleep(0.1);
    }

    close(fd);
    return 0;
}