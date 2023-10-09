#include "color.h"
#include "debug.h"

uint8_t _mix(uint8_t a, uint8_t b, double t) {
    DEBUG_ENTER_FUNC();
    DEBUG_ASSERT(0.0 <= t, "t has to be greater or equal to 0.");
    DEBUG_ASSERT(t <= 1.0, "t has to be smaller or equal to 1.");
    uint8_t result = (uint8_t)((double)(a) * (1.0 - t) + (double)(b) * t);
    DEBUG_EXIT_FUNC();
    return result;
}

void color_mix(const Color *color1, const Color *color2, double t, Color *newColor) {
    DEBUG_ENTER_FUNC();
    DEBUG_ASSERT(0.0 <= t, "t has to be greater or equal to 0.");
    DEBUG_ASSERT(t <= 1.0, "t has to be smaller or equal to 1.");
    *newColor = (Color){
        .c = _mix(color1->c, color2->c, t),
        .m = _mix(color1->m, color2->m, t),
        .y = _mix(color1->y, color2->y, t),
    };
    DEBUG_EXIT_FUNC();
}

void color_sub(const Color *color1, const Color *color2, Color *newColor) {
    DEBUG_ENTER_FUNC();
    *newColor = (Color){
        .c = color1->c - color2->c,
        .m = color1->m - color2->m,
        .y = color1->y - color2->y
    };
    DEBUG_EXIT_FUNC();
}

uint64_t color_componentSum(const Color *color) {
    DEBUG_ENTER_FUNC();
    uint64_t result = (uint64_t)(color->c) + (uint64_t)(color->m) + (uint64_t)(color->y);
    DEBUG_EXIT_FUNC();
    return result;
}

uint64_t color_weightedSquaredError(const Color *color1, const Color *color2, double weight) {
    DEBUG_ENTER_FUNC();
    Color errorColor = COLOR_NULL;
    color_sub(color1, color2, &errorColor);
    const uint64_t sqrtError = color_componentSum(&errorColor);
    uint64_t result = (uint64_t)((double)(sqrtError * sqrtError) * weight);
    DEBUG_ASSERT(result >= 0.0, "result has to be greater or equal to 0.")
    DEBUG_EXIT_FUNC();
    return result;
}
