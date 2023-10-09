#include "line_renderer.h"

#include "error_handling.h"
#include "debug.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

LineRenderer * lineRenderer_new(DrawPixelFunction drawPixelFunction) {
    DEBUG_ENTER_FUNC();
    LineRenderer *lineRenderer = (LineRenderer*)malloc(sizeof(LineRenderer));
    if (!lineRenderer) {
        PRINT_ERROR("error while allocating lineRenderer");
        DEBUG_EXIT_FUNC();
        return NULL;
    }

    lineRenderer->drawPixelFunction = drawPixelFunction;
    lineRenderer->argument = NULL;

    DEBUG_EXIT_FUNC();
    return lineRenderer;
}

void lineRenderer_delete(LineRenderer *self) {
    DEBUG_ENTER_FUNC();
    free(self);
    DEBUG_EXIT_FUNC();
}

void lineRenderer_setArgument(LineRenderer *self, void *argument) {
    DEBUG_ENTER_FUNC();
    self->argument = argument;
    DEBUG_EXIT_FUNC();
}

double _lineRenderer_drawEndPoint(LineRenderer *self, double x, double y, double width, double gradient, bool isSteep, uint64_t *xPixels) {
    DEBUG_ENTER_FUNC();
    DEBUG_ASSERT(width > 0.0, "width has to be greater than 0.");
    DEBUG_ASSERT(x >= 0.0, "x has to be greater than or equal to 0.");
    DEBUG_ASSERT(y >= 0.0, "y has to be greater than or equal to 0.");
    const double xPoint = round(x);
    const double yPoint = (
        y - (width - 1.0) * 0.5 + gradient * (xPoint - x)
    );
    const double xGap = 1.0 - (x + 0.5 - xPoint);
    *xPixels = (uint64_t)xPoint;
    const double yPixels = (uint64_t)yPoint;
    const double fPart = yPoint - floor(yPoint);
    const double rfPart = 1.0 - fPart;
    const uint64_t uWidth = (uint64_t)width;

    if (isSteep) {
        self->drawPixelFunction(yPixels, *xPixels, rfPart * xGap, self->argument);
        for (uint64_t i = 1; i < width; ++i) {
            self->drawPixelFunction(yPixels + i, *xPixels, 1.0, self->argument);
        }
        self->drawPixelFunction(yPixels + uWidth, *xPixels, fPart * xGap, self->argument);
    } else {
        self->drawPixelFunction(*xPixels, yPixels, rfPart * xGap, self->argument);
        for (uint64_t i = 1; i < width; ++i) {
            self->drawPixelFunction(*xPixels, yPixels + i, 1.0, self->argument);
        }
        self->drawPixelFunction(*xPixels, yPixels + uWidth, fPart * xGap, self->argument);
    }

    DEBUG_EXIT_FUNC();
    return yPoint;
}

void lineRenderer_draw(
    LineRenderer *self,
    uint64_t x0,
    uint64_t y0,
    uint64_t x1,
    uint64_t y1,
    double width
) {
    DEBUG_ENTER_FUNC();
    // https://github.com/jambolo/thick-xiaolin-wu/blob/master/cs/thick-xiaolin-wu.coffee
    const bool isSteep = fabs(y1 - y0) > fabs(x1 - x0);

    if (isSteep) {
        double temp = x0;
        x0 = y0;
        y0 = temp;
        temp = x1;
        x1 = y1;
        y1 = temp;
    }

    if (x0 > x1) {
        double temp = x0;
        x0 = x1;
        x1 = temp;
        temp = y0;
        y0 = y1;
        y1 = temp;
    }

    const double dx = (double)x1 - (double)x0;
    const double dy = (double)y1 - (double)y0;
    const double gradient = (dx > 0.0) ? dy / dx : 1.0;

    width *= sqrt(1.0 + gradient * gradient);

    uint64_t xPixels0 = 0;
    uint64_t xPixels1 = 0;
    _lineRenderer_drawEndPoint(
        self, x1, y1, width, gradient, isSteep, &xPixels1
    );
    const double yPoint0 = _lineRenderer_drawEndPoint(
        self, x0, y0, width, gradient, isSteep, &xPixels0
    );
    double intery = yPoint0 + gradient;

    const uint64_t uWidth = (uint64_t)width;
    if (isSteep) {
        for (uint64_t x = xPixels0 + 1; x < xPixels1; ++x) {
            const double fPart = intery - floor(intery);
            const double rfPart = 1.0 - fPart;
            const uint64_t y = (uint64_t)intery;

            self->drawPixelFunction(y, x, rfPart, self->argument);
            for (uint64_t i = 1; i < width; ++i) {
                self->drawPixelFunction(y + i, x, 1.0, self->argument);
            }
            self->drawPixelFunction(y + uWidth, x, fPart, self->argument);

            intery += gradient;
        }
    } else {
        for (uint64_t x = xPixels0 + 1; x < xPixels1; ++x) {
            const double fPart = intery - floor(intery);
            const double rfPart = 1.0 - fPart;
            const uint64_t y = (uint64_t)intery;

            self->drawPixelFunction(x, y, rfPart, self->argument);
            for (uint64_t i = 1; i < width; ++i) {
                self->drawPixelFunction(x, y + i, 1.0, self->argument);
            }
            self->drawPixelFunction(x, y + uWidth, fPart, self->argument);

            intery += gradient;
        }
    }

    DEBUG_EXIT_FUNC();
}