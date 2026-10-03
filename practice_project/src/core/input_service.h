#ifndef PVZ_INPUT_SERVICE_H
#define PVZ_INPUT_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "pos.h"

typedef struct input_service input_service_t;

typedef enum {
    INPUT_SERVICE_STARTING = 0,
    INPUT_SERVICE_RUNNING,
    INPUT_SERVICE_STOPPED,
    INPUT_SERVICE_FAILED
} input_service_status_t;

input_service_t *input_service_start(const char *device_path);
bool input_service_try_pop(input_service_t *service, touch_event_t *event);
input_service_status_t input_service_status(input_service_t *service);
int input_service_last_error(input_service_t *service);
uint64_t input_service_overflow_count(input_service_t *service);
void input_service_stop(input_service_t *service);
void input_service_destroy(input_service_t *service);

#endif
