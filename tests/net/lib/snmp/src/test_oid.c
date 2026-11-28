/*
 * Copyright (c) 2026 StructCore Embedded
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * OID Handling Unit Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/net/snmp/snmp_ber.h>

#define ASSERT(c)          zassert_true(c, #c)
#define ASSERT_EQ(a, b)    zassert_equal(a, b, NULL)
#define ASSERT_NE(a, b)    zassert_not_equal(a, b, NULL)
#define ASSERT_GT(a, b)    zassert_true((a) > (b), NULL)
#define ASSERT_LT(a, b)    zassert_true((a) < (b), NULL)
#define ASSERT_STREQ(a, b) zassert_mem_equal(a, b, strlen(b), NULL)

ZTEST(snmp_oid, test_oid_equal)
{
	struct snmp_oid a = {.len = 5, .ids = {1, 3, 6, 1, 2}};
	struct snmp_oid b = {.len = 5, .ids = {1, 3, 6, 1, 2}};
	struct snmp_oid c = {.len = 5, .ids = {1, 3, 6, 1, 3}};
	struct snmp_oid d = {.len = 4, .ids = {1, 3, 6, 1}};

	ASSERT(snmp_oid_equal(&a, &b) == true);
	ASSERT(snmp_oid_equal(&a, &c) == false);
	ASSERT(snmp_oid_equal(&a, &d) == false);
	ASSERT(snmp_oid_equal(NULL, &a) == false);
	ASSERT(snmp_oid_equal(&a, NULL) == false);
}

ZTEST(snmp_oid, test_oid_compare_lexicographic)
{
	struct snmp_oid a = {.len = 5, .ids = {1, 3, 6, 1, 2}};
	struct snmp_oid b = {.len = 5, .ids = {1, 3, 6, 1, 3}};
	struct snmp_oid c = {.len = 6, .ids = {1, 3, 6, 1, 2, 1}};
	struct snmp_oid d = {.len = 4, .ids = {1, 3, 6, 1}};

	ASSERT(snmp_oid_compare(&a, &b) < 0);
	ASSERT(snmp_oid_compare(&b, &a) > 0);
	ASSERT(snmp_oid_compare(&a, &a) == 0);
	ASSERT(snmp_oid_compare(&d, &a) < 0);
	ASSERT(snmp_oid_compare(&c, &a) > 0);
}

ZTEST(snmp_oid, test_oid_encode_decode_roundtrip)
{
	uint8_t buf[256];
	size_t off = 0;

	struct snmp_oid original = {.len = 14,
				    .ids = {1, 3, 6, 1, 4, 1, 54321, 1, 2, 3, 4, 5, 6, 0}};

	int ret = snmp_ber_encode_oid(buf, 256, &off, &original);
	ASSERT_EQ(ret, 0);

	size_t dec_off = 0;
	struct snmp_oid decoded = {0};
	ret = snmp_ber_decode_oid(buf, off, &dec_off, &decoded);

	ASSERT_EQ(ret, 0);
	ASSERT_EQ(decoded.len, original.len);
	for (int i = 0; i < original.len; i++) {
		ASSERT_EQ(decoded.ids[i], original.ids[i]);
	}
}

ZTEST(snmp_oid, test_oid_max_depth)
{
	uint8_t buf[256];
	size_t off = 0;

	struct snmp_oid max_oid = {.len = 16,
				   .ids = {1, 3, 6, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

	int ret = snmp_ber_encode_oid(buf, 256, &off, &max_oid);
	ASSERT_EQ(ret, 0);

	size_t dec_off = 0;
	struct snmp_oid decoded = {0};
	ret = snmp_ber_decode_oid(buf, off, &dec_off, &decoded);

	ASSERT_EQ(ret, 0);
	ASSERT_EQ(decoded.len, 16);
}
