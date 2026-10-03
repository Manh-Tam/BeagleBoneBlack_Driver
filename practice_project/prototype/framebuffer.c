#ifndef _FRAMEBUFFER_H_
#define _FRAMEBUFFER_H_

typedef struct {
    uint8_t *pixels;
    size_t size;

    uint16_t width;
    uint16_t height;
    uint16_t stride;
} framebuffer_t;

#endif