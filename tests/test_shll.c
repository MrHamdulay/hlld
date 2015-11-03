#include <check.h>
#include "shll.h"

START_TEST(test_shll_init_and_destroy)
{
    hll_t h;
    fail_unless(shll_init(10, 10, 1, &h) == 0);
    fail_unless(shll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_add_register)
{
    hll_t h;
    fail_unless(shll_init(10, 100, 1, &h) == 0);
    shll_point p = {100, 3};
    shll_register_add_point(&h.sliding, &h.sliding.registers[0], p);
    fail_unless(h.sliding.registers[0].size == 1);

    fail_unless(shll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_remove_smaller)
{
    hll_t h;
    fail_unless(shll_init(10, 100, 1, &h) == 0);
    int num_points = 10;
    int points_leading_value[] = {8, 9, 6, 6, 7, 4, 5, 2, 9, 1, 5};
    int expected_size[] = {1, 1, 2, 2, 2, 3, 3, 4, 1, 2, 2};

    shll_register *r = &h.sliding.registers[0];
    for(int i=0; i<num_points; i++) {
        shll_point p = {100, points_leading_value[i]};
        shll_register_add_point(&h.sliding, r, p);
        fail_unless(r->size == expected_size[i]);
    }

    fail_unless(shll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_remove_time)
{
    hll_t h;
    fail_unless(shll_init(10, 100, 1, &h) == 0);
    int num_points = 6;
    int points_time[] = {100, 200, 299, 300, 301, 302};
    int expected_size[] = {1, 1, 2, 2, 3, 4};

    shll_register *r = &h.sliding.registers[0];
    for(int i=0; i<num_points; i++) {
        shll_point p = {points_time[i], num_points-i};
        shll_register_add_point(&h.sliding, r, p);
        fail_unless(r->size == expected_size[i]);
    }

    fail_unless(shll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_add_hash)
{
    hll_t h;
    fail_unless(shll_init(10, 100, 1, &h) == 0);

    for (uint64_t i=0; i < 100; i++) {
        shll_add_hash(&h, i ^ rand());
    }

    fail_unless(shll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_shrink_register)
{
    hll_t h;
    fail_unless(shll_init(10, 100, 1, &h) == 0);

    shll_register *r = &h.sliding.registers[0];
    // add 100 points
    for(int i=0; i<100; i++) {
        shll_point p = {0, 100-i};
        shll_register_add_point(&h.sliding, r, p);
        fail_unless(r->size == i+1);
    }
    // remove all points
    shll_point p = {200, 1};
    shll_register_add_point(&h.sliding, r, p);
    fail_unless(r->size == 1);
    fail_unless(r->size*1.5*1.5+1 >= r->capacity);

    // add all back and check bounds on capacity
    for(int i=0; i<100; i++) {
        shll_point p = {200, 100-i};
        shll_register_add_point(&h.sliding, r, p);
        fail_unless(r->size == i+1);
        // check that capacity is bounded
        fail_unless(r->size*1.5*1.5+1 >= r->capacity);
    }

    fail_unless(shll_destroy(&h) == 0);
}
END_TEST
