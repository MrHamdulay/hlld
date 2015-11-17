/*
 * Based on the Google paper
 * "HyperLogLog in Practice: Algorithmic Engineering of a
State of The Art Cardinality Estimation Algorithm"
 *
 * We implement a HyperLogLog using 6 bits for register,
 * and a 64bit hash function. For our needs, we always use
 * a dense representation and avoid the sparse/dense conversions.
 *
 */
#include <syslog.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "hll.h"
#include "hll_constants.h"

#define REG_WIDTH 6     // Bits per register
#define INT_WIDTH 32    // Bits in an int
#define REG_PER_WORD 5  // floor(INT_WIDTH / REG_WIDTH)
#define GROWTH_FACTOR 1.5

#define INT_CEIL(num, denom) (((num) + (denom) - 1) / (denom))

// Link the external murmur hash in
extern void MurmurHash3_x64_128(const void * key, const int len, const uint32_t seed, void *out);


/**
 * Initializes a new SHLL
 * @arg precision The digits of precision to use
 * @arg window_period the length of time we store samples for
 * @arg window_precision smallest amount of time we distinguish
 * @arg h the SHLL to initialize
 */
int hll_init(unsigned char precision, int window_period, int window_precision, hll_t *h) {
    if (precision < HLL_MIN_PRECISION || precision > HLL_MAX_PRECISION || 
        window_period <= 0 || window_precision <= 0) {
        return -1;
    }

    // Store parameters
    h->representation = HLL_SPARSE;
    h->precision = precision;
    h->window_period = window_period;
    h->window_precision = window_precision;

    h->sparse = (hll_sparse*)malloc(sizeof(hll_sparse));
    h->sparse->points = NULL;
    //h->dense_registers = (hll_register*)calloc(reg, sizeof(hll_register));

    return 0;
}


/**
 * Destroys an hll. Closes the bitmap, but does not free it.
 * @return 0 on success
 */
int hll_destroy(hll_t *h) {
    if (h->representation == HLL_DENSE) {
        for(int i=0; i<NUM_REG(h->precision); i++) {
            if(h->dense_registers[i].points != NULL) {
                free(h->dense_registers[i].points);
            }
        }
        free(h->dense_registers);
        h->dense_registers = NULL;
    } else {
        free(h->sparse->points);
        free(h->sparse);
    }
    return 0;
}

/**
 * Adds a new key to the HLL
 * @arg h The hll to add to
 * @arg key The key to add
 */
/*void hll_add(hll_t *h, char *key) {
    // Compute the hash value of the key
    uint64_t out[2];
    MurmurHash3_x64_128(key, strlen(key), 0, &out);

    // Add the hashed value
    hll_add_hash(h, out[1]);
}*/
void hll_add_at_time(hll_t *h, char *key, time_t time) {
    // Compute the hash value of the key
    uint64_t out[2];
    MurmurHash3_x64_128(key, strlen(key), 0, &out);

    // Add the hashed value
    hll_add_hash_at_time(h, out[1], time);
}

/**
 * Remove a point from a register
 * @arg r register to remove
 * @arg idx index of the point to remove
 */
void hll_register_remove_point(hll_register *r, size_t idx) {
    r->size--;
    assert(r->size >= 0);
    r->points[idx] = r->points[r->size];

    // shrink array when below a certain bound
    if (r->size*GROWTH_FACTOR*GROWTH_FACTOR < r->capacity) {
        r->capacity = r->capacity/GROWTH_FACTOR+1;
        assert(r->capacity > r->size);
        r->points = (hll_dense_point*)realloc(r->points, r->capacity*sizeof(hll_dense_point));
    }
}

/**
 * Adds a time/leading point to a register
 * @arg r The register to add the point to
 * @arg p The time/leading point to add to the register
 */
void hll_register_add_point(hll_t *h, hll_register *r, hll_dense_point p) {
    // remove all points with smaller register value or that have expired.
    long long max_time = p.timestamp - h->window_period/ h->window_precision;
    // do this in reverse order because we remove points from the right end
    for (int i=r->size-1; i>=0; i--) {
        if (r->points[i].register_ <= p.register_ ||
            r->points[i].timestamp <= max_time) {
            hll_register_remove_point(r, i);
        }
    }

    r->size++;

    // if we have exceeded capacity we resize
    if(r->size > r->capacity) {

        r->capacity = (size_t)(GROWTH_FACTOR * r->capacity + 1);
        r->points = (hll_dense_point*)realloc(r->points, r->capacity*sizeof(hll_dense_point));
    }

    assert((long long)r->size-1 >= 0);
    assert(r->points != NULL);
    // add point to register
    r->points[r->size-1] = p;
}

int hll_get_register(hll_t *h, int register_index, time_t time_length, time_t current_time) {
    hll_register *r = &h->dense_registers[register_index];

    time_t min_time = current_time - time_length/h->window_precision;
    int register_value = 0;

    for(int i=0; i<r->size; i++) {
        if (r->points[i].timestamp > min_time && r->points[i].register_ > register_value) {
            register_value = r->points[i].register_;
        }
    }

    return register_value;
}

void convert_dense(hll_t *h) {
    // converting an already converted repr is a no-op
    if (h->representation == HLL_DENSE)
        return;
    h->representation = HLL_DENSE;
    h->dense_registers = (hll_register*)calloc(NUM_REG(h->precision), sizeof(hll_register));

    for(int i=0; i<h->sparse->size; i++) {
        hll_add_hash_at_time(h, h->sparse->points[i].hash, h->sparse->points[i].timestamp);
    }
    free(h->sparse->points);
    free(h->sparse);
}

void hll_sparse_remove_point(hll_t *h, size_t i) {
    h->sparse->points[i] = h->sparse->points[h->sparse->size];
    h->sparse->size--;
}

void hll_sparse_add_point(hll_t *h, uint64_t hash, time_t time_added) {
    hll_sparse *sparse = h->sparse;
    if (sparse->points == NULL) {
        sparse->capacity = 4;
        sparse->points = (hll_sparse_point*)calloc(sparse->capacity, sizeof(hll_sparse_point));
    }

    int register_idx = hash >> (64 - h->precision);

    // Shift out the index bits
    uint64_t hash2 = hash << h->precision | (1 << (h->precision -1));

    // Determine the count of leading zeros
    int leading_insert = __builtin_clzll(hash2) + 1;
    
    // remove all points with smaller register value or that have expired.
    // it's the worst that this is duplicated code....
    long long max_time = time_added - h->window_period/ h->window_precision;
    // do this in reverse order because we remove points from the right end
    for (int i=sparse->size; i>=0; i--) {
        uint64_t other_hash = sparse->points[i].hash;
        // if we are inserting the same value again just break out
        if (other_hash == hash)
            return;

        int register_index_other = other_hash >> (64 - h->precision);
        other_hash = other_hash << h->precision | (1 << (h->precision -1));
        int leading_other = __builtin_clzll(other_hash)+1;
        
        // if it's the same register, the time is out of the max time bound 
        // or the leading values are smaller then remove
        if (register_idx == register_index_other &&
            (
            leading_other <= leading_insert ||
            sparse->points[i].timestamp <= max_time
            )) {
            hll_sparse_remove_point(h, i);
        }
    }

    if (sparse->size+1 >= h->sparse->capacity) {
        sparse->capacity *= 1.5;

        // if increasing the capacity takes us over the limit convert to dense representation
        int max_size = 0;
        // convert to the dense representation
        if (sparse->capacity > max_size) {
            convert_dense(h);
            assert(h->representation == HLL_DENSE);
            hll_add_hash_at_time(h, hash, time_added);
            return;
        }
        // increase size of array
        sparse->points = (hll_sparse_point*)realloc(
                sparse->points,
                sparse->capacity*sizeof(hll_sparse_point));
    }
    sparse->points[sparse->size].hash = hash;
    sparse->points[sparse->size].timestamp = time_added;
    sparse->size++;
}

/**
 * Adds a new hash to the SHLL
 * @arg h The hll to add to
 * @arg hash The hash to add
 */
void hll_add_hash_at_time(hll_t *h, uint64_t hash, time_t time_added) {
    if (h->representation == HLL_DENSE) {
        // Determine the index using the first p bits
        int idx = hash >> (64 - h->precision);

        // Shift out the index bits
        hash = hash << h->precision | (1 << (h->precision -1));

        // Determine the count of leading zeros
        int leading = __builtin_clzll(hash) + 1;

        hll_dense_point p = {time_added, leading};
        hll_register *r = &h->dense_registers[idx];

        hll_register_add_point(h, r, p);
    } else {
        hll_sparse_add_point(h, hash, time_added);
    }
}

/*
 * Computes the raw cardinality estimate
 */
static double hll_raw_estimate_union(hll_t **h, int num_hls, int *num_zero, time_t time_length, time_t current_time) {
    unsigned char precision = h[0]->precision;
    int num_reg = NUM_REG(precision);
    double multi = hll_alpha(precision) * num_reg * num_reg;

    double inv_sum = 0;
    for (int i=0; i < num_reg; i++) {
        int reg_val = 0;
        for(int j=0; j<num_hls; j++) {
            int reg = hll_get_register(h[j], i, time_length, current_time);
            if (reg > reg_val)
                reg_val = reg;
        }
        inv_sum += pow(2.0, -1 * reg_val);
        if (!reg_val) *num_zero += 1;
    }
    return multi * (1.0 / inv_sum);
}
/*
 * Returns the bias correctors from the
 * hyperloglog paper
 */
double hll_alpha(unsigned char precision) {
    switch (precision) {
        case 4:
            return 0.673;
        case 5:
            return 0.697;
        case 6:
            return 0.709;
        default:
            return 0.7213 / (1 + 1.079 / NUM_REG(precision));
    }
}

/*
 * Estimates cardinality using a linear counting.
 * Used when some registers still have a zero value.
 */
double hll_linear_count(hll_t *hu, int num_zero) {
    int registers = NUM_REG(hu->precision);
    return registers *
        log((double)registers / (double)num_zero);
}

/**
 * Binary searches for the nearest matching index
 * @return The matching index, or closest match
 */
int binary_search(double val, int num, const double *array) {
    int low=0, mid, high=num-1;
    while (low < high) {
        mid = (low + high) / 2;
        if (val > array[mid]) {
            low = mid + 1;
        } else if (val == array[mid]) {
            return mid;
        } else {
            high = mid - 1;
        }
    }
    return low;
}

/**
 * Interpolates the bias estimate using the
 * empircal data collected by Google, from the
 * paper mentioned above.
 */
double hll_bias_estimate(hll_t *hu, double raw_est) {
    // Determine the samples available
    int samples;
    int precision = hu->precision;
    switch (precision) {
        case 4:
            samples = 80;
            break;
        case 5:
            samples = 160;
            break;
        default:
            samples = 200;
            break;
    }

    // Get the proper arrays based on precision
    double *estimates = *(rawEstimateData+(precision-4));
    double *biases = *(biasData+(precision-4));

    // Get the matching biases
    int idx = binary_search(raw_est, samples, estimates);
    if (idx == 0)
        return biases[0];
    else if (idx == samples)
        return biases[samples-1];
    else
        return (biases[idx] + biases[idx-1]) / 2;
}

/**
 * Computes the minimum number of registers
 * needed to hit a target error.
 * @arg error The target error rate
 * @return The number of registers needed, or
 * negative on error.
 */
int hll_precision_for_error(double err) {
    // Check that the error bound is sane
    if (err >= 1 || err <= 0)
        return -1;

    /*
     * Error of HLL is 1.04 / sqrt(m)
     * m is given by 2^p, so solve for p,
     * and use the ceiling.
     */
    double p = log2(pow(1.04 / err, 2));
    return ceil(p);
}


/**
 * Computes the upper bound on variance given
 * a precision
 * @arg prec The precision to use
 * @return The expected variance in the count,
 * or zero on error.
 */
double hll_error_for_precision(int prec) {
    // Check that the error bound is sane
    if (prec < HLL_MIN_PRECISION || prec > HLL_MAX_PRECISION)
        return 0;

    /*
     * Error of HLL is 1.04 / sqrt(m)
     * m is given by 2^p
     */
    int registers = pow(2, prec);
    return 1.04 / sqrt(registers);
}


/**
 * Computes the bytes required for a HLL of the
 * given precision.
 * @arg prec The precision to use
 * @return The bytes required or 0 on error.
 */
uint64_t hll_bytes_for_precision(int prec) {
    // Check that the error bound is sane
    if (prec < HLL_MIN_PRECISION || prec > HLL_MAX_PRECISION)
        return 0;

    // Determine how many registers are needed
    int reg = NUM_REG(prec);

    // Get the full words required
    int words = INT_CEIL(reg, REG_PER_WORD);

    // Convert to byte size
    return words * sizeof(uint32_t);
}

/**
 * Estimates the cardinality of the HLL
 * @arg h The hll to query
 * @return An estimate of the cardinality
 */
double hll_size(hll_t *h, time_t time_length, time_t current_time) {
    if (h->representation == HLL_DENSE) {
        int num_zero = 0;
        hll_t *hs[] = {h};
        double raw_est = hll_raw_estimate_union(hs, 1, &num_zero, time_length, current_time);

        // Check if we need to apply bias correction
        int num_reg = NUM_REG(h->precision);
        if (raw_est <= 5 * num_reg) {
            raw_est -= hll_bias_estimate(h, raw_est);
        }

        // Check if linear counting should be used
        double alt_est;
        if (num_zero) {
            alt_est = hll_linear_count(h, num_zero);
        } else {
            alt_est = raw_est;
        }

        // Determine which estimate to use
        if (alt_est <= switchThreshold[h->precision-4]) {
            return alt_est;
        } else {
            return raw_est;
        }
    } else {
        double size = 0;
        for(int i=0; i<h->sparse->size; i++) {
            if (h->sparse->points[i].timestamp >= current_time - time_length &&
                h->sparse->points[i].timestamp <= current_time) {
                size++;
            }
        }
        return size;
    }
}

double hll_size_total(hll_t *h) {
    time_t ctime = time(NULL);
    return hll_size(h, ctime, ctime);
}


/**
 * Takes the union of a few sets and returns the cardinality
 *
 * returns -1 when the precision of all the hll's do not match
 */
double hll_union_size(hll_t **hs, int num_hs, time_t time_length, time_t current_time) {
    // the precision of each hll needs to be the same
    int precision = (int)hs[0]->precision;
    for(int i=1; i<num_hs; i++) {
        if (hs[i]->precision != (int)precision) {
            return -2;
        }
    }
    int num_zero = 0;
    double raw_est = hll_raw_estimate_union(hs, num_hs, &num_zero, time_length, current_time);

    // Check if we need to apply bias correction
    int num_reg = NUM_REG(hs[0]->precision);
    if (raw_est <= 5 * num_reg) {
        raw_est -= hll_bias_estimate(hs[0], raw_est);
    }

    // Check if linear counting should be used
    double alt_est;
    if (num_zero) {
        alt_est = hll_linear_count(hs[0], num_zero);
    } else {
        alt_est = raw_est;
    }

    // Determine which estimate to use
    if (alt_est <= switchThreshold[hs[0]->precision-4]) {
        return alt_est;
    } else {
        return raw_est;
    }
}
