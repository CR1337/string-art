#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "color.h"

#ifdef DEBUG

void debug_enterMainThreadMode();
void debug_enterWorkerMode();
void debug_enterFunc(const char funcName[]);
void debug_exitFunc(int lineNumber);
void debug_setImageWidth(uint64_t imageWidth);
void debug_saveImage(const char filename[], Color *data);
void debug_save_ImageWithNumber(const char filename[], Color *data, int number);

#define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#define DEBUG_ENTER_MAIN_THREAD_MODE() debug_enterMainThreadMode()
#define DEBUG_ENTER_WORKER_MODE() debug_enterWorkerMode()
#define DEBUG_ENTER_FUNC() debug_enterFunc(__func__)
#define DEBUG_EXIT_FUNC() debug_exitFunc(__LINE__)
#define DEBUG_SET_IMAGE_WIDTH(imageWidth) debug_setImageWidth(imageWidth)
#define DEBUG_SAVE_IMAGE(filename, data) debug_saveImage(filename, data)
#define DEBUG_SAVE_IMAGE_WITH_NUMBER(filename, data, number) debug_save_ImageWithNumber(filename, data, number)
#define DEBUG_ASSERT(assertion, message) assert(assertion && message)

#else // DEBUG

#define DEBUG_PRINT(format, ...)
#define DEBUG_ENTER_MAIN_THREAD_MODE()
#define DEBUG_ENTER_WORKER_MODE()
#define DEBUG_ENTER_FUNC()
#define DEBUG_EXIT_FUNC()
#define DEBUG_SET_IMAGE_WIDTH(imageWidth)
#define DEBUG_SAVE_IMAGE(filename, data)
#define DEBUG_SAVE_IMAGE_WITH_NUMBER(filename, data, number)
#define DEBUG_ASSERT(assertion, message)

#endif // DEBUG

#endif // __DEBUG_H__
