#include <assert.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "serialize.h"
#include "hll.h"

#define SERIAL_VERSION 1
#define ERR(err) if (err == -1) { return -1; }

inline int serialize_int(serialize_t *s, int i) {
    if (s->offset + sizeof(int) >= s->size)
        return -1;
    *(int*)(s->memory+s->offset) = i;
    s->offset += sizeof(int);
    return 0;
}

inline int unserialize_int(serialize_t *s, int *i) {
    if (s->offset + sizeof(int) >= s->size)
        return -1;
    *i = *(int*)(s->memory+s->offset);
    s->offset += sizeof(int);
    return 0;
}

inline int serialize_long(serialize_t *s, long i) {
    if (s->offset + sizeof(long) >= s->size)
        return -1;
    *(long*)(s->memory+s->offset) = i;
    s->offset += sizeof(long);
    return 0;
}

inline int unserialize_long(serialize_t *s, long *i) {
    if (s->offset + sizeof(long) >= s->size)
        return -1;
    *i = *(long*)(s->memory+s->offset);
    s->offset += sizeof(long);
    return 0;
}

inline int serialize_unsigned_char(serialize_t *s, unsigned char c) {
    if (s->offset + sizeof(char) >= s->size)
        return -1;
    s->memory[s->offset] = c;
    s->offset += sizeof(char);
    return 0;
}

inline int unserialize_unsigned_char(serialize_t *s, unsigned char *c) {
    if (s->offset + sizeof(char) >= s->size)
        return -1;
    *c = s->memory[s->offset];
    s->offset += sizeof(char);
    return 0;
}


int serialize_hll_register(serialize_t *s, hll_register *h) {
    ERR(serialize_long(s, h->size));
    for (long i=0; i<h->size; i++) {
        ERR(serialize_long(s, h->points[i].timestamp));
        ERR(serialize_long(s, h->points[i].register_));
    }
    return 0;
}

int unserialize_hll_register(serialize_t *s, hll_register *h) {
    ERR(unserialize_long(s, &h->size));
    h->capacity = h->size;
    h->points = malloc(h->capacity*sizeof(hll_register));
    for (long i=0; i<h->size; i++) {
        ERR(unserialize_long(s, &h->points[i].timestamp));
        ERR(unserialize_long(s, &h->points[i].register_));
    }
    return 0;
}

int serialize_hll(serialize_t *s, hll_t *h) {
    ERR(serialize_int(s, SERIAL_VERSION));
    ERR(serialize_unsigned_char(s, h->precision));
    ERR(serialize_int(s, h->window_period));
    ERR(serialize_int(s, h->window_precision));
    int num_regs = NUM_REG(h->precision);
    for(int i=0; i<num_regs; i++) {
        ERR(serialize_hll_register(s, &h->registers[i]));
    }
    return 0;
}

int unserialize_hll(serialize_t *s, hll_t *h) {
    int version;
    ERR(unserialize_int(s, &version));
    if (version != SERIAL_VERSION)
        return -1;
    ERR(unserialize_unsigned_char(s, &h->precision));
    ERR(unserialize_int(s, &h->window_period));
    ERR(unserialize_int(s, &h->window_precision));
    int num_regs = NUM_REG(h->precision);
    h->registers = malloc(num_regs*sizeof(hll_register));
    for(int i=0; i<num_regs; i++) {
        ERR(unserialize_hll_register(s, &h->registers[i]));
    }
    return 0;
}
int unserialize_hll_from_filename(char *filename, hll_t *h) {
    int fileno = open(filename, O_RDWR, 0644);
    if (fileno == -1) {
        perror("open failed on bitmap!");
        return -errno;
    }

    struct stat buf;
    int res = fstat(fileno, &buf);
    if (res != 0) {
        perror("fstat failed on bitmap!");
        close(fileno);
        return -errno;
    }

    uint64_t len = buf.st_size;

    res = unserialize_hll_from_file(fileno, len, h);

    close(fileno);
    return res;
}

int unserialize_hll_from_file(int fileno, uint64_t len, hll_t *h) {
    // Hack for old kernels and bad length checking
    if (len == 0) {
        return -EINVAL;
    }

    // Check for and clear NEW_BITMAP from the mode
    // Handle each mode
    int flags;
    int newfileno;
    flags = MAP_SHARED;
    newfileno = dup(fileno);
    if (newfileno < 0) return -errno;

    // Perform the map in
    unsigned char* addr = mmap(NULL, len, PROT_READ|PROT_WRITE,
            flags, newfileno, 0);

    // Check for an error, otherwise return
    if (addr == MAP_FAILED) {
        perror("mmap failed!");
        if (newfileno >= 0) {
            close(newfileno);
        }
        return -errno;
    }

    // Provide some advise on how the memory will be used
    int res;
    res = madvise(addr, len, MADV_WILLNEED);
    if (res != 0) {
        perror("Failed to call madvise() [MADV_WILLNEED]");
    }
    res = madvise(addr, len, MADV_RANDOM);
    if (res != 0) {
        perror("Failed to call madvise() [MADV_RANDOM]");
    }

    serialize_t s = {addr, 0, len};
    res = unserialize_hll(&s, h);
    if (res != 0) {
        perror("Failed to unserialize hll");
    }

    return res;
}
