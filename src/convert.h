#ifndef CONVERT_H
#define CONVERT_H
/**
 * Attempts to convert a string to an integer (64bit),
 * and write the value out.
 * @arg val The string value
 * @arg result The destination for the result
 * @return 0 on success, 0 on error.
 */
int value_to_int64(const char *val, uint64_t *result);

/**
 * Attempts to convert a string to an integer,
 * and write the value out.
 * @arg val The string value
 * @arg result The destination for the result
 * @return 1 on success, 0 on error.
 */
int value_to_int(const char *val, int *result);

/**
 * Attempts to convert a string to a double,
 * and write the value out.
 * @arg val The string value
 * @arg result The destination for the result
 * @return 0 on success, -EINVAL on error.
 */
int value_to_double(const char *val, double *result);

#endif
