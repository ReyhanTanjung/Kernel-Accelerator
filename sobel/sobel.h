#ifndef _SOBEL_H_
#define _SOBEL_H_

#define MAX_WIDTH 1024
#define MAX_HEIGHT 1024

typedef unsigned char pixel_t;
typedef short gradient_t;

extern "C" {
    void sobel_filter(const pixel_t *input, gradient_t *output, int width, int height);
}

#endif