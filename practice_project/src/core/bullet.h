#ifndef __BULLET_H__
#define __BULLET_H__
#include "pos.h"

enum bullet_state
{
    BULLET_INACTIVE,
    BULLET_FLYING,
    BULLET_EXPLODING,
};

struct bullet
{
    struct pos pos;
    int speed;
    int damage;
    enum bullet_state state;
};

#endif