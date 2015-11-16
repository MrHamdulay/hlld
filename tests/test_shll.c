#include <check.h>
#include "hll.h"

START_TEST(test_shll_init_and_destroy)
{
    hll_t h;
    fail_unless(hll_init(10, 10, 1, &h) == 0);
    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_add_register)
{
    hll_t h;
    fail_unless(hll_init(10, 100, 1, &h) == 0);
    hll_point p = {100, 3};
    fail_unless(h.precision == 10);
    hll_register_add_point(&h, &h.registers[0], p);
    fail_unless(h.registers[0].size == 1);

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_remove_smaller)
{
    hll_t h;
    fail_unless(hll_init(10, 100, 1, &h) == 0);
    int num_points = 10;
    int points_leading_value[] = {8, 9, 6, 6, 7, 4, 5, 2, 9, 1, 5};
    int expected_size[] = {1, 1, 2, 2, 2, 3, 3, 4, 1, 2, 2};

    hll_register *r = &h.registers[0];
    for(int i=0; i<num_points; i++) {
        hll_point p = {100, points_leading_value[i]};
        hll_register_add_point(&h, r, p);
        fail_unless(r->size == expected_size[i]);
    }

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_remove_time)
{
    hll_t h;
    fail_unless(hll_init(10, 100, 1, &h) == 0);
    int num_points = 6;
    int points_time[] = {100, 200, 299, 300, 301, 302};
    int expected_size[] = {1, 1, 2, 2, 3, 4};

    hll_register *r = &h.registers[0];
    for(int i=0; i<num_points; i++) {
        hll_point p = {points_time[i], num_points-i};
        hll_register_add_point(&h, r, p);
        fail_unless(r->size == expected_size[i]);
    }

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_add_hash)
{
    hll_t h;
    fail_unless(hll_init(10, 100, 1, &h) == 0);

    for (uint64_t i=0; i < 100; i++) {
        hll_add_hash_at_time(&h, i ^ rand(), 100);
    }

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_shrink_register)
{
    hll_t h;
    fail_unless(hll_init(10, 100, 1, &h) == 0);

    hll_register *r = &h.registers[0];
    // add 100 points
    for(int i=0; i<100; i++) {
        hll_point p = {0, 100-i};
        hll_register_add_point(&h, r, p);
        fail_unless(r->size == i+1);
    }
    // remove all points
    hll_point p = {200, 1};
    hll_register_add_point(&h, r, p);
    fail_unless(r->size == 1);
    // check that capacity was reduced appropriately
    fail_unless(r->size*1.5*1.5+1 >= r->capacity);

    // add all back and check bounds on capacity
    for(int i=0; i<100; i++) {
        hll_point p = {200, 100-i};
        hll_register_add_point(&h, r, p);
        fail_unless(r->size == i+1);
        // check that capacity is bounded
        fail_unless(r->size*1.5*1.5+1 >= r->capacity);
    }

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST


START_TEST(test_shll_error_bound)
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
    double s = hll_size(&h, 100, time(NULL));
    fail_unless(s > 9900 && s < 10100);

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST


START_TEST(test_shll_time_queries)
{
    // Precision 14 -> variance of 1%
    hll_t h;
    fail_unless(hll_init(14, 100, 1, &h) == 0);

    char buf[100];
    for (int i=0; i < 90000; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add_at_time(&h, (char*)&buf, i/10000);
    }

    // Should be within 1'ish%
    for(int i=0; i<9; i++) {
        double s = hll_size(&h, i+2, 9);
        fail_unless(s > 9900*(i+1)-100 && s < 10100*(i+1)+100);
    }

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST
