#ifndef __SHARED_DATA_H__
#define __SHARED_DATA_H__

#include <stdint.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <stdbool.h>

#include "color.h"

#define DEBUG_STORE_IMAGES (0b00000001)
#define DEBUG_STORE_ABSOLUTE_ERROR (0b00000010)

#define TERMINATE_ON_MIN_RELATIVE_ERROR (0b00000001)
#define TERMINATE_ON_UNAVAILABLE_CONNECTION (0b00000010)

#pragma region InputData

#pragma pack(1)
typedef struct {
    uint64_t radiusInMicrometers;
    Color backgroundColor;
} Disc;

#pragma pack(1)
typedef struct {
    uint64_t pointAmount;
    uint64_t threadAmount;
} Indexer;

#pragma pack(1)
typedef struct {
    uint8_t flags;
    uint64_t maxIterations;
    double minRelativeError;
    uint64_t relativeErrorStreak;
} Termination;

#pragma pack(1)
typedef struct {
    uint64_t imageWidth;
    uint64_t threadOrderSize;
    uint8_t debugFlags;
    Disc disc;
    Indexer indexer;
    Termination termination;
} InputHeader;

#pragma pack(1)
typedef struct {
    uint8_t alpha;
    uint64_t thicknessInMicrometers;
    Color color;
} Thread;

#pragma pack(1)
typedef struct {
    InputHeader *header;
    Thread *threads;
    uint64_t *threadOrder;
    uint64_t *startPoints;
    Color *target;
    double *importance;
} InputData;

#pragma endregion

#pragma region OutputData

#pragma pack(1)
typedef struct {
    uint64_t instructionAmount;
    uint64_t absoluteError;
    double normalizedError;
} OutputHeader;

#pragma pack(1)
typedef struct {
    uint64_t startIndex;
    uint64_t endIndex;
    uint64_t threadIndex;
} Instruction;

#pragma pack(1)
typedef struct {
    Color *images;
    uint64_t *absoluteErrors;
} DebugData;

#pragma pack(1)
typedef struct {
    OutputHeader *header;
    Color *result;
    Instruction *instructions;
    DebugData debugData;
} OutputData;

#pragma endregion

#pragma pack(1)
typedef struct {
    void *memory;
    InputData inputData;
    OutputData outputData;
} SharedData;

bool sharedData_attach(SharedData *sharedData, key_t key, size_t size);
bool sharedData_detach(SharedData *sharedData);

#endif // __SHARED_DATA_H__
