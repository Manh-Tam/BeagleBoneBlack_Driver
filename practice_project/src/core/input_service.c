#include "input_service.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../config.h"
#include "event_queue.h"
#include "touch.h"

struct input_service {
    char *device_path;
    touch_queue_t *queue;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t started_condition;
    bool stop_requested;
    bool thread_started;
    bool thread_joined;
    input_service_status_t status;
    int last_error;
};

static char *copy_string(const char *source)
{
    size_t length;
    char *copy;

    if (source == NULL) {
        return NULL;
    }
    length = 0U;
    while (source[length] != '\0') {
        ++length;
    }
    copy = malloc(length + 1U);
    if (copy != NULL) {
        for (size_t i = 0U; i <= length; ++i) {
            copy[i] = source[i];
        }
    }
    return copy;
}

static bool stop_requested(input_service_t *service)
{
    bool stop;

    (void)pthread_mutex_lock(&service->mutex);
    stop = service->stop_requested;
    (void)pthread_mutex_unlock(&service->mutex);
    return stop;
}

static void set_status(input_service_t *service, input_service_status_t status,
                       int error_number)
{
    (void)pthread_mutex_lock(&service->mutex);
    service->status = status;
    service->last_error = error_number;
    (void)pthread_cond_broadcast(&service->started_condition);
    (void)pthread_mutex_unlock(&service->mutex);
}

static void *input_thread(void *argument)
{
    input_service_t *service = argument;
    struct pollfd descriptor;
    bool warned_legacy_read = false;
    int fd = open(service->device_path, O_RDONLY);

    if (fd < 0) {
        set_status(service, INPUT_SERVICE_FAILED, errno);
        return NULL;
    }

    descriptor.fd = fd;
    descriptor.events = POLLIN;
    descriptor.revents = 0;
    set_status(service, INPUT_SERVICE_RUNNING, 0);

    while (!stop_requested(service)) {
        int result = poll(&descriptor, 1, PVZ_INPUT_POLL_TIMEOUT_MS);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            set_status(service, INPUT_SERVICE_FAILED, errno);
            break;
        }
        if (result == 0) {
            continue;
        }
        if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            set_status(service, INPUT_SERVICE_FAILED, EIO);
            break;
        }
        if ((descriptor.revents & POLLIN) != 0) {
            uint8_t raw[4] = {0U, 0U, 0U, 0U};
            ssize_t count = read(fd, raw, sizeof(raw));
            touch_event_t event;

            if (count < 0) {
                if (errno == EINTR) {
                    continue;
                }
                set_status(service, INPUT_SERVICE_FAILED, errno);
                break;
            }
            if (count != (ssize_t)sizeof(raw) && count != 0) {
                set_status(service, INPUT_SERVICE_FAILED, EPROTO);
                break;
            }
            if (count == 0 && !warned_legacy_read) {
                (void)fprintf(stderr,
                              "warning: /dev/touch returned 0 after filling a sample; "
                              "using legacy driver compatibility\n");
                warned_legacy_read = true;
            }
            if (touch_decode_raw(raw, &event)) {
                (void)touch_queue_push(service->queue, event);
            }
        }
    }

    (void)close(fd);
    (void)pthread_mutex_lock(&service->mutex);
    if (service->status != INPUT_SERVICE_FAILED) {
        service->status = INPUT_SERVICE_STOPPED;
    }
    (void)pthread_mutex_unlock(&service->mutex);
    return NULL;
}

input_service_t *input_service_start(const char *device_path)
{
    input_service_t *service;
    int result;

    if (device_path == NULL) {
        errno = EINVAL;
        return NULL;
    }

    service = calloc(1U, sizeof(*service));
    if (service == NULL) {
        return NULL;
    }
    service->device_path = copy_string(device_path);
    service->queue = touch_queue_create();
    service->status = INPUT_SERVICE_STARTING;
    if (service->device_path == NULL || service->queue == NULL ||
        pthread_mutex_init(&service->mutex, NULL) != 0) {
        free(service->device_path);
        touch_queue_destroy(service->queue);
        free(service);
        errno = ENOMEM;
        return NULL;
    }
    if (pthread_cond_init(&service->started_condition, NULL) != 0) {
        (void)pthread_mutex_destroy(&service->mutex);
        free(service->device_path);
        touch_queue_destroy(service->queue);
        free(service);
        errno = ENOMEM;
        return NULL;
    }

    result = pthread_create(&service->thread, NULL, input_thread, service);
    if (result != 0) {
        (void)pthread_cond_destroy(&service->started_condition);
        (void)pthread_mutex_destroy(&service->mutex);
        free(service->device_path);
        touch_queue_destroy(service->queue);
        free(service);
        errno = result;
        return NULL;
    }
    service->thread_started = true;

    (void)pthread_mutex_lock(&service->mutex);
    while (service->status == INPUT_SERVICE_STARTING) {
        (void)pthread_cond_wait(&service->started_condition, &service->mutex);
    }
    result = service->last_error;
    (void)pthread_mutex_unlock(&service->mutex);

    if (input_service_status(service) == INPUT_SERVICE_FAILED) {
        input_service_destroy(service);
        errno = result;
        return NULL;
    }
    return service;
}

bool input_service_try_pop(input_service_t *service, touch_event_t *event)
{
    return service != NULL && touch_queue_try_pop(service->queue, event);
}

input_service_status_t input_service_status(input_service_t *service)
{
    input_service_status_t status = INPUT_SERVICE_FAILED;

    if (service != NULL) {
        (void)pthread_mutex_lock(&service->mutex);
        status = service->status;
        (void)pthread_mutex_unlock(&service->mutex);
    }
    return status;
}

int input_service_last_error(input_service_t *service)
{
    int error_number = EINVAL;

    if (service != NULL) {
        (void)pthread_mutex_lock(&service->mutex);
        error_number = service->last_error;
        (void)pthread_mutex_unlock(&service->mutex);
    }
    return error_number;
}

uint64_t input_service_overflow_count(input_service_t *service)
{
    return service == NULL ? 0U : touch_queue_overflow_count(service->queue);
}

void input_service_stop(input_service_t *service)
{
    if (service == NULL) {
        return;
    }

    (void)pthread_mutex_lock(&service->mutex);
    service->stop_requested = true;
    (void)pthread_mutex_unlock(&service->mutex);

    if (service->thread_started && !service->thread_joined) {
        (void)pthread_join(service->thread, NULL);
        service->thread_joined = true;
    }
}

void input_service_destroy(input_service_t *service)
{
    if (service == NULL) {
        return;
    }
    input_service_stop(service);
    (void)pthread_cond_destroy(&service->started_condition);
    (void)pthread_mutex_destroy(&service->mutex);
    touch_queue_destroy(service->queue);
    free(service->device_path);
    free(service);
}
