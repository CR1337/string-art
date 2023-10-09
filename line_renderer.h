#ifndef __LINE_RENDERER_H__
#define __LINE_RENDERER_H__

#include <stdint.h>

typedef void(*DrawPixelFunction)(
    uint64_t x,
    uint64_t y,
    double intensity,
    void *argument
);

typedef struct {
    DrawPixelFunction drawPixelFunction;
    void *argument;
} LineRenderer;

LineRenderer * lineRenderer_new(DrawPixelFunction drawPixelFunction);
void lineRenderer_delete(LineRenderer *self);

void lineRenderer_setArgument(LineRenderer *self, void *argument);
void lineRenderer_draw(
    LineRenderer *self,
    uint64_t x0,
    uint64_t y0,
    uint64_t x1,
    uint64_t y1,
    double width
);


#endif // __LINE_RENDERER_H__
