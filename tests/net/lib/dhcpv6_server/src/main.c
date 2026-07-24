/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/net_ip.h>

/* White-box test: pull in the server implementation directly so we can
 * exercise its internal binding and pool-allocation logic.
 */
#include "../../../../subsys/net/lib/dhcpv6/dhcpv6_server.c"

static void reset_server_ctx(void)
{
	struct net_in6_addr base = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0 } } };

	memset(&server_ctx, 0, sizeof(server_ctx));
	server_ctx.in_use = true;
	server_ctx.params.offer_addr = true;
	server_ctx.params.offer_prefix = true;
	server_ctx.params.prefix_len = 64;
	net_ipaddr_copy(&server_ctx.params.prefix, &base);
	net_ipaddr_copy(&server_ctx.params.addr, &base);
}

ZTEST(dhcpv6_server, test_binding_alloc_find_free)
{
	struct dhcpv6_server_binding *a;
	struct dhcpv6_server_binding *b;
	uint8_t duid_a[] = { 0, 3, 0, 1, 1, 2, 3, 4, 5, 6 };
	uint8_t duid_b[] = { 0, 3, 0, 1, 9, 8, 7, 6, 5, 4 };

	reset_server_ctx();

	a = dhcpv6_server_alloc_binding();
	zassert_not_null(a, "Expected a free binding");
	memcpy(a->duid, duid_a, sizeof(duid_a));
	a->duid_len = sizeof(duid_a);

	zassert_equal_ptr(dhcpv6_server_find_binding(duid_a, sizeof(duid_a)), a,
			  "Lookup should return the allocated binding");
	zassert_is_null(dhcpv6_server_find_binding(duid_b, sizeof(duid_b)),
			"Unknown DUID must not match");

	b = dhcpv6_server_alloc_binding();
	zassert_not_null(b, "Expected a second free binding");
	zassert_not_equal(a, b, "Bindings must be distinct");

	dhcpv6_server_free_binding(a);
	zassert_is_null(dhcpv6_server_find_binding(duid_a, sizeof(duid_a)),
			"Freed binding must no longer match");
}

ZTEST(dhcpv6_server, test_prefix_allocation_unique)
{
	struct dhcpv6_server_binding *a;
	struct dhcpv6_server_binding *b;

	reset_server_ctx();

	a = dhcpv6_server_alloc_binding();
	b = dhcpv6_server_alloc_binding();
	zassert_not_null(a, NULL);
	zassert_not_null(b, NULL);

	dhcpv6_server_assign_prefix(a, 0x11111111);
	dhcpv6_server_assign_prefix(b, 0x22222222);

	zassert_true(a->has_prefix, NULL);
	zassert_true(b->has_prefix, NULL);
	zassert_equal(a->prefix_len, 64, NULL);

	/* Distinct clients must receive distinct delegated prefixes. */
	zassert_true(memcmp(&a->prefix, &b->prefix,
			    sizeof(struct net_in6_addr)) != 0,
		     "Delegated prefixes must be unique");

	/* Subnet id lives in byte 7 for a /64. */
	zassert_not_equal(a->prefix.s6_addr[7], b->prefix.s6_addr[7], NULL);
}

ZTEST(dhcpv6_server, test_addr_allocation_unique)
{
	struct dhcpv6_server_binding *a;
	struct dhcpv6_server_binding *b;

	reset_server_ctx();

	a = dhcpv6_server_alloc_binding();
	b = dhcpv6_server_alloc_binding();

	dhcpv6_server_assign_addr(a, 0x11111111);
	dhcpv6_server_assign_addr(b, 0x22222222);

	zassert_true(a->has_addr, NULL);
	zassert_true(b->has_addr, NULL);
	zassert_true(memcmp(&a->addr, &b->addr,
			    sizeof(struct net_in6_addr)) != 0,
		     "Leased addresses must be unique");
	/* Host id offset avoids the ::0 address. */
	zassert_not_equal(a->addr.s6_addr[15], 0, NULL);
}

ZTEST(dhcpv6_server, test_default_lifetimes)
{
	reset_server_ctx();

	zassert_equal(dhcpv6_server_valid_lifetime(),
		      CONFIG_NET_DHCPV6_SERVER_VALID_LIFETIME, NULL);
	zassert_equal(dhcpv6_server_preferred_lifetime(),
		      CONFIG_NET_DHCPV6_SERVER_PREFERRED_LIFETIME, NULL);

	server_ctx.params.valid_lifetime = 1234;
	server_ctx.params.preferred_lifetime = 567;
	zassert_equal(dhcpv6_server_valid_lifetime(), 1234, NULL);
	zassert_equal(dhcpv6_server_preferred_lifetime(), 567, NULL);
}

struct foreach_ctx {
	int count;
	struct net_in6_addr last_addr;
	bool last_has_addr;
};

static void foreach_test_cb(struct net_if *iface,
			    struct net_dhcpv6_server_lease *lease,
			    void *user_data)
{
	struct foreach_ctx *ctx = user_data;

	ARG_UNUSED(iface);

	ctx->count++;
	ctx->last_has_addr = lease->has_addr;
	if (lease->has_addr) {
		net_ipaddr_copy(&ctx->last_addr, &lease->addr);
	}
}

ZTEST(dhcpv6_server, test_foreach_lease)
{
	struct dhcpv6_server_binding *a;
	struct foreach_ctx ctx = { 0 };
	int ret;

	reset_server_ctx();

	/* No bindings yet: callback must not be invoked. */
	ret = net_dhcpv6_server_foreach_lease(NULL, foreach_test_cb, &ctx);
	zassert_equal(ret, 0, "foreach should succeed when server is running");
	zassert_equal(ctx.count, 0, "No leases expected yet");

	a = dhcpv6_server_alloc_binding();
	zassert_not_null(a, NULL);
	dhcpv6_server_assign_addr(a, 0x11111111);

	ctx.count = 0;
	ret = net_dhcpv6_server_foreach_lease(NULL, foreach_test_cb, &ctx);
	zassert_equal(ret, 0, NULL);
	zassert_equal(ctx.count, 1, "Expected the allocated binding");
	zassert_true(ctx.last_has_addr, "Lease should carry the IA_NA address");

	/* When the server is not running, foreach reports -ENOENT. */
	server_ctx.in_use = false;
	ctx.count = 0;
	ret = net_dhcpv6_server_foreach_lease(NULL, foreach_test_cb, &ctx);
	zassert_equal(ret, -ENOENT, "Expected -ENOENT when server stopped");
	zassert_equal(ctx.count, 0, NULL);
}

ZTEST_SUITE(dhcpv6_server, NULL, NULL, NULL, NULL, NULL);
