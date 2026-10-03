#ifndef PVZ_ASSETS_H
#define PVZ_ASSETS_H

#include <stdint.h>

#include "game.h"

typedef struct {
    const uint8_t *pixels;
    uint16_t width;
    uint16_t height;
} asset_image_t;

const asset_image_t *assets_menu_item(plant_type_t type);
const asset_image_t *assets_plant(plant_type_t type, plant_state_t state);
const asset_image_t *assets_zombie(zombie_state_t state);
const asset_image_t *assets_bullet(bullet_state_t state);

#endif
