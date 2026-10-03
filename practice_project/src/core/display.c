#include "display.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "../config.h"

struct display {
    int fd;
};

display_t *display_open(const char *device_path)
{
    display_t *display;

    if (device_path == NULL) {
        errno = EINVAL;
        return NULL;
    }
    display = malloc(sizeof(*display));
    if (display == NULL) {
        return NULL;
    }
    display->fd = open(device_path, O_WRONLY);
    if (display->fd < 0) {
        int saved_error = errno;
        free(display);
        errno = saved_error;
        return NULL;
    }
    return display;
}

int display_present(display_t *display, const framebuffer_t *framebuffer)
{
    ssize_t written;

    if (display == NULL || framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->size != PVZ_FRAMEBUFFER_SIZE) {
        errno = EINVAL;
        return -1;
    }

    do {
        written = write(display->fd, framebuffer->pixels, framebuffer->size);
    } while (written < 0 && errno == EINTR);

    if (written < 0) {
        return -1;
    }
    if ((size_t)written != framebuffer->size) {
        errno = EIO;
        return -1;
    }
    return 0;
}

void display_close(display_t *display)
{
    if (display != NULL) {
        if (display->fd >= 0) {
            (void)close(display->fd);
        }
        free(display);
    }
}
