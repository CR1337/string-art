#ifndef __OPTIMIZER_H__
#define __OPTIMIZER_H__

#include "shared_data.h"
#include "worker_pool.h"
#include "line_renderer.h"

#include <stdbool.h>

typedef struct {
    SharedData *sharedData;
    WorkerPool *workerPool;
    LineRenderer **lineRenderers;

    Color *lastBestImage;
    Color *imageBuffer;
    uint64_t *lastBestErrorImage;
    uint64_t *errorBuffer;
    uint64_t lastBestError;
    uint64_t *errors;
    uint64_t *lastBestPointIndices;
    uint64_t *possibleConnections;
    bool **connectionIsDone;
    double *thicknessesInPixels;

    uint64_t currentThreadIndex;
    uint64_t currentIteration;
    uint64_t possibleConnectionAmount;

    // uint64_t minError;
    // double minRelativeError;
    uint64_t lastNormalizedError;
    uint64_t currentNormalizedError;
    uint64_t relativeErrorStreak;
} Optimizer;

Optimizer *optimizer_new(SharedData *sharedData);
void optimizer_delete(Optimizer *self);

void optimizer_optimize(Optimizer *self);

#endif // __OPTIMIZER_H__
