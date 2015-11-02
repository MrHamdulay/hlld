#include <assert.h>
#include "shll.h"

#define NUM_REG(precision) ((1 << precision))
#define INT_CEIL(num, denom) (((num) + (denom) - 1) / (denom))

#define GROWTH_FACTOR 1.5


/**
 * Initializes a new SHLL
 * @arg precision The digits of precision to use
 * @arg window_period the length of time we store samples for
 * @arg window_precision smallest amount of time we distinguish
 * @arg h the SHLL to initialize
 */
int shll_init(unsigned char precision, int window_period, int window_precision, hll_t *hy) {
    if (precision < HLL_MIN_PRECISION || precision > HLL_MAX_PRECISION)
        return -1;

    hy->type = SLIDING;
    shll_t *h = &(hy->sliding);

    // Store parameters
    h->precision = precision;
    h->window_period = window_period;
    h->window_precision = window_precision;

    // Determine how many registers are needed
    int reg = NUM_REG(precision);

    h->registers = calloc(reg, sizeof(shll_register));

    return 0;
}

/**
 * Destroys an shll. Closes the bitmap, but does not free it.
 * @return 0 on success
 */
int shll_destroy(hll_t *hy) {
    assert(hy->type == SLIDING);
    free(hy->sliding.registers);
    hy->sliding.registers = NULL;
    return 0;
}

/**
 * Adds a new hash to the SHLL
 * @arg h The hll to add to
 * @arg hash The hash to add
 */
void shll_add_hash(hll_t *hy, uint64_t hash) {
    assert(hy->type == SLIDING);
    shll_t *h = &(hy->sliding);

    // Determine the index using the first p bits
    int idx = hash >> (64 - h->precision);

    // Shift out the index bits
    hash = hash << h->precision | (1 << (h->precision -1));

    // Determine the count of leading zeros
    int leading = __builtin_clzll(hash) + 1;

    shll_point p = {time(NULL), leading};
    shll_register *r = &h->registers[idx];

    shll_register_add_point(&hy->sliding, r, p);
}

/**
 * Remove a point from a register
 * @arg r register to remove
 * @arg idx index of the point to remove
 */
void shll_register_remove_point(shll_register *r, size_t idx) {
    r->size--;
    r->points[idx] = r->points[r->size];

    // TODO shrink array when below a certain bound
}

/**
 * Adds a time/leading point to a register
 * @arg r The register to add the point to
 * @arg p The time/leading point to add to the register
 */
void shll_register_add_point(shll_t *h, shll_register *r, shll_point p) {
    // remove all points with smaller register value or that have expired.
    time_t max_time = p.timestamp - h->window_period;
    for (size_t i=0; i<r->size; i++) {
        if (r->points[i].register_ <= p.register_ ||
            r->points[i].timestamp < max_time) {
            shll_register_remove_point(r, i);
        }
    }

    r->size++;

    // if we have exceeded capacity we resize
    if(r->size > r->capacity) {
        r->capacity = (size_t)(GROWTH_FACTOR * r->capacity + 1);
        realloc(r->points, r->capacity);
    }

    // add point to register
    r->points[r->size-1] = p;
}
