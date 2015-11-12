#include <assert.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "serialize.h"
#include <signal.h>
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
    h->points = (hll_point*)malloc(h->capacity*sizeof(hll_point));
    for (long i=0; i<h->size; i++) {
        ERR(unserialize_long(s, &h->points[i].timestamp));
        ERR(unserialize_long(s, &h->points[i].register_));
    }
    return 0;
}

int serialize_hll(serialize_t *s, hll_t *h) {
    ERR(serialize_int(s, SERIAL_VERSION));
    ERR(serialize_int(s, (int)h->precision));
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
    if (version != SERIAL_VERSION) {
        return -1;
    }
    int precision;
    ERR(unserialize_int(s, &precision));
    h->precision = (unsigned char)precision;
    ERR(unserialize_int(s, &h->window_period));
    ERR(unserialize_int(s, &h->window_precision));
    int num_regs = NUM_REG(h->precision);
    h->registers = (hll_register*)malloc(num_regs*sizeof(hll_register));
    for(int i=0; i<num_regs; i++) {
        ERR(unserialize_hll_register(s, &h->registers[i]));
    }
    return 0;
}
int unserialize_hll_from_filename(char *filename, hll_t *h) {
    int fileno = open(filename, O_RDWR, 0644);
    if (fileno == -1) {
        printf("open failed to unserialize %s: %d\n", filename, errno);
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
    printf("serializing from file\n");
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
    unsigned char* addr = (unsigned char*)mmap(NULL, len, PROT_READ|PROT_WRITE,
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

size_t serialized_hll_size(hll_t *h) {
    // VERSION, precision, window_period, window_precision
    size_t size = sizeof(int)*3 + sizeof(long);

    for(int i=0; i<NUM_REG(h->precision); i++) {
        // size, size*(timestamp, register)
        size += sizeof(long) + 2*sizeof(long)*h->registers[i].size; 
    }
    return size;
}

int serialize_hll_to_filename(char *filename, hll_t *h) {
    size_t serialized_size  = serialized_hll_size(h)+20;

    int fileno = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fileno == -1) {
        syslog(LOG_ERR, "open failed on serialization %s", strerror(errno) );
        return -errno;
    }

    unsigned char* addr = (unsigned char*)malloc(sizeof(char)*serialized_size);

    serialize_t s = {addr, 0, serialized_size};
    int res = serialize_hll(&s, h);
    if (res == -1) {
        syslog(LOG_ERR, "unable to serialize hl");
        return -1;
    }
    size_t written = 0;
    while(written < serialized_size) {
        res = pwrite(fileno, addr, serialized_size-written, written);
        if (res == -1 && errno != EINTR) {
            return -errno;
        }
        written += res;
        res = 0;
    }

    close(fileno);
    free(addr);

    return 0;
}
