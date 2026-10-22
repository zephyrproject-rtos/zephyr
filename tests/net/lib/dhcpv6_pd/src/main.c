/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/net_ip.h>

/* White-box test: pull in the DHCPv6 client implementation directly so that
 * the sub-prefix derivation used by a requesting router can be exercised on
 * its own. The directory is added to the include path by CMakeLists.txt.
 */
#include "dhcpv6.c"

static void check_prefix(const char *delegated, uint8_t delegated_len,
			 uint32_t index, const char *expected)
{
	struct net_in6_addr in = { 0 };
	struct net_in6_addr want = { 0 };
	struct net_in6_addr out = { 0 };
	uint8_t out_len = 0;
	int ret;

	zassert_ok(net_addr_pton(NET_AF_INET6, delegated, &in), NULL);
	zassert_ok(net_addr_pton(NET_AF_INET6, expected, &want), NULL);

	ret = dhcpv6_derive_downstream_prefix(&in, delegated_len, index, &out,
					      &out_len);
	zassert_ok(ret, "Derivation of %s/%u index %u failed (%d)", delegated,
		   delegated_len, index, ret);
	zassert_equal(out_len, 64, "A /64 must be derived");
	zassert_mem_equal(&out, &want, sizeof(out),
			  "Unexpected sub-prefix for %s/%u index %u", delegated,
			  delegated_len, index);
}

ZTEST(dhcpv6_pd, test_derive_from_short_delegation)
{
	/* A /56 leaves 8 bits of subnet id, so each downstream interface gets
	 * its own /64.
	 */
	check_prefix("2001:db8:abcd:ee00::", 56, 0, "2001:db8:abcd:ee00::");
	check_prefix("2001:db8:abcd:ee00::", 56, 1, "2001:db8:abcd:ee01::");
	check_prefix("2001:db8:abcd:ee00::", 56, 255, "2001:db8:abcd:eeff::");

	/* A /48 leaves 16 bits of subnet id. */
	check_prefix("2001:db8:abcd::", 48, 0, "2001:db8:abcd::");
	check_prefix("2001:db8:abcd::", 48, 1, "2001:db8:abcd:1::");
	check_prefix("2001:db8:abcd::", 48, 0x0102, "2001:db8:abcd:102::");
}

ZTEST(dhcpv6_pd, test_derive_keeps_delegated_bits)
{
	/* The subnet id must only replace the bits between the delegated
	 * length and the /64, the delegated part is left alone.
	 */
	check_prefix("2001:db8:abcd:ff00::", 56, 1, "2001:db8:abcd:ff01::");
}

ZTEST(dhcpv6_pd, test_derive_exact_64_only_serves_one)
{
	struct net_in6_addr in = { 0 };
	struct net_in6_addr out = { 0 };
	uint8_t out_len = 0;

	zassert_ok(net_addr_pton(NET_AF_INET6, "2001:db8:abcd:1::", &in), NULL);

	/* A delegation of exactly a /64 has no subnet id bits left, so it can
	 * serve a single downstream interface only. Every other interface has
	 * to be refused instead of silently getting the same prefix.
	 */
	check_prefix("2001:db8:abcd:1::", 64, 0, "2001:db8:abcd:1::");

	zassert_equal(dhcpv6_derive_downstream_prefix(&in, 64, 1, &out,
						      &out_len),
		      -ENOSPC, "A second /64 must not be derived");
	zassert_equal(dhcpv6_derive_downstream_prefix(&in, 64, 2, &out,
						      &out_len),
		      -ENOSPC, "A third /64 must not be derived");
}

ZTEST(dhcpv6_pd, test_derive_longer_than_64_refused)
{
	struct net_in6_addr in = { 0 };
	struct net_in6_addr out = { 0 };
	uint8_t out_len = 0;

	zassert_ok(net_addr_pton(NET_AF_INET6, "2001:db8:abcd:1:2::", &in),
		   NULL);

	/* Advertising a /64 out of a /80 would cover addresses that were never
	 * delegated to us.
	 */
	zassert_equal(dhcpv6_derive_downstream_prefix(&in, 80, 0, &out,
						      &out_len),
		      -EINVAL, "A prefix longer than /64 must be refused");
}

ZTEST(dhcpv6_pd, test_derive_index_out_of_range)
{
	struct net_in6_addr in = { 0 };
	struct net_in6_addr out = { 0 };
	uint8_t out_len = 0;

	zassert_ok(net_addr_pton(NET_AF_INET6, "2001:db8:abcd:ee00::", &in),
		   NULL);

	/* A /56 holds 256 distinct /64s, numbered 0..255. */
	check_prefix("2001:db8:abcd:ee00::", 56, 255, "2001:db8:abcd:eeff::");

	zassert_equal(dhcpv6_derive_downstream_prefix(&in, 56, 256, &out,
						      &out_len),
		      -ENOSPC, "Subnet id must fit in the delegated prefix");
}

ZTEST_SUITE(dhcpv6_pd, NULL, NULL, NULL, NULL, NULL);
