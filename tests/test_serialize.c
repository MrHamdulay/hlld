#include <check.h>
#include "hll.h"
#include "serialize.h"

START_TEST(test_hll_serialize)
{
    unsigned char buf[2048];
    serialize_t s = { buf, 0, 2048 };
    
    hll_t h, h_unserialize;
    fail_unless(hll_init(HLL_MIN_PRECISION, 100+1<<12, 12, &h) == 0);
    serialize_hll(&s, &h);
    fail_unless(hll_destroy(&h) == 0);

    s.offset = 0;
    fail_unless(unserialize_hll(&s, &h_unserialize) == 0);

    fail_unless(h_unserialize.representation == HLL_SPARSE);
    fail_unless(h_unserialize.precision == HLL_MIN_PRECISION);
    fail_unless(h_unserialize.window_period == 100+1<<12);
    fail_unless(h_unserialize.window_precision == 12);
    fail_unless(hll_destroy(&h_unserialize) == 0);
}
END_TEST

START_TEST(test_serialize_register)
{
    unsigned char buf[2048];
    serialize_t s = { buf, 0, 2048 };
    
    hll_t h;
    hll_register r = {0, 0, NULL}, r_unserialize;
    hll_dense_point p = {13, 19};
    hll_register_add_point(&h, &r, p);

    serialize_hll_register(&s, &r);

    s.offset = 0;
    fail_unless(unserialize_hll_register(&s, &r_unserialize) == 0);
    fail_unless(r_unserialize.size == 1);
    fail_unless(r_unserialize.points[0].timestamp == 13);
    fail_unless(r_unserialize.points[0].register_ == 19);

}
END_TEST

START_TEST(test_hll_serialize_registers)
{
    unsigned char buf[2048];
    serialize_t s = { buf, 0, 2048 };
    
    hll_t h, h_unserialize;
    fail_unless(hll_init(HLL_MIN_PRECISION, 100, 1, &h) == 0);
    hll_convert_dense(&h);

    hll_dense_point p = {2, 2};
    hll_register_add_point(&h, &h.dense_registers[0], p);
    p.register_ = 1;
    p.timestamp = 1;
    hll_register_add_point(&h, &h.dense_registers[0], p);
    p.register_ = 2;
    hll_register_add_point(&h, &h.dense_registers[1], p);
    serialize_hll(&s, &h);
    fail_unless(hll_destroy(&h) == 0);

    s.offset = 0;
    fail_unless(unserialize_hll(&s, &h_unserialize) == 0);

    fail_unless(h_unserialize.dense_registers[0].size == 2);
    fail_unless(h_unserialize.dense_registers[1].size == 1);
    fail_unless(h_unserialize.dense_registers[0].points[0].register_ == 2);
    fail_unless(h_unserialize.dense_registers[0].points[0].timestamp == 2);
    fail_unless(h_unserialize.dense_registers[0].points[1].register_ == 1);
    fail_unless(h_unserialize.dense_registers[0].points[1].timestamp == 1);
    fail_unless(h_unserialize.dense_registers[1].points[0].register_ == 2);
    fail_unless(h_unserialize.dense_registers[1].points[0].timestamp == 1);

    fail_unless(h_unserialize.precision == HLL_MIN_PRECISION);
    fail_unless(h_unserialize.window_period == 100);
    fail_unless(h_unserialize.window_precision == 1);
    fail_unless(hll_destroy(&h_unserialize) == 0);
}
END_TEST

START_TEST(test_serialize_primitives)
{
    unsigned char buf[2048];
    serialize_t s = { buf, 0, 2048 };

    int t1 = 2;
    int e1 = 0;
    long t2 = 5;
    long e2 = 0;
    unsigned char t3 = 7;
    unsigned char e3 = 0;
    time_t t4 = time(NULL);
    time_t e4 = 0;
    uint64_t t5 = 1002+(1ll<<35);
    uint64_t e5 = 0;

    fail_unless(serialize_int(&s, t1) == 0);
    fail_unless(serialize_long(&s, t2) == 0);
    fail_unless(serialize_unsigned_char(&s, t3) == 0);
    fail_unless(serialize_time(&s, t4) == 0);
    fail_unless(serialize_ulong_long(&s, t5) == 0);

    s.offset = 0;
    fail_unless(unserialize_int(&s, &e1) == 0);
    fail_unless(unserialize_long(&s, &e2) == 0);
    fail_unless(unserialize_unsigned_char(&s, &e3) == 0);
    fail_unless(unserialize_time(&s, &e4) == 0);
    fail_unless(unserialize_ulong_long(&s, &e5) == 0);

    fail_unless(t1 == e1);
    fail_unless(t2 == e2);
    fail_unless(t3 == e3);
    fail_unless(t4 == e4);
    fail_unless(t5 == e5);
}
END_TEST

START_TEST(test_hll_serialize_sparse) {
    hll_t h, h_unserialize;
    char buf[200];
    fail_unless(hll_init(12, 100, 1, &h) == 0);
    for (int i=0; i < 100; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add_at_time(&h, (char*)&buf, 100);
    }
    for (int i=0; i < 100; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add_at_time(&h, (char*)&buf, 100);
    }

    unsigned char buf2[4096];
    serialize_t s = { buf2, 0, 4096 };
    fail_unless(serialize_hll(&s, &h) == 0);
    fail_unless(hll_destroy(&h) == 0);

    s.offset = 0;
    fail_unless(unserialize_hll(&s, &h_unserialize) == 0);

    double size = hll_size_total(&h_unserialize);
    fail_unless(97 < size && 102 > size);

    fail_unless(hll_destroy(&h_unserialize) == 0);

}
END_TEST
