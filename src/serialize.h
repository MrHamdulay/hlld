#ifndef SERIALIZE_H
#define SERIALIZE_H

typedef struct {
    unsigned char *memory;
    size_t offset;
    size_t size;
} serialize_t;

int serialize_hll(serialize_t *s, hll_t *h);
int unserialize_hll(serialize_t *s, hll_t *h);
int serialize_hll_register(serialize_t *s, hll_register *h);
int unserialize_hll_register(serialize_t *s, hll_register *h);
#endif
