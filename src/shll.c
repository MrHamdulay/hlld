#include <assert.h>
#include <syslog.h>
#include <stdio.h>
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
    for(int i=0; i<NUM_REG(hy->sliding.precision); i++) {
        if(hy->sliding.registers[i].points != NULL) {
            free(hy->sliding.registers[i].points);
        }
    }
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
    assert(r->size >= 0);
    r->points[idx] = r->points[r->size];

    // shrink array when below a certain bound
    if (r->size*GROWTH_FACTOR*GROWTH_FACTOR < r->capacity) {
        r->capacity = r->capacity/GROWTH_FACTOR+1;
        assert(r->capacity > r->size);
        r->points = realloc(r->points, r->capacity*sizeof(shll_point));
    }
}

/**
 * Adds a time/leading point to a register
 * @arg r The register to add the point to
 * @arg p The time/leading point to add to the register
 */
void shll_register_add_point(shll_t *h, shll_register *r, shll_point p) {
    // remove all points with smaller register value or that have expired.
    time_t max_time = p.timestamp - h->window_period;
    // do this in reverse order because we remove points from the right end
    for (int i=r->size-1; i>=0; i--) {
        if (r->points[i].register_ <= p.register_ ||
            r->points[i].timestamp <= max_time) {
            shll_register_remove_point(r, i);
        }
    }

    r->size++;

    // if we have exceeded capacity we resize
    if(r->size > r->capacity) {

        r->capacity = (size_t)(GROWTH_FACTOR * r->capacity + 1);
        r->points = realloc(r->points, r->capacity*sizeof(shll_point));
    }

    assert((long long)r->size-1 >= 0);
    assert(r->points != NULL);
    // add point to register
    r->points[r->size-1] = p;
}

int shll_get_register(hll_t *h, int register_index, int time_length, time_t current_time) {
    assert(h->type == SLIDING);
    assert(register_index < NUM_REG(h->sliding.precision) && register_index >= 0);
    shll_register *r = &h->sliding.registers[register_index];

    time_t min_time = current_time - time_length;
    int register_value = 0;

    for(size_t i=0; i<r->size; i++) {
        if (r->points[i].timestamp > min_time && r->points[i].register_ > register_value) {
            register_value = r->points[i].register_;
        }
    }

    return register_value;
}
