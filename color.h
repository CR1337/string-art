#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>

#define COLOR_NULL ((Color){ 0, 0, 0 })

#pragma pack(1)
typedef struct {
    uint8_t c;
    uint8_t m;
    uint8_t y;
} Color;

void color_mix(const Color *color1, const Color *color2, double t, Color *newColor);
void color_sub(const Color *color1, const Color *color2, Color *newColor);
uint64_t color_componentSum(const Color *color);
uint64_t color_weightedSquaredError(const Color *color1, const Color *color2, double weight);

#endif // __COLOR_H__
