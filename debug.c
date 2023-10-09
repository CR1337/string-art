#include <stdbool.h>

#include "debug.h"
#include "string.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define __USE_GNU
#include <sched.h>

static int _debug_funcDepth = 0;
static int _debug_workerFuncDepth[128] = { 0 };
static uint64_t _debug_imageWidth = 0;
static uint64_t _debug_imageSize = 0;
static bool _debug_workerMode = false;

int _debug_workerId() {
    return sched_getcpu();
}

void _debug_printIndented(int indentation, const char string[]) {
    char indentationBuffer[128] = { '\0' };
    memset(indentationBuffer, '\t', indentation);
    printf("%s%s", indentationBuffer, string);
}

void debug_enterMainThreadMode() {
    _debug_workerMode = false;
}

void debug_enterWorkerMode() {
    _debug_workerMode = true;
}

void debug_enterFunc(const char funcName[]) {
#ifdef VERBOSE
    char buffer[2048];
    if (_debug_workerMode) {
        snprintf(buffer, sizeof(buffer), "(worker-%d):::%s\n", _debug_workerId(), funcName);
        _debug_printIndented(_debug_workerFuncDepth[_debug_workerId()], buffer);
    } else {
        snprintf(buffer, sizeof(buffer), "(mainThread):::%s\n", funcName);
        _debug_printIndented(_debug_funcDepth, buffer);
    }
#endif // VERBOSE
    if (_debug_workerMode) {
        ++_debug_workerFuncDepth[_debug_workerId()];
    } else {
        ++_debug_funcDepth;
    }
}

void debug_exitFunc(int lineNumber) {
    if (_debug_workerMode) {
        --_debug_workerFuncDepth[_debug_workerId()];
    } else {
        --_debug_funcDepth;
    }
#ifdef VERBOSE
    char buffer[32];
    if (_debug_workerMode) {
        snprintf(buffer, sizeof(buffer), "<--(L:%d, W:%d)\n", lineNumber, _debug_workerId());
        _debug_printIndented(_debug_workerFuncDepth[_debug_workerId()], buffer);
    } else {
        snprintf(buffer, sizeof(buffer), "<--(L:%d)\n", lineNumber);
        _debug_printIndented(_debug_funcDepth, buffer);
    }
#endif // VERBOSE
}

void debug_setImageWidth(uint64_t imageWidth) {
    _debug_imageWidth = imageWidth;
    _debug_imageSize = imageWidth * imageWidth;
}

void debug_saveImage(const char filename[], Color *data) {
#ifdef IMAGES
    Color imageData[_debug_imageSize];
    for (uint64_t i = 0; i < _debug_imageSize; ++i) {
        imageData[i].c = 0xff - data[i].c;
        imageData[i].m = 0xff - data[i].m;
        imageData[i].y = 0xff - data[i].y;
    }
    stbi_write_jpg(
        filename,
        _debug_imageWidth,
        _debug_imageWidth,
        3,
        imageData,
        100
    );
#endif // IMAGES
}

void debug_save_ImageWithNumber(const char filename[], Color *data, int number) {
#ifdef IMAGES
    char buffer[512];
    snprintf(buffer, sizeof(buffer), number);
    debug_saveImage(buffer, data);
#endif
}