#include <stdlib.h>
#include <errno.h>

/**
 * Attempts to convert a string to an integer (64bit),
 * and write the value out.
 * @arg val The string value
 * @arg result The destination for the result
 * @return 0 on success, 0 on error.
 */
int value_to_int64(const char *val, uint64_t *result) {
    long long res = strtoll(val, NULL, 10);
    if (res == 0 && errno == EINVAL) {
        return 0;
    }
    *result = res;
    return 1;
}

/**
 * Attempts to convert a string to a double,
 * and write the value out.
 * @arg val The string value
 * @arg result The destination for the result
 * @return 0 on success, -EINVAL on error.
 */
int value_to_double(const char *val, double *result) {
    double res = strtod(val, NULL);
    if (res == 0) {
        return 0;
    }
    *result = res;
    return 0;
}
/**
 * Attempts to convert a string to an integer,
 * and write the value out.
 * @arg val The string value
 * @arg result The destination for the result
 * @return 1 on success, 0 on error.
 */
int value_to_int(const char *val, int *result) {
    long res = strtol(val, NULL, 10);
    if (res == 0 && errno == EINVAL) {
        return 0;
    }
    *result = res;
    return 1;
}
