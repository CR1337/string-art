#include "optimizer.h"
#include "error_handling.h"
#include "debug.h"

#include <string.h>
#include <math.h>

#define UINT64_T_MAX (0xffffffffffffffff)
#define TWO_PI (2.0 * 3.14159265358979323846264338328)

void _optimizer_drawPixel(
    uint64_t x,
    uint64_t y,
    double intensity,
    void *argument
);

Optimizer * _optimizer_construct(SharedData *sharedData) {
    DEBUG_ENTER_FUNC();
    const uint64_t imageWidth = sharedData->inputData.header->imageWidth;
    const uint64_t imageSize = imageWidth * imageWidth;
    const size_t workerAmount = 1; //workerPool_coreAmount();  #FIXME
    const Indexer *indexer = &(sharedData->inputData.header->indexer);

    DEBUG_PRINT("workerAmount: %ld\n", workerAmount);

    Optimizer *optimizer = (Optimizer*)calloc(1, sizeof(Optimizer));
    if (!optimizer) {
        PRINT_ERROR("error while allocating optimizer");
        DEBUG_EXIT_FUNC();
        return NULL;
    }

    optimizer->sharedData = sharedData;

    optimizer->workerPool = workerPool_new(workerAmount, true);
    if (!optimizer->workerPool) {
        PRINT_ERROR("error while constructing optimizer->workerPool");
        goto ERROR;
    }

    optimizer->lineRenderers = (LineRenderer**)calloc(workerAmount, sizeof(LineRenderer*));
    if (!optimizer->lineRenderers) {
        PRINT_ERROR("error while allocating optimizer->lineRenderers");
        goto ERROR;
    }

    for (size_t i = 0; i < workerAmount; ++i) {
        optimizer->lineRenderers[i] = lineRenderer_new(_optimizer_drawPixel);
        if (!optimizer->lineRenderers[i]) {
            char buffer[128];
            snprintf(
                buffer,
                sizeof(buffer),
                "error while constructing optimizer->lineRenderers[%ld]",
                i
            );
            PRINT_ERROR(buffer);
            goto ERROR;
        }
    }

    optimizer->lastBestImage = (Color*)malloc(imageSize * sizeof(Color));
    if (!optimizer->lastBestImage) {
        PRINT_ERROR("error while allocating optimizer->lastBestImage");
        goto ERROR;
    }

    optimizer->lastBestErrorImage = (uint64_t*)malloc(imageSize * sizeof(uint64_t));
    if (!optimizer->lastBestErrorImage) {
        PRINT_ERROR("error while allocating optimizer->lastBestErrorImage");
        goto ERROR;
    }

    DEBUG_PRINT("imageSize: %ld\n", imageSize);
    DEBUG_PRINT("pointAmount: %ld\n", indexer->pointAmount);
    optimizer->imageBuffer = (Color*)malloc(
        imageSize * indexer->pointAmount * sizeof(Color)
    );
    if (!optimizer->imageBuffer) {
        PRINT_ERROR("error while allocating optimizer->imageBuffer");
        goto ERROR;
    }

    optimizer->errorBuffer = (uint64_t*)malloc(
        imageSize * indexer->pointAmount * sizeof(uint64_t)
    );
    if (!optimizer->errorBuffer) {
        PRINT_ERROR("error while allocating optimizer->errorBuffer");
        goto ERROR;
    }

    optimizer->errors = (uint64_t*)malloc(
        indexer->pointAmount * sizeof(uint64_t)
    );
    if (!optimizer->errors) {
        PRINT_ERROR("error while allocating optimizer->errors");
        goto ERROR;
    }

    optimizer->lastBestPointIndices = (uint64_t*)malloc(
        indexer->threadAmount * sizeof(uint64_t)
    );
    if (!optimizer->lastBestPointIndices) {
        PRINT_ERROR("error while allocating optimizer->lastBestPointIndices");
        goto ERROR;
    }

    optimizer->possibleConnections = (uint64_t*)malloc(
        indexer->pointAmount * sizeof(uint64_t)
    );
    if (!optimizer->possibleConnections) {
        PRINT_ERROR("error while allocating optimizer->possibleConnections");
        goto ERROR;
    }

    optimizer->thicknessesInPixels = (double*)malloc(
        indexer->threadAmount * sizeof(double)
    );
    if (!optimizer->thicknessesInPixels) {
        PRINT_ERROR("error while allocating optimizer->thicknessesInPixels")
        goto ERROR;
    }

    optimizer->connectionIsDone = (bool**)malloc(
        indexer->pointAmount * sizeof(bool*)
    );
    if (!optimizer->connectionIsDone) {
        PRINT_ERROR("error while allocating optimizer->connectionIsDone");
        goto ERROR;
    }

    for (uint64_t i = 0; i < indexer->pointAmount; ++i) {
        optimizer->connectionIsDone[i] = (bool*)calloc(
            indexer->pointAmount, sizeof(bool)
        );
        if (!optimizer->connectionIsDone[i]) {
            char buffer[128];
            snprintf(
                buffer,
                sizeof(buffer),
                "error while allocating optimizer->connectionIsDone[%ld]",
                i
            );
            PRINT_ERROR(buffer);
            goto ERROR;
        }
    }

    DEBUG_EXIT_FUNC();
    return optimizer;

ERROR:
    optimizer_delete(optimizer);
    DEBUG_EXIT_FUNC();
    return NULL;
}

Optimizer * _optimizer_initialize(Optimizer *self, SharedData *sharedData) {
    DEBUG_ENTER_FUNC();
    const Indexer *indexer = &(sharedData->inputData.header->indexer);
    const uint64_t imageWidth = sharedData->inputData.header->imageWidth;
    const uint64_t imageRadius = imageWidth / 2;
    const Disc *disc = &(sharedData->inputData.header->disc);

    for (uint64_t i = 0; i < indexer->threadAmount; ++i) {
        const Thread *thread = &(sharedData->inputData.threads[i]);
        self->thicknessesInPixels[i] = (
            (double)(thread->thicknessInMicrometers)
            * (double)imageWidth
            / (double)(disc->radiusInMicrometers * 2)
        );
    }

    uint64_t errorSum = 0;
    for (uint64_t y = 0; y < imageWidth; ++y) {
        for (uint64_t x = 0; x < imageWidth; ++x) {
            if (x * x + y * y > imageRadius * imageRadius) continue;

            // draw background
            self->lastBestImage[imageWidth * y + x] = (
                sharedData->inputData.header->disc.backgroundColor
            );

            // calculate error
            const uint64_t error = color_weightedSquaredError(
                &sharedData->inputData.target[imageWidth * y + x],
                &sharedData->inputData.header->disc.backgroundColor,
                self->sharedData->inputData.importance[imageWidth * y + x]
            );

            errorSum += error;
            self->lastBestErrorImage[imageWidth * y + x] = error;
        }
    }
    self->lastBestError = errorSum;

    memcpy(
        (void*)self->lastBestPointIndices,
        (void*)self->sharedData->inputData.startPoints,
        indexer->threadAmount * sizeof(uint64_t)
    );

    DEBUG_EXIT_FUNC();
    return self;
}

Optimizer * optimizer_new(SharedData *sharedData) {
    DEBUG_ENTER_FUNC();
    Optimizer *result = _optimizer_initialize(
        _optimizer_construct(sharedData),
        sharedData
    );
    DEBUG_EXIT_FUNC();
    return result;
}

void optimizer_delete(Optimizer *self) {
    DEBUG_ENTER_FUNC();
    for (
        uint64_t i = 0;
        i < self->sharedData->inputData.header->indexer.pointAmount;
        ++i
    ) {
        free(self->connectionIsDone[i]);
    }
    free(self->connectionIsDone);
    free(self->thicknessesInPixels);
    free(self->possibleConnections);
    free(self->lastBestPointIndices);
    free(self->errors);
    free(self->errorBuffer);
    free(self->lastBestErrorImage);
    free(self->imageBuffer);
    free(self->lastBestImage);
    for (
        uint64_t i = 0;
        i < self->workerPool->workerAmount;
        ++i
    ) {
        lineRenderer_delete(self->lineRenderers[i]);
    }
    free(self->lineRenderers);
    workerPool_delete(self->workerPool);
    free(self);
    DEBUG_EXIT_FUNC();
}

typedef struct {
    Optimizer *optimizer;
    uint64_t pointIndex;
    size_t workerIndex;
} DrawPixelArgument;

void _optimizer_drawPixel(
    uint64_t x,
    uint64_t y,
    double intensity,
    void *argument
) {
    DEBUG_ENTER_FUNC();
    Optimizer *self = ((DrawPixelArgument*)argument)->optimizer;
    const uint64_t pointIndex = ((DrawPixelArgument*)argument)->pointIndex;
    const Thread *thread = &(
        self->sharedData->inputData.threads[self->currentThreadIndex]
    );
    const uint64_t imageWidth = (
        self->sharedData->inputData.header->imageWidth
    );
    const uint64_t imageSize = imageWidth * imageWidth;
    const uint64_t imageIndex = y * imageWidth + x;
    const uint64_t bufferIndex = pointIndex * imageSize + imageIndex;

    if (x < 0 || y < 0 || x >= imageWidth || y >= imageWidth) {
        DEBUG_EXIT_FUNC();
        return;
    }

    const Color oldColor = self->lastBestImage[imageIndex];
    const double alpha = (double)(thread->alpha) / (double)0xff;
    Color newColor = COLOR_NULL;
    color_mix(&oldColor, &(thread->color), alpha * intensity, &newColor);

    const Color targetColor = (
        self->sharedData->inputData.target[imageIndex]
    );

    const uint64_t newError = color_weightedSquaredError(
        &targetColor, &newColor,
        self->sharedData->inputData.importance[imageIndex]
    );
    const uint64_t oldError = self->lastBestErrorImage[imageIndex];

    self->errors[pointIndex] -= oldError;
    self->errors[pointIndex] += newError;
    self->errorBuffer[bufferIndex] = newError;
    self->imageBuffer[bufferIndex] = newColor;
    DEBUG_EXIT_FUNC();
}

void _optimizer_drawLine(
    Optimizer *optimizer,
    uint64_t workerIndex,
    uint64_t startIndex,
    uint64_t endIndex
) {
    DEBUG_ENTER_FUNC();
    DEBUG_PRINT("wid: %ld, s: %ld, e: %ld\n", workerIndex, startIndex, endIndex);
    const Indexer *indexer = &(
        optimizer->sharedData->inputData.header->indexer
    );
    const double startAngle = (
        TWO_PI * (double)startIndex / (double)(indexer->pointAmount)
    );
    const double endAngle = (
        TWO_PI * (double)endIndex / (double)(indexer->pointAmount)
    );
    const uint64_t imageWidth = optimizer->sharedData->inputData.header->imageWidth;
    const double imageRadius = (double)imageWidth / 2.0;
    const double x0 = cos(startAngle) * imageRadius + imageRadius;
    const double y0 = (double)imageWidth - (sin(startAngle) * imageRadius + imageRadius);
    const double x1 = cos(endAngle) * imageRadius + imageRadius;
    const double y1 = (double)imageWidth - (sin(endAngle) * imageRadius + imageRadius);

    lineRenderer_setArgument(
        optimizer->lineRenderers[workerIndex],
        (void*)&(DrawPixelArgument){
            .optimizer = optimizer,
            .pointIndex = endIndex,
            .workerIndex = workerIndex
        }
    );

    const double thicknessInPixels = (
        optimizer->thicknessesInPixels[optimizer->currentThreadIndex]
    );

    lineRenderer_draw(
        optimizer->lineRenderers[workerIndex],
        x0, y0, x1, y1, thicknessInPixels
    );

    DEBUG_EXIT_FUNC();
}

void _optimizer_optimizeTask(
    void *argument,
    size_t workerIndex,
    size_t workerAmount
) {
    DEBUG_ENTER_FUNC();
    Optimizer *self = (Optimizer *const)argument;
    for (
        uint64_t i = workerIndex * workerAmount;
        i < self->possibleConnectionAmount; ++i
    ) {
        const uint64_t startIndex = self->lastBestPointIndices[self->currentThreadIndex];
        const uint64_t endIndex = self->possibleConnections[i];

        _optimizer_drawLine(self, workerIndex, startIndex, endIndex);

        self->connectionIsDone[startIndex][endIndex] = true;
        self->connectionIsDone[endIndex][startIndex] = true;
    }
    DEBUG_EXIT_FUNC();
}

void _optimizer_storeDebugInformation(Optimizer *self) {
    DEBUG_ENTER_FUNC();
    const uint64_t imageWidth = self->sharedData->inputData.header->imageWidth;
    const uint64_t imageSize = imageWidth * imageWidth;
    const uint8_t debugFlags = self->sharedData->inputData.header->debugFlags;
    if (debugFlags & DEBUG_STORE_IMAGES) {
        memcpy(
            (void*)self->sharedData->outputData.debugData.images + self->currentIteration * imageSize * self->sharedData->inputData.header->indexer.pointAmount,
            (void*)self->imageBuffer,
            imageSize * self->sharedData->inputData.header->indexer.pointAmount
        );
    }
    if (debugFlags & DEBUG_STORE_ABSOLUTE_ERROR) {
        memcpy(
            (void*)self->sharedData->outputData.debugData.absoluteErrors + self->currentIteration * imageSize * self->sharedData->inputData.header->indexer.pointAmount,
            (void*)self->errorBuffer,
            imageSize * self->sharedData->inputData.header->indexer.pointAmount
        );
    }
    DEBUG_EXIT_FUNC();
}

void _optimizer_prepareIteration(Optimizer *self) {
    DEBUG_ENTER_FUNC();
    self->currentThreadIndex = self->sharedData->inputData.threadOrder[self->currentIteration % self->sharedData->inputData.header->threadOrderSize];
    self->possibleConnectionAmount = 0;
    if (self->sharedData->inputData.header->termination.flags & TERMINATE_ON_UNAVAILABLE_CONNECTION) {
        for (uint64_t i = 0, j = 0; i < self->sharedData->inputData.header->indexer.pointAmount; ++i) {
            if (self->connectionIsDone[j][self->lastBestPointIndices[self->currentThreadIndex]]) {
                continue;
            }
            if (i == self->lastBestPointIndices[self->currentThreadIndex]) {
                continue;
            }
            self->possibleConnections[j++] = i;
            ++(self->possibleConnectionAmount);
        }
    } else {
        for (uint64_t i = 0, j = 0; i < self->sharedData->inputData.header->indexer.pointAmount; ++i) {
            if (i == self->lastBestPointIndices[self->currentThreadIndex]) {
                continue;
            }
            self->possibleConnections[j++] = i;
            ++(self->possibleConnectionAmount);
        }
    }
    DEBUG_EXIT_FUNC();
}

void _optimizer_handleIterationResults(Optimizer *self) {
    DEBUG_ENTER_FUNC();
    const uint64_t imageWidth = self->sharedData->inputData.header->imageWidth;
    const uint64_t imageSize = imageWidth * imageWidth;
    uint64_t bestPointIndex = 0;
    uint64_t bestError = self->errors[0];
    for (uint64_t i = 1; i < self->sharedData->inputData.header->indexer.pointAmount; ++i) {
        if (self->errors[i] < bestError) {
            bestError = self->errors[i];
            bestPointIndex = i;
        }
    }
    self->sharedData->outputData.instructions[self->currentIteration] = (Instruction){
        .startIndex = self->lastBestPointIndices[self->currentThreadIndex],
        .endIndex = bestPointIndex,
        .threadIndex = self->currentThreadIndex
    };
    self->lastBestPointIndices[self->currentThreadIndex] = bestPointIndex;
    memcpy(
        (void*)self->lastBestImage,
        (void*)self->imageBuffer + imageSize * bestPointIndex,
        imageSize * sizeof(Color)
    );
    memcpy(
        (void*)self->lastBestErrorImage,
        (void*)self->errorBuffer + imageSize * bestPointIndex,
        imageSize * sizeof(uint64_t)
    );
    self->lastBestError = bestError;
    self->lastNormalizedError = self->currentNormalizedError;
    self->currentNormalizedError = bestError / imageSize;
    DEBUG_EXIT_FUNC();
}

bool _optimizer_preCheckTermination(Optimizer *self) {
    DEBUG_ENTER_FUNC();
    if (self->sharedData->inputData.header->termination.flags & TERMINATE_ON_UNAVAILABLE_CONNECTION) {
        if (self->possibleConnectionAmount == 0) {
            DEBUG_EXIT_FUNC();
            return true;
        }
    }
    DEBUG_EXIT_FUNC();
    return false;
}

bool _optimizer_postCheckTermination(Optimizer *self) {
    DEBUG_ENTER_FUNC();
    if (self->sharedData->inputData.header->termination.flags & TERMINATE_ON_MIN_RELATIVE_ERROR) {
        if (1.0 - (double)(self->currentNormalizedError) / (double)(self->lastNormalizedError) <= self->sharedData->inputData.header->termination.minRelativeError) {
            ++(self->relativeErrorStreak);
        } else {
            self->relativeErrorStreak = 0;
        }
        if (self->relativeErrorStreak == self->sharedData->inputData.header->termination.relativeErrorStreak) {
            DEBUG_EXIT_FUNC();
            return true;
        }
    }
    DEBUG_EXIT_FUNC();
    return false;
}

uint64_t _optimizer_mainloop(Optimizer *self) {
    DEBUG_ENTER_FUNC();
    for (
        self->currentIteration = 0;
        self->currentIteration < self->sharedData->inputData.header->termination.maxIterations;
        ++(self->currentIteration)
    ) {
        printf("%ld\n", self->currentIteration);
        _optimizer_prepareIteration(self);
        if (_optimizer_preCheckTermination(self)) {
            uint64_t result = self->currentIteration;
            DEBUG_EXIT_FUNC();
            return result;
        }
        workerPool_runTask(self->workerPool);
        _optimizer_handleIterationResults(self);
        if (self->sharedData->inputData.header->debugFlags) {
            _optimizer_storeDebugInformation(self);
        }

        char buffer[128];
        snprintf(buffer, sizeof(buffer), "images/lastBestImage_%ld.jpg", self->currentIteration);
        DEBUG_SAVE_IMAGE(
            buffer,
            self->lastBestImage
        );

        if (_optimizer_postCheckTermination(self)) {
            uint64_t result = self->currentIteration + 1;
            DEBUG_EXIT_FUNC();
            return result;
        }
    }
    uint64_t result = self->sharedData->inputData.header->termination.maxIterations;
    DEBUG_EXIT_FUNC();
    return result;
}

void _optimizer_writeOutputData(Optimizer *self, uint64_t iterationAmount) {
    DEBUG_ENTER_FUNC();
    const uint64_t imageWidth = self->sharedData->inputData.header->imageWidth;
    const uint64_t imageSize = imageWidth * imageWidth;
    self->sharedData->outputData.header->instructionAmount = iterationAmount;
    self->sharedData->outputData.header->absoluteError = self->lastBestError;
    self->sharedData->outputData.header->normalizedError = self->currentNormalizedError;
    memcpy(
        (void*)self->sharedData->outputData.result,
        (void*)self->lastBestImage,
        imageSize
    );
    DEBUG_EXIT_FUNC();
}

void optimizer_optimize(Optimizer *self) {
    DEBUG_ENTER_FUNC();
    workerPool_start(self->workerPool);
    workerPool_setTask(self->workerPool, _optimizer_optimizeTask);
    workerPool_setArgument(self->workerPool, (void*)self);
    uint64_t iterationAmount = _optimizer_mainloop(self);
    workerPool_stop(self->workerPool);
    _optimizer_writeOutputData(self, iterationAmount);
    DEBUG_EXIT_FUNC();
}
