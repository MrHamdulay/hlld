#include <assert.h>
#include "shll.h"

#define NUM_REG(precision) ((1 << precision))
#define INT_CEIL(num, denom) (((num) + (denom) - 1) / (denom))


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
}

/**
 * Adds a new hash to the SHLL
 * @arg h The hll to add to
 * @arg hash The hash to add
 */
void shll_add_hash(hll_t *h, uint64_t hash);
