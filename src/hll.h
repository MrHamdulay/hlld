#include <stdint.h>
#include <time.h>
#include "bitmap.h"

#ifndef HLL_H
#define HLL_H

#define NUM_REG(precision) ((1 << precision))

// Ensure precision in a sane bound
#define HLL_MIN_PRECISION 4      // 16 registers
#define HLL_MAX_PRECISION 18     // 262,144 registers

#define HLL_SPARSE 0
#define HLL_DENSE 1

typedef struct {
    time_t timestamp;
    long register_;
} hll_dense_point;

typedef struct {
    time_t timestamp;
    uint64_t hash;
} hll_sparse_point;

typedef struct {
    long size;
    long capacity;
    hll_dense_point *points;
} hll_register;

typedef struct {
    unsigned char representation;
    unsigned char precision;
    // amount of seconds worth of samples we store (in seconds)
    int window_period;
    // precision to which we keep samples (in seconds)
    int window_precision;
    union {
        hll_register *dense_registers;
        hll_sparse_point *sparse_points;
    };
} hll_t;

/**
 * Initializes a new SHLL
 * @arg precision The digits of precision to use
 * @arg window_period the length of time we store samples for
 * @arg window_precision smallest amount of time we distinguish
 * @arg h the SHLL to initialize
 */
int hll_init(unsigned char precision, int window_period, int window_precision, hll_t *h);

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
void hll_add_at_time(hll_t *h, char *key, time_t time);

/**
 * Adds a new hash to the HLL
 * @arg h The hll to add to
 * @arg hash The hash to add
 */
void hll_add_hash_at_time(hll_t *h, uint64_t hash, time_t time);

/**
 * Estimates the cardinality of the HLL
 * @arg h The hll to query
 * @return An estimate of the cardinality
 */
double hll_size(hll_t *h, time_t time_length, time_t current_time);
double hll_size_total(hll_t *h);

/**
 * Takes the union of a few sets and returns the cardinality
 */
double hll_union_size(hll_t **hs, int num_hs, time_t time_length, time_t current_time);

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

/**
 * Adds a time/leading point to a register
 * @arg r The register to add the point to
 * @arg p The time/leading point to add to the register
 */
void hll_register_add_point(hll_t *h, hll_register *r, hll_dense_point p);

int hll_get_register(hll_t *h, int register_index, time_t time_length, time_t current_time);

#endif
