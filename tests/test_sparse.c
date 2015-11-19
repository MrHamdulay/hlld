#include <check.h>

#include "hll.h"
#include "serialize.h"

START_TEST(test_sparse_insert) {
    hll_t h;
    char buf[200];
    fail_unless(hll_init(12, 100, 1, &h) == 0);
    for (int i=0; i < 100; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add_at_time(&h, (char*)&buf, 100);
    }
    double size = hll_size_total(&h);
    fail_unless(100-1e-4 < size && 100+1e-4 > size);
    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_sparse_convert) {
    hll_t h;
    char buf[200];
    fail_unless(hll_init(12, 100, 1, &h) == 0);
    for (int i=0; i < 100; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add_at_time(&h, (char*)&buf, 100);
    }
    double size = hll_size_total(&h);
    fail_unless(100-1e-4 < size && 100+1e-4 > size);
    fail_unless(h.representation == HLL_SPARSE);

    hll_convert_dense(&h);
    fail_unless(h.representation == HLL_DENSE);
    size = hll_size_total(&h);
    fail_unless(97 < size && 102 > size);

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST
