#include <check.h>
#include "shll.h"

START_TEST(test_shll_init_and_destroy)
{
    hll_t h;
    fail_unless(shll_init(10, 10, 1, &h) == 0);
    fail_unless(shll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_shll_add_hash)
{
    hll_t h;
    fail_unless(shll_init(10, 100, 1, &h) == 0);

    for (uint64_t i=0; i < 100; i++) {
        hll_add_hash(&h, i ^ rand());
    }

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST
