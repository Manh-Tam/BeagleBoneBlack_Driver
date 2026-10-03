#ifndef PVZ_DISPLAY_H
#define PVZ_DISPLAY_H

#include "draw.h"

typedef struct display display_t;

display_t *display_open(const char *device_path);
int display_present(display_t *display, const framebuffer_t *framebuffer);
void display_close(display_t *display);

#endif
