#ifndef SHLL_H
#define SHLL_H

#include <time.h>
#include "hll.h"

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
int shll_init(unsigned char precision, int window_period, int window_precision, hll_t *h);

/**
 * Destroys an shll. Closes the bitmap, but does not free it.
 * @return 0 on success
 */
int shll_destroy(hll_t *hy);

/**
 * Adds a new hash to the SHLL
 * @arg h The hll to add to
 * @arg hash The hash to add
 */
void shll_add_hash(hll_t *h, uint64_t hash);

/**
 * Estimates the cardinality of the SHLL
 * @arg h The hll to query
 * @return An estimate of the cardinality
 */
double shll_size(hll_t *h);
#endif
