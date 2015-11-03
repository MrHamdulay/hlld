#include <stdint.h>
#include "bitmap.h"
#include "shll.h"

#ifndef HLL_H
#define HLL_H

// Ensure precision in a sane bound
#define HLL_MIN_PRECISION 4      // 16 registers
#define HLL_MAX_PRECISION 18     // 262,144 registers

typedef struct {
    uint32_t *registers;
    hlld_bitmap *bm;
} hll_t_normal;

typedef enum { NORMAL, SLIDING } hll_type;

typedef struct {
    time_t timestamp;
    int register_;
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
    shll_register *registers;
} shll_t;


typedef struct {
    unsigned char precision;
    hll_type type;
    union {
        hll_t_normal normal;
        shll_t sliding;
    };
} hll_t;

/**
 * Initializes a new HLL
 * @arg precision The digits of precision to use
 * @arg h The HLL to initialize
 * @return 0 on success
 */
int hll_init(unsigned char precision, hll_t *h);

/**
 * Initializes a new HLL from a bitmap
 * @arg precision The digits of precision to use
 * @arg bm The bitmap to use
 * @arg h The HLL to initialize
 * @return 0 on success
 */
int hll_init_from_bitmap(unsigned char precision, hlld_bitmap *bm, hll_t *h);

/**
 * Destroys an hll. Closes the bitmap, but does not free it.
 * @return 0 on success
 */
int hll_destroy(hll_t *h);

/**
 * Adds a new key to the HLL
 * @arg h The hll to add to
 * @arg key The key to add
 */
void hll_add(hll_t *h, char *key);

/**
 * Adds a new hash to the HLL
 * @arg h The hll to add to
 * @arg hash The hash to add
 */
void hll_add_hash(hll_t *h, uint64_t hash);

/**
 * Estimates the cardinality of the HLL
 * @arg h The hll to query
 * @return An estimate of the cardinality
 */
double hll_size(hll_t *h);

/**
 * Computes the minimum digits of precision
 * needed to hit a target error.
 * @arg error The target error rate
 * @return The number of digits needed, or
 * negative on error.
 */
int hll_precision_for_error(double err);

/**
 * Computes the upper bound on variance given
 * a precision
 * @arg prec The precision to use
 * @return The expected variance in the count,
 * or zero on error.
 */
double hll_error_for_precision(int prec);

/**
 * Computes the bytes required for a HLL of the
 * given precision.
 * @arg prec The precision to use
 * @return The bytes required or 0 on error.
 */
uint64_t hll_bytes_for_precision(int prec);

/*
 * Returns the bias correctors from the
 * hyperloglog paper
 */
double hll_alpha(unsigned char precision);

/*
 * Estimates cardinality using a linear counting.
 * Used when some registers still have a zero value.
 */
double hll_linear_count(hll_t *hu, int num_zero);

/**
 * Binary searches for the nearest matching index
 * @return The matching index, or closest match
 */
int binary_search(double val, int num, const double *array);

/**
 * Interpolates the bias estimate using the
 * empircal data collected by Google, from the
 * paper mentioned above.
 */
double hll_bias_estimate(hll_t *hu, double raw_est);

#endif
