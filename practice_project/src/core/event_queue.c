#include "event_queue.h"

#include <pthread.h>
#include <stdlib.h>

#include "../config.h"

struct touch_queue {
    touch_event_t events[PVZ_INPUT_QUEUE_CAPACITY];
    size_t head;
    size_t count;
    uint64_t overflow_count;
    pthread_mutex_t mutex;
};

touch_queue_t *touch_queue_create(void)
{
    touch_queue_t *queue = calloc(1U, sizeof(*queue));
    if (queue == NULL) {
        return NULL;
    }
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue);
        return NULL;
    }
    return queue;
}

void touch_queue_destroy(touch_queue_t *queue)
{
    if (queue != NULL) {
        (void)pthread_mutex_destroy(&queue->mutex);
        free(queue);
    }
}

bool touch_queue_push(touch_queue_t *queue, touch_event_t event)
{
    bool pushed = false;

    if (queue == NULL) {
        return false;
    }
    (void)pthread_mutex_lock(&queue->mutex);
    if (queue->count < PVZ_INPUT_QUEUE_CAPACITY) {
        size_t tail = (queue->head + queue->count) % PVZ_INPUT_QUEUE_CAPACITY;
        queue->events[tail] = event;
        ++queue->count;
        pushed = true;
    } else {
        ++queue->overflow_count;
    }
    (void)pthread_mutex_unlock(&queue->mutex);
    return pushed;
}

bool touch_queue_try_pop(touch_queue_t *queue, touch_event_t *event)
{
    bool popped = false;

    if (queue == NULL || event == NULL) {
        return false;
    }
    (void)pthread_mutex_lock(&queue->mutex);
    if (queue->count > 0U) {
        *event = queue->events[queue->head];
        queue->head = (queue->head + 1U) % PVZ_INPUT_QUEUE_CAPACITY;
        --queue->count;
        popped = true;
    }
    (void)pthread_mutex_unlock(&queue->mutex);
    return popped;
}

size_t touch_queue_size(touch_queue_t *queue)
{
    size_t size = 0U;

    if (queue != NULL) {
        (void)pthread_mutex_lock(&queue->mutex);
        size = queue->count;
        (void)pthread_mutex_unlock(&queue->mutex);
    }
    return size;
}

uint64_t touch_queue_overflow_count(touch_queue_t *queue)
{
    uint64_t count = 0U;

    if (queue != NULL) {
        (void)pthread_mutex_lock(&queue->mutex);
        count = queue->overflow_count;
        (void)pthread_mutex_unlock(&queue->mutex);
    }
    return count;
}
