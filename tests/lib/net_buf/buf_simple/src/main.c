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

/*
 * Integrity validation and CONFIG_NET_BUF_HARDENING fail-closed behaviour.
 *
 * net_buf_simple_is_valid() is always compiled and checks the invariant that
 * data stays within [__buf, __buf+size] and len does not extend past the
 * storage. The hardening tests additionally verify that, when
 * CONFIG_NET_BUF_HARDENING is enabled, the mutating primitives refuse a
 * corrupted or overflowing buffer (returning NULL) instead of reading or
 * writing out of bounds; they are skipped when the option is disabled.
 */

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_is_valid_fresh)
{
	zassert_true(net_buf_simple_is_valid(&buf), "Freshly reset buffer must be valid");

	net_buf_simple_add(&buf, 4);
	zassert_true(net_buf_simple_is_valid(&buf), "Buffer with in-bounds data must be valid");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_is_valid_len_boundary)
{
	buf.len = net_buf_simple_max_len(&buf);
	zassert_true(net_buf_simple_is_valid(&buf), "len == max_len must be valid");

	buf.len = net_buf_simple_max_len(&buf) + 1U;
	zassert_false(net_buf_simple_is_valid(&buf), "len past capacity must be invalid");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_is_valid_corrupt_len)
{
	/* Simulate the corruption: len overwritten with a wrapped/huge value. */
	buf.len = 0xFFFF;
	zassert_false(net_buf_simple_is_valid(&buf),
		      "Corrupted (wrapped) len must be detected as invalid");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_is_valid_bad_data_ptr)
{
	uint8_t *saved = buf.data;

	/* data moved before the storage start */
	buf.data = buf.__buf - 1;
	zassert_false(net_buf_simple_is_valid(&buf), "data before __buf must be invalid");
	buf.data = saved;

	/* headroom larger than the whole buffer */
	buf.data = buf.__buf + buf.size + 1U;
	zassert_false(net_buf_simple_is_valid(&buf), "data past end of storage must be invalid");
	buf.data = saved;
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_hardening_add_corrupt_len)
{
	void *ptr;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_BUF_HARDENING);

	net_buf_simple_add(&buf, 4);

	/* Corrupt len as a heap overflow / attacker might. */
	buf.len = 0xFFFF;

	/* add() must refuse rather than advancing len and returning a tail
	 * pointer past the storage.
	 */
	ptr = net_buf_simple_add(&buf, 1);
	zassert_is_null(ptr, "add() on corrupted buffer must return NULL");
	zassert_equal(buf.len, 0xFFFF, "add() must not mutate a rejected buffer");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_hardening_add_mem_corrupt_len)
{
	uint8_t payload[2] = {0xAA, 0xBB};
	void *ptr;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_BUF_HARDENING);

	buf.len = 0xFFFF;

	ptr = net_buf_simple_add_mem(&buf, payload, sizeof(payload));
	zassert_is_null(ptr, "add_mem() on corrupted buffer must return NULL (no memcpy)");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_hardening_add_overflow)
{
	void *ptr;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_BUF_HARDENING);

	/* Valid buffer, but request more than the tailroom. */
	ptr = net_buf_simple_add(&buf, net_buf_simple_max_len(&buf) + 1U);
	zassert_is_null(ptr, "add() beyond tailroom must return NULL");
	zassert_equal(buf.len, 0, "rejected add() must not change len");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_hardening_pull_underflow)
{
	void *ptr;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_BUF_HARDENING);

	net_buf_simple_add(&buf, 2);

	/* Pulling more than present would underflow len. */
	ptr = net_buf_simple_pull(&buf, 5);
	zassert_is_null(ptr, "pull() beyond len must return NULL");
	zassert_equal(buf.len, 2, "rejected pull() must not change len");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_hardening_pull_corrupt_len)
{
	void *ptr;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_BUF_HARDENING);

	buf.len = 0xFFFF;

	ptr = net_buf_simple_pull(&buf, 1);
	zassert_is_null(ptr, "pull() on corrupted buffer must return NULL");
}

/*
 * Typed scalar accessors dereference the pointer returned by the raw
 * add/push/pull/remove primitives. Under hardening a corrupted buffer must
 * make them fail closed (add_* returns NULL / skips the store, pull_* and
 * remove_* return 0) instead of writing to or reading from a NULL pointer.
 */

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_hardening_add_u8_corrupt_len)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_BUF_HARDENING);

	buf.len = 0xFFFF;

	zassert_is_null(net_buf_simple_add_u8(&buf, 0x42),
			"add_u8() on corrupted buffer must return NULL");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_hardening_add_scalar_corrupt_len)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_BUF_HARDENING);

	buf.len = 0xFFFF;

	/* Void helper: must skip the store rather than dereferencing NULL. The
	 * buffer must be left untouched (len not advanced).
	 */
	net_buf_simple_add_le32(&buf, 0xdeadbeef);
	zassert_equal(buf.len, 0xFFFF, "add_le32() must not mutate a rejected buffer");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_hardening_pull_scalar_corrupt_len)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_BUF_HARDENING);

	buf.len = 0xFFFF;

	zassert_equal(net_buf_simple_pull_u8(&buf), 0,
		      "pull_u8() on corrupted buffer must return 0");
	zassert_equal(net_buf_simple_pull_le16(&buf), 0,
		      "pull_le16() on corrupted buffer must return 0");
	zassert_equal(net_buf_simple_pull_be32(&buf), 0,
		      "pull_be32() on corrupted buffer must return 0");
}

ZTEST(net_buf_simple_test_suite, test_net_buf_simple_hardening_remove_scalar_corrupt_len)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_BUF_HARDENING);

	buf.len = 0xFFFF;

	zassert_equal(net_buf_simple_remove_le16(&buf), 0,
		      "remove_le16() on corrupted buffer must return 0");
}
