#ifndef __PEASHOOTER_H__
#define __PEASHOOTER_H__

#include "pos.h"

enum plant_state
{
    PLANT_IDLE,
    PLANT_RECOVERING,
    PLANT_READY,
    PLANT_SHOOTING,
    PLANT_DEAD,
};

struct peashooter
{
    int shoot_cooldown;
    
};

struct sunflower
{
    int sun_cooldown;
};


void peashooter_init(struct peashooter *ps, struct pos pos);


typedef enum
{
    PLANT_NONE = 0,
    PLANT_PEASHOOTER,
    PLANT_SUNFLOWER,
}plant_type;

struct plant
{
    plant_type type;
    int row;
    int col;
    int health;
    enum plant_state state;
    union 
    {
        struct peashooter peashooter;
        struct sunflower sunflower;
    }data;
};


#endif