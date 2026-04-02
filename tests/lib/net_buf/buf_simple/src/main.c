/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

DEFINE_FFF_GLOBALS;

NET_BUF_SIMPLE_DEFINE_STATIC(buf, 16);
static const uint8_t le16[2] = { 0x02, 0x01 };
static const uint8_t be16[2] = { 0x01, 0x02 };
static const uint8_t le24[3] = { 0x03, 0x02, 0x01 };
static const uint8_t be24[3] = { 0x01, 0x02, 0x03 };
static const uint8_t le32[4] = { 0x04, 0x03, 0x02, 0x01 };
static const uint8_t be32[4] = { 0x01, 0x02, 0x03, 0x04 };
static const uint8_t le40[5] = { 0x05, 0x04, 0x03, 0x02, 0x01 };
static const uint8_t be40[5] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
static const uint8_t le48[6] = { 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
static const uint8_t be48[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
static const uint8_t le64[8] = { 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
static const uint8_t be64[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
static const uint16_t u16 = 0x0102;
static const uint32_t u24 = 0x010203;
static const uint32_t u32 = 0x01020304;
static const uint64_t u40 = 0x0102030405;
static const uint64_t u48 = 0x010203040506;
static const uint64_t u64 = 0x0102030405060708;

static void net_buf_simple_test_suite_before(void *f)
{
	net_buf_simple_reset(&buf);
}

ZTEST_SUITE(net_buf_simple_test_suite, NULL, NULL,
	    net_buf_simple_test_suite_before, NULL, NULL);

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_clone)
{
	struct net_buf_simple clone;

	net_buf_simple_clone(&buf, &clone);

	zassert_equal(buf.data, clone.data, "Incorrect clone data pointer");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_le16)
{
	net_buf_simple_add_mem(&buf, &le16, sizeof(le16));

	zassert_equal(u16, net_buf_simple_pull_le16(&buf),
		      "Invalid 16 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_be16)
{
	net_buf_simple_add_mem(&buf, &be16, sizeof(be16));

	zassert_equal(u16, net_buf_simple_pull_be16(&buf),
		      "Invalid 16 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_le16)
{
	net_buf_simple_add_le16(&buf, u16);

	zassert_mem_equal(le16, net_buf_simple_pull_mem(&buf, sizeof(le16)),
			  sizeof(le16), "Invalid 16 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_be16)
{
	net_buf_simple_add_be16(&buf, u16);

	zassert_mem_equal(be16, net_buf_simple_pull_mem(&buf, sizeof(be16)),
			  sizeof(be16), "Invalid 16 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_le24)
{
	net_buf_simple_add_mem(&buf, &le24, sizeof(le24));

	zassert_equal(u24, net_buf_simple_pull_le24(&buf),
		      "Invalid 24 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_be24)
{
	net_buf_simple_add_mem(&buf, &be24, sizeof(be24));

	zassert_equal(u24, net_buf_simple_pull_be24(&buf),
		      "Invalid 24 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_le24)
{
	net_buf_simple_add_le24(&buf, u24);

	zassert_mem_equal(le24, net_buf_simple_pull_mem(&buf, sizeof(le24)),
			  sizeof(le24), "Invalid 24 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_be24)
{
	net_buf_simple_add_be24(&buf, u24);

	zassert_mem_equal(be24, net_buf_simple_pull_mem(&buf, sizeof(be24)),
			  sizeof(be24), "Invalid 24 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_le32)
{
	net_buf_simple_add_mem(&buf, &le32, sizeof(le32));

	zassert_equal(u32, net_buf_simple_pull_le32(&buf),
		      "Invalid 32 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_be32)
{
	net_buf_simple_add_mem(&buf, &be32, sizeof(be32));

	zassert_equal(u32, net_buf_simple_pull_be32(&buf),
		      "Invalid 32 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_le32)
{
	net_buf_simple_add_le32(&buf, u32);

	zassert_mem_equal(le32, net_buf_simple_pull_mem(&buf, sizeof(le32)),
			  sizeof(le32), "Invalid 32 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_be32)
{
	net_buf_simple_add_be32(&buf, u32);

	zassert_mem_equal(be32, net_buf_simple_pull_mem(&buf, sizeof(be32)),
			  sizeof(be32), "Invalid 32 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_le40)
{
	net_buf_simple_add_mem(&buf, &le40, sizeof(le40));

	zassert_equal(u40, net_buf_simple_pull_le40(&buf),
		      "Invalid 40 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_be40)
{
	net_buf_simple_add_mem(&buf, &be40, sizeof(be40));

	zassert_equal(u40, net_buf_simple_pull_be40(&buf),
		      "Invalid 40 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_le40)
{
	net_buf_simple_add_le40(&buf, u40);

	zassert_mem_equal(le40, net_buf_simple_pull_mem(&buf, sizeof(le40)),
			  sizeof(le40), "Invalid 40 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_be40)
{
	net_buf_simple_add_be40(&buf, u40);

	zassert_mem_equal(be40, net_buf_simple_pull_mem(&buf, sizeof(be40)),
			  sizeof(be40), "Invalid 40 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_le48)
{
	net_buf_simple_add_mem(&buf, &le48, sizeof(le48));

	zassert_equal(u48, net_buf_simple_pull_le48(&buf),
		      "Invalid 48 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_be48)
{
	net_buf_simple_add_mem(&buf, &be48, sizeof(be48));

	zassert_equal(u48, net_buf_simple_pull_be48(&buf),
		      "Invalid 48 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_le48)
{
	net_buf_simple_add_le48(&buf, u48);

	zassert_mem_equal(le48, net_buf_simple_pull_mem(&buf, sizeof(le48)),
			  sizeof(le48), "Invalid 48 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_be48)
{
	net_buf_simple_add_be48(&buf, u48);

	zassert_mem_equal(be48, net_buf_simple_pull_mem(&buf, sizeof(be48)),
			  sizeof(be48), "Invalid 48 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_le64)
{
	net_buf_simple_add_mem(&buf, &le64, sizeof(le64));

	zassert_equal(u64, net_buf_simple_pull_le64(&buf),
		      "Invalid 64 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_pull_be64)
{
	net_buf_simple_add_mem(&buf, &be64, sizeof(be64));

	zassert_equal(u64, net_buf_simple_pull_be64(&buf),
		      "Invalid 64 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_le64)
{
	net_buf_simple_add_le64(&buf, u64);

	zassert_mem_equal(le64, net_buf_simple_pull_mem(&buf, sizeof(le64)),
			  sizeof(le64), "Invalid 64 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_add_be64)
{
	net_buf_simple_add_be64(&buf, u64);

	zassert_mem_equal(be64, net_buf_simple_pull_mem(&buf, sizeof(be64)),
			  sizeof(be64), "Invalid 64 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_le16)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &le16, sizeof(le16));

	zassert_equal(u16, net_buf_simple_remove_le16(&buf),
		      "Invalid 16 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_be16)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &be16, sizeof(be16));

	zassert_equal(u16, net_buf_simple_remove_be16(&buf),
		      "Invalid 16 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_le16)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_le16(&buf, u16);

	zassert_mem_equal(le16, net_buf_simple_remove_mem(&buf, sizeof(le16)),
			  sizeof(le16),  "Invalid 16 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_be16)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_be16(&buf, u16);

	zassert_mem_equal(be16, net_buf_simple_remove_mem(&buf, sizeof(be16)),
			  sizeof(be16),  "Invalid 16 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_le24)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &le24, sizeof(le24));

	zassert_equal(u24, net_buf_simple_remove_le24(&buf),
		      "Invalid 24 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_be24)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &be24, sizeof(be24));

	zassert_equal(u24, net_buf_simple_remove_be24(&buf),
		      "Invalid 24 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_le24)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_le24(&buf, u24);

	zassert_mem_equal(le24, net_buf_simple_remove_mem(&buf, sizeof(le24)),
			  sizeof(le24),  "Invalid 24 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_be24)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_be24(&buf, u24);

	zassert_mem_equal(be24, net_buf_simple_remove_mem(&buf, sizeof(be24)),
			  sizeof(be24),  "Invalid 24 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_le32)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &le32, sizeof(le32));

	zassert_equal(u32, net_buf_simple_remove_le32(&buf),
		      "Invalid 32 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_be32)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &be32, sizeof(be32));

	zassert_equal(u32, net_buf_simple_remove_be32(&buf),
		      "Invalid 32 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_le32)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_le32(&buf, u32);

	zassert_mem_equal(le32, net_buf_simple_remove_mem(&buf, sizeof(le32)),
			  sizeof(le32), "Invalid 32 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_be32)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_be32(&buf, u32);

	zassert_mem_equal(be32, net_buf_simple_remove_mem(&buf, sizeof(be32)),
			  sizeof(be32), "Invalid 32 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_le40)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &le40, sizeof(le40));

	zassert_equal(u40, net_buf_simple_remove_le40(&buf), "Invalid 40 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_be40)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &be40, sizeof(be40));

	zassert_equal(u40, net_buf_simple_remove_be40(&buf), "Invalid 40 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_le40)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_le40(&buf, u40);

	zassert_mem_equal(le40, net_buf_simple_remove_mem(&buf, sizeof(le40)), sizeof(le40),
			  "Invalid 40 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_be40)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_be40(&buf, u40);

	zassert_mem_equal(be40, net_buf_simple_remove_mem(&buf, sizeof(be40)), sizeof(be40),
			  "Invalid 40 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_le48)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &le48, sizeof(le48));

	zassert_equal(u48, net_buf_simple_remove_le48(&buf),
		      "Invalid 48 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_be48)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &be48, sizeof(be48));

	zassert_equal(u48, net_buf_simple_remove_be48(&buf),
		      "Invalid 48 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_le48)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_le48(&buf, u48);

	zassert_mem_equal(le48, net_buf_simple_remove_mem(&buf, sizeof(le48)),
			  sizeof(le48),  "Invalid 48 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_be48)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_be48(&buf, u48);

	zassert_mem_equal(be48, net_buf_simple_remove_mem(&buf, sizeof(be48)),
			  sizeof(be48),  "Invalid 48 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_le64)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &le64, sizeof(le64));

	zassert_equal(u64, net_buf_simple_remove_le64(&buf),
		      "Invalid 64 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_remove_be64)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_mem(&buf, &be64, sizeof(be64));

	zassert_equal(u64, net_buf_simple_remove_be64(&buf),
		      "Invalid 64 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_le64)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_le64(&buf, u64);

	zassert_mem_equal(le64, net_buf_simple_remove_mem(&buf, sizeof(le64)),
			  sizeof(le64), "Invalid 64 bits byte order");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_push_be64)
{
	net_buf_simple_reserve(&buf, 16);

	net_buf_simple_push_be64(&buf, u64);

	zassert_mem_equal(be64, net_buf_simple_remove_mem(&buf, sizeof(be64)),
			  sizeof(be64), "Invalid 64 bits byte order");
}
