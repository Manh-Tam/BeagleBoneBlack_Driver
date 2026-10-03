#include "assets.h"

#include "../../script/move1.h"
#include "../../script/move2.h"
#include "../../script/move3.h"
#include "../../script/move4.h"
#include "../../script/peashooter1.h"
#include "../../script/peashooter2.h"
#include "../../script/peashooter_item.h"
#include "../../script/shot1.h"
#include "../../script/sparkle.h"
#include "../../script/sunflower1.h"
#include "../../script/sunflower_1.h"
#include "../../script/sunflower_bloom.h"

static const asset_image_t menu_sunflower = {image_sunflower_1, 40U, 40U};
static const asset_image_t menu_peashooter = {image_peashooter_item, 40U, 40U};
static const asset_image_t plant_sunflower = {image_sunflower1, 40U, 40U};
static const asset_image_t plant_sunflower_ready = {image_sunflower_bloom, 40U, 40U};
static const asset_image_t plant_peashooter = {image_peashooter1, 40U, 40U};
static const asset_image_t plant_peashooter_ready = {image_peashooter2, 40U, 40U};
static const asset_image_t zombie_walk_1 = {image_move1, 40U, 40U};
static const asset_image_t zombie_walk_2 = {image_move2, 40U, 40U};
static const asset_image_t zombie_hurt = {image_move3, 40U, 40U};
static const asset_image_t zombie_dead = {image_move4, 40U, 40U};
static const asset_image_t bullet_flying = {image_shot1, 40U, 40U};
static const asset_image_t bullet_exploding = {image_sparkle, 40U, 40U};

const asset_image_t *assets_menu_item(plant_type_t type)
{
    if (type == PLANT_SUNFLOWER) {
        return &menu_sunflower;
    }
    if (type == PLANT_PEASHOOTER) {
        return &menu_peashooter;
    }
    return NULL;
}

const asset_image_t *assets_plant(plant_type_t type, plant_state_t state)
{
    if (type == PLANT_SUNFLOWER) {
        return state == PLANT_READY ? &plant_sunflower_ready : &plant_sunflower;
    }
    if (type == PLANT_PEASHOOTER) {
        return state == PLANT_RECOVERING || state == PLANT_READY
                   ? &plant_peashooter_ready : &plant_peashooter;
    }
    return NULL;
}

const asset_image_t *assets_zombie(zombie_state_t state)
{
    switch (state) {
    case ZOMBIE_WALKING_1:
        return &zombie_walk_1;
    case ZOMBIE_WALKING_2:
        return &zombie_walk_2;
    case ZOMBIE_HURT:
    case ZOMBIE_EATING:
        return &zombie_hurt;
    case ZOMBIE_DEAD:
        return &zombie_dead;
    default:
        return NULL;
    }
}

const asset_image_t *assets_bullet(bullet_state_t state)
{
    return state == BULLET_EXPLODING ? &bullet_exploding : &bullet_flying;
}
