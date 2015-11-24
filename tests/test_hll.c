#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "hll.h"

START_TEST(test_hll_init_bad)
{
    hll_t h;
    fail_unless(hll_init(HLL_MIN_PRECISION-1, 100, 1, &h) == -1);
    fail_unless(hll_init(HLL_MAX_PRECISION+1,100, 1, &h) == -1);

    fail_unless(hll_init(HLL_MIN_PRECISION, 100, 1, &h) == 0);
    fail_unless(hll_destroy(&h) == 0);

    fail_unless(hll_init(HLL_MAX_PRECISION, 100, 1, &h) == 0);
    fail_unless(hll_destroy(&h) == 0);
}
END_TEST


START_TEST(test_hll_init_and_destroy)
{
    hll_t h;
    fail_unless(hll_init(10, 100, 1, &h) == 0);
    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_add)
{
    hll_t h;
    fail_unless(hll_init(10, 100, 1, &h) == 0);

    char buf[100];
    for (int i=0; i < 100; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add_at_time(&h, (char*)&buf, 100);
    }

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_add_hash)
{
    hll_t h;
    fail_unless(hll_init(10, 100, 1, &h) == 0);

    for (uint64_t i=0; i < 100; i++) {
        hll_add_hash_at_time(&h, i ^ rand(), 100);
    }

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_add_size)
{
    hll_t h;
    fail_unless(hll_init(10, 100, 1, &h) == 0);

    char buf[100];
    for (int i=0; i < 100; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add_at_time(&h, (char*)&buf, 100);
    }

    double s = hll_size_total(&h);
    fail_unless(s > 95 && s < 105);

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_size_total)
{
    hll_t h;
    fail_unless(hll_init(10, 100, 1, &h) == 0);

    double s = hll_size_total(&h);
    fail_unless(s == 0);

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_error_bound)
{
    // Precision 14 -> variance of 1%
    hll_t h;
    fail_unless(hll_init(14, 100, 1, &h) == 0);

    char buf[100];
    for (int i=0; i < 10000; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add_at_time(&h, (char*)&buf, 100);
    }

    // Should be within 1%
    double s = hll_size_total(&h);
    fail_unless(s > 9900 && s < 10100);

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_precision_for_error)
{
    fail_unless(hll_precision_for_error(1.0) == -1);
    fail_unless(hll_precision_for_error(0.0) == -1);
    fail_unless(hll_precision_for_error(0.02) == 12);
    fail_unless(hll_precision_for_error(0.01) == 14);
    fail_unless(hll_precision_for_error(0.005) == 16);
}
END_TEST


START_TEST(test_hll_error_for_precision)
{
    fail_unless(hll_error_for_precision(3) == 0);
    fail_unless(hll_error_for_precision(20) == 0);
    fail_unless(hll_error_for_precision(12) == .01625);
    fail_unless(hll_error_for_precision(10) == .0325);
    fail_unless(hll_error_for_precision(16) == .0040625);
}
END_TEST

START_TEST(test_hll_bytes_for_precision)
{
    fail_unless(hll_bytes_for_precision(3) == 0);
    fail_unless(hll_bytes_for_precision(20) == 0);
    fail_unless(hll_bytes_for_precision(12) == 3280);
    fail_unless(hll_bytes_for_precision(10) == 820);
    fail_unless(hll_bytes_for_precision(16) == 52432);
}
END_TEST

START_TEST(test_hll_union)
{
    hll_t *hlls[10];
    for(int i=0; i<10; i++) {
        hlls[i] = (hll_t*)malloc(sizeof(hll_t));
        fail_unless(hll_init(12, 100, 1, hlls[i]) == 0);
    }
    char buf[100];
    for (int i=0; i < 10000; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add_at_time(hlls[i%10], (char*)&buf, 100);
    }


    double s = hll_union_size(hlls, 10, 100, 100);
    fail_unless(s > 9900 && s < 10100);
    for(int i=0; i<10; i++) {
        // check that it doesn't change the sizes of the original sets
        double s = hll_size_total(hlls[i]);
        fail_unless(s > 980 && s < 1100);
        fail_unless(hll_destroy(hlls[i]) == 0);
        free(hlls[i]);
    }
}
END_TEST
