#ifndef SHLL_H
#define SHLL_H

#include <time.h>
#include "hll.h"

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

/**
 * Adds a time/leading point to a register
 * @arg r The register to add the point to
 * @arg p The time/leading point to add to the register
 */
void shll_register_add_point(shll_t *h, shll_register *r, shll_point p);
#endif
