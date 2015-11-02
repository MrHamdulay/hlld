#ifndef SHLL_H
#define SHLL_H

#include <time.h>

typedef struct {
    time_t timestamp;
    uint32_t register_;
} shll_point;

typedef struct {
    size_t size;
    size_t capacity;
    shll_point *points;
} shll_register;

typedef struct {
    // amount of seconds worth of samples we store (in seconds)
    int window_period;
    // precision to which we keep samples (in seconds)
    int window_precision;
    unsigned char precision;
    shll_register *registers;
} shll_t;

/**
 * Initializes a new SHLL
 * @arg precision The digits of precision to use
 * @arg window_period the length of time we store samples for
 * @arg window_precision smallest amount of time we distinguish
 * @arg h the SHLL to initialize
 */
int shll_init(unsigned char precision, int window_period, int window_precision, shll_t *h);

#endif
