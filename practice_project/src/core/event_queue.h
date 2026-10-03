#ifndef PVZ_EVENT_QUEUE_H
#define PVZ_EVENT_QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "pos.h"

typedef struct touch_queue touch_queue_t;

touch_queue_t *touch_queue_create(void);
void touch_queue_destroy(touch_queue_t *queue);
bool touch_queue_push(touch_queue_t *queue, touch_event_t event);
bool touch_queue_try_pop(touch_queue_t *queue, touch_event_t *event);
size_t touch_queue_size(touch_queue_t *queue);
uint64_t touch_queue_overflow_count(touch_queue_t *queue);

#endif
