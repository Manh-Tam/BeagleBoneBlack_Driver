#ifndef __ZOMBIE_H__
#define __ZOMBIE_H__
#include "pos.h"

enum zombie_state
{
    ZOMBIE_WALKING_1,
    ZOMBIE_WALKING_2,
    ZOMBIE_HURT,
    ZOMBIE_EATING,
    ZOMBIE_DEAD,
};

enum zombie_type
{
    ZOMBIE_NONE,
    ZOMBIE_NORMAL,
    ZOMBIE_CONEHEAD,
};

struct zombie
{
    enum zombie_type type;
    enum zombie_state state;
    struct pos pos;
    int speed;
    int damage;
    int health;
};


#endif