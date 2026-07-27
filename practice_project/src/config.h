#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <stdio.h>

#define LCD_DEVICE                  "/dev/lcd"
#define TOUCH_DEVICE                "/dev/touch"
#define QUEUE_NAME                  "/touch_queue"
#define MAX_BULLET                  100
#define MAX_ZOMBIE                  5
/*diplay size in bytes*/
#define DISPLAY_SIZE                (320*240*2)
#define GRID_ROW                    (5)
#define GRID_COLUMN                 (8)
#define SUNFLOWER_PRICE             (50)
#define PEASHOOTER_PRICE            (100)

typedef enum 
{
    GENERATE,
    DISPLAY,
    UPDATE,
}program_state;

#endif