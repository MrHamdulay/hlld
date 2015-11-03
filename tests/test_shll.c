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
    /*
     * 8
     * 9
     * 9 6
     * 9 6
     * 9 7
     * 9 7 4
     * 9 7 5
     * 9 7 5 2
     * 9
     * 9 1
     * 9 5*/
    int num_points = 10;
    int points[] = {8, 9, 6, 6, 7, 4, 5, 2, 9, 1, 5};
    int expected_size[] = {1, 1, 2, 2, 2, 3, 3, 4, 1, 2, 2};
    shll_register *r = &h.sliding.registers[0];
    for(int i=0; i<num_points; i++) {
        shll_point p = {100, points[i]};
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

    /*for (uint64_t i=0; i < 100; i++) {
        shll_add_hash(&h, i ^ rand());
    }*/

    fail_unless(shll_destroy(&h) == 0);
}
END_TEST
