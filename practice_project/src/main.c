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
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/sunflower_1.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/peashooter_item.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/sunflower1.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/sparkle.h"
#include "/home/tamle/Workspace/BBB_driver/practice_project/script/sunflower_bloom.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <poll.h>
#include <mqueue.h>
#include "config.h"
#include "touch.h"
#include "zombie.h"
#include "bullet.h"
#include "peashooter.h"
#include "draw.h"
#include "digit_bitmaps.h"

/*game objects*/
struct plant plant_grid[GRID_ROW][GRID_COLUMN] = {0};
struct zombie zombie_pool[MAX_ZOMBIE] = {0};
struct bullet bullet_pool[MAX_BULLET] = {0};
uint8_t display_ram[DISPLAY_SIZE];
static int sun_counter = 50;
static long int elapsed_time = 0;
int num_of_zombies = 0;

plant_type plant_selected = PLANT_NONE;

/*game state*/
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
program_state current_state;
struct mq_attr attr;
mqd_t mq;

/*file decriptor*/
int display_fd;
int touch_fd;

void* display_thread_fuc(void* arg);
void* GENERATE_thread_func(void *arg);
void* logic_thread_func(void* arg);
void* plant_placement_thread_func(void* arg);
void draw_sun_counter(int x, int y, int amount);
int main()
{
    int ret = 0;
    pthread_t generate_thread, display_thread, logic_thread, plant_placement_thread;
    display_fd = open(LCD_DEVICE, O_WRONLY);
    if (display_fd < 0)
    {
        perror("Failed to open display\n");
        return display_fd;
    }

    touch_fd = open(TOUCH_DEVICE, O_RDWR);
    if (touch_fd < 0)
    {
        perror("Failed to open touch\n");
        return touch_fd;
    }

    /* Create message queue */
    attr = (struct mq_attr){.mq_maxmsg = 10, .mq_msgsize = sizeof(struct touch_data)};
    mq_unlink(QUEUE_NAME);
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR | O_NONBLOCK, 0666, &attr);
    if (mq == (mqd_t)-1) 
    {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    /*2. Init objects*/
    current_state = GENERATE;

    /*Init zombie pool*/
    for (int i = 0; i < MAX_ZOMBIE; i++)
    {
        zombie_pool[i].type = ZOMBIE_NONE;
        zombie_pool[i].damage = 20;
        zombie_pool[i].health = 100;
        zombie_pool[i].pos = (struct pos){280, 40};
        zombie_pool[i].speed = 20;
        zombie_pool[i].state = ZOMBIE_WALKING_1;
    }

    /*Init plant*/
    for (int i = 0; i < GRID_ROW; i++)
    {
        for (int j = 0; j < GRID_COLUMN; j++)
        {
            plant_grid[i][j].type = PLANT_NONE;
        }
    }

    /*Init bullets*/
    for (int i = 0; i < MAX_BULLET; i++)
    {
        bullet_pool[i].state = BULLET_INACTIVE;
    }

    /*3. Create threads*/
    ret = pthread_create(&generate_thread, NULL, GENERATE_thread_func, NULL);
    if (ret != 0)
    {
        perror("Failed to create generate thread\n");
        return ret;
    }

    ret = pthread_create(&display_thread, NULL, display_thread_fuc, NULL);
    if (ret != 0)
    {
        perror("Failed to create display thread\n");
        return ret;
    }

    ret = pthread_create(&logic_thread, NULL, logic_thread_func, NULL);
    if (ret != 0)
    {
        perror("Failed to create logic thread\n");
        return ret;
    }

    ret = pthread_create(&plant_placement_thread, NULL, plant_placement_thread_func, NULL);
    if (ret != 0)
    {
        perror("Failed to create plant placement thread\n");
        return ret;
    }

    printf("Game running\n");
    pthread_join(display_thread, NULL);
    pthread_join(logic_thread, NULL);
    pthread_join(plant_placement_thread, NULL);
    pthread_mutex_destroy(&buffer_mutex);
    mq_close(mq);
    mq_unlink(QUEUE_NAME);
    if (display_fd >= 0)
    {
        close(display_fd);
    }
    printf("Game stopped\n");
    return 0;
}

void* GENERATE_thread_func(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&buffer_mutex);
        while (current_state != GENERATE)
        {
            pthread_cond_wait(&cond, &buffer_mutex);
        }

        printf("Generate zombies\n");
        /*1. Generate ramdon zombies*/
        int num = 1 + rand() % 5; //random number between 1 and 5
        elapsed_time++;
        num_of_zombies = 1 + elapsed_time/100;
        for (int i = 0; i < num_of_zombies; i++)
        {
            if (zombie_pool[i].type == ZOMBIE_NONE)
            {
                struct zombie *zombie = &zombie_pool[i];
                zombie->type = ZOMBIE_NORMAL;
                zombie->pos.x = 280;
                zombie->pos.y = num * 40;
                zombie->health = 100;
                zombie->speed = 1;
                zombie->state = ZOMBIE_WALKING_1;
                break;
            }
        }

        /*2. Generate pea bullets*/
        printf("Generate bullets\n");
        for (int i = 0; i < GRID_ROW; i++)
        {
            for (int j = 0; j < GRID_COLUMN; j++)
            {
                if (plant_grid[i][j].type == PLANT_PEASHOOTER)
                {
                    struct plant *peashooter = &plant_grid[i][j];
                    if (peashooter->state == PLANT_SHOOTING)
                    {
                        for (int k = 0; k < MAX_BULLET; k++)
                        {
                            if (bullet_pool[k].state == BULLET_INACTIVE)
                            {
                                struct bullet *pbullet = &bullet_pool[k];
                                pbullet->damage = 2;
                                pbullet->pos = (struct pos) {peashooter->row, peashooter->col};
                                pbullet->speed = 10;
                                pbullet->state = BULLET_FLYING;
                                break;
                            }
                        }
                    }
                }
            }
        }

        /*3. Generate plant_grid*/
        printf("Generate plant_grid\n");
        struct touch_data touch_data = {0};
        if (mq_receive(mq, (char*)&touch_data, sizeof(touch_data), NULL) >= 0) {
            struct pos pos = {0};
            touch_data_to_coordinate(&touch_data, &pos.x, &pos.y);
            printf("x: %u\n", pos.x);
            printf("y: %u\n", pos.y);
            pos.x = pos.x - pos.x % 40;
            pos.y = pos.y - pos.y % 40;

            int row = pos.y / 40 - 1;
            int col = pos.x / 40;
            
            if ((row >= 0) && (plant_grid[row][col].type == PLANT_NONE))
            {
                switch (plant_selected)
                {
                case PLANT_SUNFLOWER:
                {
                    if (sun_counter >= SUNFLOWER_PRICE)
                    {
                        sun_counter -= SUNFLOWER_PRICE;
                        struct plant *sunflower = &plant_grid[row][col];
                        sunflower->type = PLANT_SUNFLOWER;
                        sunflower->col = pos.y;
                        sunflower->row = pos.x;
                        sunflower->data.sunflower.sun_cooldown = 3;
                        sunflower->state = PLANT_IDLE;
                        sunflower->health = 100;
                        printf("Plant sunflower placed\n");
                    }
                    break;
                }
                case PLANT_PEASHOOTER:
                {
                    if (sun_counter >= PEASHOOTER_PRICE)
                    {
                        sun_counter -= PEASHOOTER_PRICE;
                        struct plant *peashooter = &plant_grid[row][col];
                        peashooter->type = PLANT_PEASHOOTER;
                        peashooter->col = pos.y;
                        peashooter->row = pos.x;
                        peashooter->data.peashooter.shoot_cooldown = 3;
                        peashooter->state = PLANT_IDLE;
                        peashooter->health = 100;
                        printf("Peashooter placed\n");
                    }
                    break;
                }
                default:
                    break;
                }
            }
            else if ((row == -1))
            {
                switch (col)
                {
                    case 0:
                        plant_selected = PLANT_SUNFLOWER;
                        break;
                    case 1:
                        plant_selected = PLANT_PEASHOOTER;
                        break;
                    case 2:
                        plant_selected = PLANT_NONE;
                        break;
                    case 3:
                        plant_selected = PLANT_NONE;
                        break;
                    case 4:
                        plant_selected = PLANT_NONE;
                        break;
                    case 5:
                        plant_selected = PLANT_NONE;
                        break;
                    case 6:
                        plant_selected = PLANT_NONE;
                        break;
                    case 7:
                        plant_selected = PLANT_NONE;
                        break;
                    default:
                        plant_selected = PLANT_NONE;
                        break;
                }
            }
        } else {
            // Queue empty
        }
        
        printf("Generate done\n");
        current_state = DISPLAY;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&buffer_mutex);
        usleep(1000);
    }
}

void draw_sun_counter(int x, int y, int amount) {
    char str[8];
    snprintf(str, sizeof(str), "%d", amount);

    int char_x = x + 5; // X offset (leaving room for Sun icon sprite)

    for (int i = 0; str[i] != '\0'; i++) {
        int digit = str[i] - '0';
        
        if (digit >= 0 && digit <= 9) {
            draw_digit_sprite(char_x, y, digit, COLOR_YELLOW);
        }
        
        char_x += DIGIT_WIDTH + 2; // Advance cursor (10 pixels pitch)
    }
}

void* display_thread_fuc(void* arg)
{
    while(1)
    {
        pthread_mutex_lock(&buffer_mutex);
        while(current_state != DISPLAY)
        {
            pthread_cond_wait(&cond, &buffer_mutex);
        }
        printf("display\n");
        
        /*1. Draw the grass field*/
        render_frame(display_ram);
        
        // memcpy(display_ram, image_map, DISPLAY_SIZE);
        draw_sprite(display_ram, image_sunflower_1, 0, 0, 40, 40);
        draw_sprite(display_ram, image_peashooter_item, 40, 0, 40, 40);

        /*2. Draw zombies*/
        for (int i = 0; i < MAX_ZOMBIE; i++)
        {
            if (zombie_pool[i].type != ZOMBIE_NONE)
            {
                struct zombie *zombie = &zombie_pool[i];
                switch (zombie->state)
                {
                case ZOMBIE_WALKING_1:
                    draw_sprite(display_ram, image_move1, zombie->pos.x, zombie->pos.y, 40, 40);
                    break;
                case ZOMBIE_WALKING_2:
                    draw_sprite(display_ram, image_move2, zombie->pos.x, zombie->pos.y, 40, 40);
                    break;
                case ZOMBIE_HURT:
                    draw_sprite(display_ram, image_move3, zombie->pos.x, zombie->pos.y, 40, 40);
                    break;
                case ZOMBIE_EATING:
                    draw_sprite(display_ram, image_move3, zombie->pos.x, zombie->pos.y, 40, 40);
                    break;
                case ZOMBIE_DEAD:
                    draw_sprite(display_ram, image_move4, zombie->pos.x, zombie->pos.y, 40, 40);
                    break;
                default:
                    break;
                }
            }
        }

        /*3. Draw plant_grid: peashooters, sunflowers*/
        for (int i = 0; i < GRID_ROW; i++)
        {
            for (int j = 0; j < GRID_COLUMN; j++)
            {
                struct plant *plant = &plant_grid[i][j];
                if (plant->type != PLANT_NONE)
                {
                    switch (plant->type)
                    {
                    case PLANT_PEASHOOTER:
                    {
                        printf("hello peashooter\n");
                        switch (plant->state)
                        {
                        case PLANT_IDLE:
                            printf("shooter idle\n");
                            draw_sprite(display_ram, image_peashooter1, plant->row, plant->col, 40, 40);
                            break;
                        case PLANT_RECOVERING:
                            printf("shooter recovering\n");
                            draw_sprite(display_ram, image_peashooter2, plant->row, plant->col, 40, 40);
                            break;
                        case PLANT_READY:
                            printf("shooter recovering\n");
                            draw_sprite(display_ram, image_peashooter2, plant->row, plant->col, 40, 40);
                            break;
                        case PLANT_SHOOTING:
                            printf("shooter shooting\n");
                            draw_sprite(display_ram, image_peashooter1, plant->row, plant->col, 40, 40);
                            break;
                        case PLANT_DEAD:
                            printf("shooter dead\n");
                            
                            break;
                        default:
                            break;
                        }
                        break;
                    }
                    case PLANT_SUNFLOWER:
                    {
                        printf("hello peashooter\n");
                        switch (plant->state)
                        {
                        case PLANT_IDLE:
                            printf("sunflower idle\n");
                            draw_sprite(display_ram, image_sunflower1, plant->row, plant->col, 40, 40);
                            break;
                        case PLANT_RECOVERING:
                            printf("sunflower recovering\n");
                            draw_sprite(display_ram, image_sunflower1, plant->row, plant->col, 40, 40);
                            break;
                        case PLANT_READY:
                            printf("sunflower recovering\n");
                            draw_sprite(display_ram, image_sunflower_bloom, plant->row, plant->col, 40, 40);
                            break;
                        case PLANT_DEAD:
                            printf("sunflower dead\n");
                            break;
                        default:
                            break;
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
        }

        /*4. Draw bullets*/
        for (int i = 0; i < MAX_BULLET; i++)
        {
            if (bullet_pool[i].state != BULLET_INACTIVE)
            {
                if (bullet_pool[i].state == BULLET_FLYING)
                    draw_sprite(display_ram, image_shot1, bullet_pool[i].pos.x, bullet_pool[i].pos.y, 40, 40);
                else if (bullet_pool[i].state == BULLET_EXPLODING)
                    draw_sprite(display_ram, image_sparkle, bullet_pool[i].pos.x, bullet_pool[i].pos.y, 40, 40);
            }
        }

        /*5. Draw numbers*/
        draw_sun_counter(0, 0, 50);
        draw_sun_counter(40, 0, 100);
        draw_sun_counter(80, 30, sun_counter);
        draw_sun_counter(120, 0, num_of_zombies);

        write(display_fd, display_ram, DISPLAY_SIZE);
        current_state = UPDATE;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&buffer_mutex);
        usleep(1000);
    }
    return NULL;
}

void* logic_thread_func(void* arg)
{

    while (1)
    {
        pthread_mutex_lock(&buffer_mutex);
        while (current_state != UPDATE)
        {
            pthread_cond_wait(&cond, &buffer_mutex);
        }
        sun_counter++;
        printf("logic_thread_func\n");

        /*1. Update plant_grid*/

        for (int i = 0; i < GRID_ROW; i++)
        {
            for (int j = 0; j < GRID_COLUMN; j++)
            {
                if (plant_grid[i][j].type != PLANT_NONE)
                {
                    if (plant_grid[i][j].type == PLANT_PEASHOOTER)
                    {
                        struct plant *plant = &plant_grid[i][j];
                        switch (plant->state)
                        {
                        case PLANT_IDLE:
                            plant->state = PLANT_RECOVERING;
                            break;
                        case PLANT_RECOVERING:
                            plant->state = PLANT_READY;
                            break;
                        case PLANT_READY:
                            plant->state = PLANT_READY;
                            break;
                        case PLANT_SHOOTING:
                            plant->state = PLANT_IDLE;
                            break;
                        case PLANT_DEAD:
                            plant->type = PLANT_NONE;
                            break;
                        default:
                            break;
                        }
                    }
                    else if (plant_grid[i][j].type == PLANT_SUNFLOWER)
                    {
                        struct plant *plant = &plant_grid[i][j];
                        switch (plant->state)
                        {
                        case PLANT_IDLE:
                            plant->state = PLANT_RECOVERING;
                            break;
                        case PLANT_RECOVERING:
                            plant->state = PLANT_READY;
                            break;
                        case PLANT_READY:
                            sun_counter+=5;
                            plant->state = PLANT_IDLE;
                            break;
                        case PLANT_DEAD:
                            plant->type = PLANT_NONE;
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }

        /*plant state to shooting*/
        for (int i = 0; i < GRID_ROW; i++)
        {
            for (int j = 0; j < GRID_COLUMN; j++)
            {
                if (plant_grid[i][j].type != PLANT_NONE)
                {
                    if (plant_grid[i][j].type == PLANT_PEASHOOTER)
                    {
                        for (int k = 0; k < MAX_ZOMBIE; k++)
                        {
                            if (zombie_pool[k].type != ZOMBIE_NONE)
                            {
                                int row = zombie_pool[k].pos.y / 40 - 1;
                                int col = zombie_pool[k].pos.x / 40;
                                if ((row == i) && (j <= col))
                                {
                                    if (plant_grid[i][j].state == PLANT_READY)
                                        plant_grid[i][j].state = PLANT_SHOOTING;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        /*2. Update zombie's state*/
        for (int i = 0; i < MAX_ZOMBIE; i++)
        {
            if (zombie_pool[i].type != ZOMBIE_NONE)
            {
                zombie_pool[i].pos.x -= zombie_pool[i].speed;
                if (zombie_pool[i].pos.x <= 0)
                {
                    return NULL;
                }
                switch (zombie_pool[i].state)
                {
                case ZOMBIE_WALKING_1:
                    zombie_pool[i].state = ZOMBIE_WALKING_2;
                    break;
                case ZOMBIE_WALKING_2:
                    zombie_pool[i].state = ZOMBIE_WALKING_1;
                    break;
                case ZOMBIE_HURT:
                    zombie_pool[i].state = ZOMBIE_WALKING_1;
                    break;
                case ZOMBIE_EATING:
                    zombie_pool[i].state = ZOMBIE_WALKING_1;
                    break;
                case ZOMBIE_DEAD:
                    zombie_pool[i].type = ZOMBIE_NONE;
                    break;
                default:
                    break;
                }
            }
        }

        /*3. Handle zombie and plant collision*/
        for (int i = 0; i < MAX_ZOMBIE; i++)
        {
            if (zombie_pool[i].type != ZOMBIE_NONE)
            {
                int row = zombie_pool[i].pos.y / 40 - 1;
                int col = (zombie_pool[i].pos.x + zombie_pool[i].speed) / 40;
                if ((row >= 0) && (plant_grid[row][col].type != PLANT_NONE))
                {
                    printf("row: %d\n", row);
                    printf("col: %d\n", col);
                    printf("Zombie + Plant Collision herre\n");
                    zombie_pool[i].pos.x += zombie_pool[i].speed;
                    zombie_pool[i].state = ZOMBIE_EATING;
                    plant_grid[row][col].health -= zombie_pool[i].damage;
                    if (plant_grid[row][col].health <= 0)
                    {
                        plant_grid[row][col].state = PLANT_DEAD;
                    }
                }
            }
        }

        /*5. Update peashot's state*/
        for (int i = 0; i < MAX_BULLET; i++)
        {
            if (bullet_pool[i].state != BULLET_INACTIVE)
            {
                if (bullet_pool[i].state == BULLET_EXPLODING)
                {
                    bullet_pool[i].state = BULLET_INACTIVE;
                }
                else
                {
                    bullet_pool[i].pos.x += bullet_pool[i].speed;
                    if (bullet_pool[i].pos.x + 40 - bullet_pool[i].speed >= 320)
                        bullet_pool[i].state = BULLET_INACTIVE;
                }
            }
        }

        /*6. Handle collision of zombies and bullets*/
        for (int i = 0; i < MAX_BULLET; i++)
        {
            if (bullet_pool[i].state != BULLET_INACTIVE)
            {
                for (int j = 0; j < MAX_ZOMBIE; j++)
                {
                    if (zombie_pool[j].type != ZOMBIE_NONE)
                    {
                        if (bullet_pool[i].pos.y == zombie_pool[j].pos.y)
                        {
                            if ((bullet_pool[i].pos.x - zombie_pool[j].pos.x >= 0) &&
                                (bullet_pool[i].pos.x - zombie_pool[j].pos.x < bullet_pool[i].speed + zombie_pool[j].speed))
                            {
                                printf("Bullet Zombie collision\n");
                                bullet_pool[i].state = BULLET_EXPLODING;
                                zombie_pool[j].health -= bullet_pool[i].damage;
                                if (zombie_pool[j].health <= 0)
                                {
                                    zombie_pool[j].state = ZOMBIE_DEAD;
                                }
                            }
                        }
                    }
                }
            }
        }

        current_state = GENERATE;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&buffer_mutex);
        usleep(1000);
    }

    return NULL;
}

void* plant_placement_thread_func(void* arg)
{
    int ret = 0;
    struct touch_data touch_data = {0};
    struct pollfd pfd;
    pfd.fd = touch_fd;
    pfd.events = POLLIN;
    bool plant_selected = false;

    while (1)
    {

        printf("plant placement thread\n");
        ret = poll(&pfd, 1, -1); //sleep forever
        if (ret < 0)
        {
            perror("Failed to poll\n");
            break;
        }
        if ((pfd.revents & POLLIN) != 0)
        {
            ret = read(touch_fd, &touch_data, sizeof(touch_data));
            if (ret < 0)
            {
                perror("Failed to read\n");
                break;
            }
            else
            {
                pthread_mutex_lock(&buffer_mutex);

                if (mq_send(mq, (const char*)&touch_data, sizeof(touch_data), 0) == -1) 
                {
                    perror("mq_send");
                    mq_close(mq);
                    mq_unlink(QUEUE_NAME);
                    exit(EXIT_FAILURE);
                }

                // uint16_t x, y;
                // convert_tdata_to_poss(&touch_data, &x, &y);
                // printf("x pos: %u\n", x);
                // printf("y pos: %u\n", y);
                // bool plant_added = false;
                // x = x - x % 40;
                // y = y - y % 40;
                // int j = x / 40;
                // int i = y / 40 - 1;
                // printf("x: %d\n", x);
                // printf("y: %d\n", y);
                // printf("i: %d\n", i);
                // printf("j: %d\n", j);

                // if ((i >= 0) && (plant_grid[i][j].plant == NULL))
                // {
                //     struct peashooter *ps;
                //     ps = malloc(sizeof(*ps));
                //     ps->attack_speed = 1;
                //     ps->health = 100;
                //     ps->pos.x = x;
                //     ps->pos.y = y;
                //     ps->timeout = 3;
                //     plant_grid[i][j].plant = (void*)ps;
                //     plant_grid[i][j].type = PEASHOOTER;
                // }
                pthread_mutex_unlock(&buffer_mutex);
            }
        }
        usleep(1000);
    }
    perror("Failed to read touch data\n");
    return NULL;
}

#if 0
int main()
{
    srand(time(NULL)); // Seed RNG
    int fd = open("/dev/lcd", O_RDWR);
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
        plant_grid[i][0].type = PEASHOOTER;
        plant_grid[i][0].plant = (void*)peashooter;
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
                if (plant_grid[i][j].type == PEASHOOTER)
                {
                    struct peashooter *peashooter = (struct peashooter*)plant_grid[i][j].plant;
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
                            if (bullet_pool[i] == NULL)
                            {
                                printf("degguubb\n");
                                bullet_pool[i] = bullet;
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
            if (bullet_pool[i] != NULL)
            {
                bullet_pool[i]->pos.x += bullet_pool[i]->speed;
                if (bullet_pool[i]->pos.x >= 300)
                {
                    free(bullet_pool[i]);
                    bullet_pool[i] = NULL;
                }
                else
                {
                    for (int j = 0; j < 100; j++)
                    {
                        if (zombies[j] != NULL)
                        {
                            if (zombies[j]->pos.y == bullet_pool[i]->pos.y)
                            {
                                if (bullet_pool[i]->pos.x >= zombies[j]->pos.x)
                                {
                                    zombies[j]->state = DIE;
                                    free(bullet_pool[i]);
                                    bullet_pool[i] = NULL;
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
        //         if (bullet_pool[i] == NULL) 
        //         {
        //             bullet_pool[i] = bullet;
        //             printf("I'm here\n");
        //             break;
        //         }
        //     }

        //     /*3. reload the timeout*/
        //     peashooter->timeout = peashooter->attack_speed;
        // }
        
        // draw_sprite(display_ram, image_move1, zombie->pos.x, zombie->pos.y, 40, 40);
        // if (bullet->pos.x + 40 == zombie->pos.x)
        // {
        //     draw_sprite(display_ram, image_move3, zombie->pos.x, zombie->pos.y, 40, 40);
        //     write(fd, display_ram, DISPLAY_SIZE);
        //     break;
        // }
        // if (peashooter->timeout == 1)
        //     draw_sprite(display_ram, image_peashooter2, 0, 0, 40, 40);
        // else
        //     draw_sprite(display_ram, image_peashooter1, 0, 0, 40, 40);

        // if (zombie != NULL)
        // {
        //     draw_sprite(display_ram, image_move1, zombie->pos.x, 0, 40, 40);
        // }

        /*prepare plant_grid*/
        for (int i = 0; i < 6; i++)
        {
            printf("i=%d\n", i);
            switch (plant_grid[i][0].type)
            {
                case PEASHOOTER:
                {
                    struct peashooter *peashooter = (struct peashooter *)plant_grid[i][0].plant;
                    if (peashooter->timeout == 1)
                    {
                        draw_sprite(display_ram, image_peashooter2, peashooter->pos.x, peashooter->pos.y, 40, 40);
                    }
                    else 
                    {
                        draw_sprite(display_ram, image_peashooter1, peashooter->pos.x, peashooter->pos.y, 40, 40);
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
                    draw_sprite(display_ram, image_move1, zombies[i]->pos.x, zombies[i]->pos.y, 40, 40);
                else if (zombies[i]->state == DIE)
                    draw_sprite(display_ram, image_move4, zombies[i]->pos.x, zombies[i]->pos.y, 40, 40);
                else
                    draw_sprite(display_ram, image_move2, zombies[i]->pos.x, zombies[i]->pos.y, 40, 40);
            }
        }

        /*prepare bullet*/
        for (int i = 0; i < 100; i++)
        {
            if (bullet_pool[i] != NULL)
            {
                draw_sprite(display_ram, image_shot1, bullet_pool[i]->pos.x, bullet_pool[i]->pos.y, 40, 40);
            }
        }

        /* prepare the bullet */
        // for (int i = 0; i < 100; i++)
        // {
        //     if (bullet_pool[i] != NULL)
        //     {
        //         /*handle collisions*/
        //         /*if the bullet pos reaches the zomebie pos*/
        //         /*calculate zombie health*/
        //         /*delete the bullet*/
        //         if (bullet_pool[i]->pos.x  + 40 >= zombie->pos.x)
        //         {
        //             /*decreate zombie health*/
        //             zombie->health -= 10;
        //             /*draw animation*/
        //             if (zombie->health <= 0)
        //             {
        //                 draw_sprite(display_ram, image_move4, zombie->pos.x, 0, 40, 40);
        //                 write(fd, display_ram, DISPLAY_SIZE);
        //                 free(zombie);
        //                 zombie = NULL;
        //                 return 0;
        //             }
        //             draw_sprite(display_ram, image_move3, zombie->pos.x, 0, 40, 40);
        //             free(bullet_pool[i]);
        //             bullet_pool[i] = NULL;
        //         }
        //         else
        //         {
        //             draw_sprite(display_ram, image_shot1, bullet_pool[i]->pos.x, bullet_pool[i]->pos.y, 40, 40);
        //             bullet_pool[i]->pos.x += bullet_pool[i]->speed;
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
#endif