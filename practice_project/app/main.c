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
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/map.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

struct pos
{
    int x;
    int y;
};

struct bullet
{
    struct pos pos;
    int speed;
    int damage;
};

struct bullet* bullet_list[100] = {NULL};

typedef enum
{
    WALK_1,
    WALK_2,
    WALK_3,
    DIE,
}zombie_state;

struct zombie
{
    struct pos pos;
    int speed;
    int health;
    zombie_state state;
};

struct peashooter
{
    struct pos pos;
    int attack_speed;
    int timeout;
    int health;
};

typedef enum 
{
    TYPE_NONE,
    SUN_FLOWER,
    PEASHOOTER,
}plant_type;

struct plant
{
    plant_type type;
    void *plant;
};

struct plant plants[5][8] = {0};
struct zombie* zombies[100] = {NULL};

/*diplay size in bytes*/
#define DISPLAY_SIZE    (320*240*2)

static uint8_t display_ram[DISPLAY_SIZE];

#define SCREEN_W 320
#define SCREEN_H 240
#define TILE_W   40
#define TILE_H   40

void draw_rectangle_u8(int start_x, int start_y, int w, int h, uint16_t color) {
    /* Split the 16-bit color into Big-Endian bytes */
    uint8_t hi = (color >> 8) & 0xFF;
    uint8_t lo = color & 0xFF;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int tx = start_x + x;
            int ty = start_y + y;
            
            if (tx < SCREEN_W && ty < SCREEN_H && tx >= 0 && ty >= 0) {
                /* Calculate the byte offset: 2 bytes per pixel */
                int pixel_index = (ty * SCREEN_W + tx) * 2;
                
                display_ram[pixel_index]     = hi; /* First byte */
                display_ram[pixel_index + 1] = lo; /* Second byte */
            }
        }
    }
}

void render_frame(void) {
    /* 1. Draw Menu Row 0 */
    for (int col = 0; col < 8; col++) {
        draw_rectangle_u8(col * TILE_W, 0, TILE_W, TILE_H, 0x72A4);
    }

    /* 2. Draw Lawn Rows 1 to 5 */
    for (int row = 1; row < 6; row++) {
        for (int col = 0; col < 8; col++) {
            int x = col * TILE_W;
            int y = row * TILE_H;
            
            if ((row + col) % 2 == 0) {
                draw_rectangle_u8(x, y, TILE_W, TILE_H, 0x5E68); // Light Green
            } else {
                draw_rectangle_u8(x, y, TILE_W, TILE_H, 0x3DC4); // Dark Green
            }
        }
    }

}

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
                image[j] = display_ram[bpp * start_x + 320 * bpp * (i + start_y) + j];
            else
                image[j] = sprite[j + i * width * bpp];
        }
        // memcpy(display_ram + bpp * start_x + 320 * bpp * (i + start_y), sprite + width * bpp * i, width * bpp);
        memcpy(display_ram + bpp * start_x + 320 * bpp * (i + start_y), image, width * bpp);
    }
}

int main()
{
    srand(time(NULL)); // Seed RNG
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

    for (int i = 0; i < 5; i++)
    {
        struct peashooter* peashooter = malloc(sizeof(struct peashooter));
        peashooter->pos.x = 0;
        peashooter->pos.y = (i+1) * 40;
        peashooter->attack_speed = 1;
        peashooter->health = 10;
        peashooter->timeout = 0;
        plants[i][0].type = PEASHOOTER;
        plants[i][0].plant = (void*)peashooter;
    }
    
    for (;;)
    {
        /*1. Update the grass */
        // memset(display_ram, 0x8E, DISPLAY_SIZE);
        // memcpy(display_ram, image_map, DISPLAY_SIZE);
        render_frame();
        /*2. prepare display RAM */
        int num = 1 + rand() % 5; //random number between 1 and 6

        //generate random zombie
        struct zombie* zombie = malloc(sizeof(struct zombie));
        zombie->pos.x = 280;
        zombie->pos.y = num*40;
        zombie->health = 100;
        zombie->state = WALK_1;
        zombie->speed = 10;
        //add zombie to zombie list
        for (int i = 0; i < 3; i++)
        {
            if (zombies[i] == NULL)
            {
                printf("I'm here\n");
                zombies[i] = zombie;
                break;
            }
        }

        /* update list of zombie */
        for (int i = 0; i < 100; i++)
        {
            if (zombies[i] != NULL)
            {
                zombies[i]->pos.x -= zombies[i]->speed;
                printf("zombies[i]->pos.x: %d\n", zombies[i]->pos.x);
                if (zombies[i]->state == DIE)
                {
                    free(zombies[i]);
                    zombies[i] = NULL;
                }
                else if (zombies[i]->state == WALK_1)
                    zombies[i]->state = WALK_2;
                else 
                    zombies[i]->state = WALK_1;
            }
        }

        /* update list of peashooter*/
        for (int i = 0; i < 5; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                if (plants[i][j].type == PEASHOOTER)
                {
                    struct peashooter *peashooter = (struct peashooter*)plants[i][j].plant;
                    if (peashooter->timeout == 0)
                    {
                        peashooter->timeout = peashooter->attack_speed;
                        /* generate new bullet*/
                        struct bullet* bullet = malloc(sizeof(struct bullet));
                        bullet->pos.x = peashooter->pos.x + 40;
                        bullet->pos.y = peashooter->pos.y;
                        bullet->speed = 10;
                        bullet->damage = 10;
                        
                        for (int i = 0; i < 100; i++)
                        {
                            if (bullet_list[i] == NULL)
                            {
                                printf("degguubb\n");
                                bullet_list[i] = bullet;
                                break;
                            }
                        }

                    }
                    else
                        peashooter->timeout--;
                }
            }
        }

        /*update list of bullet*/
        for (int i = 0; i < 100; i++)
        {
            if (bullet_list[i] != NULL)
            {
                bullet_list[i]->pos.x += bullet_list[i]->speed;
                if (bullet_list[i]->pos.x >= 300)
                {
                    free(bullet_list[i]);
                    bullet_list[i] = NULL;
                }
                else
                {
                    for (int j = 0; j < 100; j++)
                    {
                        if (zombies[j] != NULL)
                        {
                            if (zombies[j]->pos.y == bullet_list[i]->pos.y)
                            {
                                if (bullet_list[i]->pos.x >= zombies[j]->pos.x)
                                {
                                    zombies[j]->state = DIE;
                                    free(bullet_list[i]);
                                    bullet_list[i] = NULL;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        // if (peashooter->timeout == 0)
        // {
        //     /*1. create a bullet */
        //     struct bullet *bullet = malloc(sizeof(struct bullet));
        //     bullet->pos.x = 40;
        //     bullet->pos.y = 0;
        //     bullet->speed = 40;
        //     /*2. push it into the bullet list */
        //     for (int i = 0; i < 100; i++)
        //     {
        //         if (bullet_list[i] == NULL) 
        //         {
        //             bullet_list[i] = bullet;
        //             printf("I'm here\n");
        //             break;
        //         }
        //     }

        //     /*3. reload the timeout*/
        //     peashooter->timeout = peashooter->attack_speed;
        // }
        
        // draw_sprite(image_move1, zombie->pos.x, zombie->pos.y, 40, 40);
        // if (bullet->pos.x + 40 == zombie->pos.x)
        // {
        //     draw_sprite(image_move3, zombie->pos.x, zombie->pos.y, 40, 40);
        //     write(fd, display_ram, DISPLAY_SIZE);
        //     break;
        // }
        // if (peashooter->timeout == 1)
        //     draw_sprite(image_peashooter2, 0, 0, 40, 40);
        // else
        //     draw_sprite(image_peashooter1, 0, 0, 40, 40);

        // if (zombie != NULL)
        // {
        //     draw_sprite(image_move1, zombie->pos.x, 0, 40, 40);
        // }

        /*prepare plants*/
        for (int i = 0; i < 6; i++)
        {
            printf("i=%d\n", i);
            switch (plants[i][0].type)
            {
                case PEASHOOTER:
                {
                    struct peashooter *peashooter = (struct peashooter *)plants[i][0].plant;
                    if (peashooter->timeout == 1)
                    {
                        draw_sprite(image_peashooter2, peashooter->pos.x, peashooter->pos.y, 40, 40);
                    }
                    else 
                    {
                        draw_sprite(image_peashooter1, peashooter->pos.x, peashooter->pos.y, 40, 40);
                    }
                }
                case SUN_FLOWER:
                case TYPE_NONE:
                default:
            }
        }
        
        /*prepare zombie*/
        for (int i = 0; i < 100; i++)
        {
            if (zombies[i] != NULL)
            {
                printf("HHHH\n");
                if (zombies[i]->state == WALK_1)
                    draw_sprite(image_move1, zombies[i]->pos.x, zombies[i]->pos.y, 40, 40);
                else if (zombies[i]->state == DIE)
                    draw_sprite(image_move4, zombies[i]->pos.x, zombies[i]->pos.y, 40, 40);
                else
                    draw_sprite(image_move2, zombies[i]->pos.x, zombies[i]->pos.y, 40, 40);
            }
        }

        /*prepare bullet*/
        for (int i = 0; i < 100; i++)
        {
            if (bullet_list[i] != NULL)
            {
                draw_sprite(image_shot1, bullet_list[i]->pos.x, bullet_list[i]->pos.y, 40, 40);
            }
        }

        /* prepare the bullet */
        // for (int i = 0; i < 100; i++)
        // {
        //     if (bullet_list[i] != NULL)
        //     {
        //         /*handle collisions*/
        //         /*if the bullet coordinate reaches the zomebie coordinate*/
        //         /*calculate zombie health*/
        //         /*delete the bullet*/
        //         if (bullet_list[i]->pos.x  + 40 >= zombie->pos.x)
        //         {
        //             /*decreate zombie health*/
        //             zombie->health -= 10;
        //             /*draw animation*/
        //             if (zombie->health <= 0)
        //             {
        //                 draw_sprite(image_move4, zombie->pos.x, 0, 40, 40);
        //                 write(fd, display_ram, DISPLAY_SIZE);
        //                 free(zombie);
        //                 zombie = NULL;
        //                 return 0;
        //             }
        //             draw_sprite(image_move3, zombie->pos.x, 0, 40, 40);
        //             free(bullet_list[i]);
        //             bullet_list[i] = NULL;
        //         }
        //         else
        //         {
        //             draw_sprite(image_shot1, bullet_list[i]->pos.x, bullet_list[i]->pos.y, 40, 40);
        //             bullet_list[i]->pos.x += bullet_list[i]->speed;
        //         }
        //     }
        // }
        /*3. flush display RAM into the Kernel */
        write(fd, display_ram, DISPLAY_SIZE);
        // peashooter->timeout--;
        // zombie->pos.x -= zombie->speed;

        sleep(1);
    }

    close(fd);
    return 0;
}