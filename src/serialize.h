#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "stdint.h"
#include "hll.h"

typedef struct {
    unsigned char *memory;
    size_t offset;
    size_t size;
} serialize_t;

int serialize_hll(serialize_t *s, hll_t *h);
int unserialize_hll(serialize_t *s, hll_t *h);
int serialize_hll_register(serialize_t *s, hll_register *h);
int unserialize_hll_register(serialize_t *s, hll_register *h);

int serialize_int(serialize_t *s, int i);
int unserialize_int(serialize_t *s, int *i);
int serialize_long(serialize_t *s, long i);
int unserialize_long(serialize_t *s, long *i);
int serialize_unsigned_char(serialize_t *s, unsigned char c);
int unserialize_unsigned_char(serialize_t *s, unsigned char *c);

int unserialize_hll_from_file(int fileno, uint64_t len, hll_t *h);
int unserialize_hll_from_filename(char *filename, hll_t *h);

size_t serialized_hll_size(hll_t *h);
int serialize_hll_to_filename(char *filename, hll_t *h);
#endif
