/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/net_ip.h>

/* White-box test: pull in the server implementation directly so we can
 * exercise its internal binding and pool-allocation logic. The directory is
 * added to the include path by CMakeLists.txt.
 */
#include "dhcpv6_server.c"

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

static void fill_msg(struct dhcpv6_server_msg *msg, uint8_t duid_last)
{
	static const uint8_t duid[] = { 0, 3, 0, 1, 0xaa, 0xbb, 0xcc, 0xdd, 0, 0 };

	memset(msg, 0, sizeof(*msg));
	memcpy(msg->clientid, duid, sizeof(duid));
	msg->clientid[sizeof(duid) - 1] = duid_last;
	msg->clientid_len = sizeof(duid);
	msg->has_clientid = true;
	msg->has_ia_na = true;
	msg->ia_na_iaid = 0x1234;
}

ZTEST(dhcpv6_server, test_pool_exhaustion)
{
	struct dhcpv6_server_binding *b;

	reset_server_ctx();

	for (int i = 0; i < CONFIG_NET_DHCPV6_SERVER_MAX_LEASES; i++) {
		b = dhcpv6_server_alloc_binding();
		zassert_not_null(b, "Binding %d should be available", i);
	}

	zassert_is_null(dhcpv6_server_alloc_binding(),
			"Pool must be exhausted after %d leases",
			CONFIG_NET_DHCPV6_SERVER_MAX_LEASES);
}

ZTEST(dhcpv6_server, test_expired_bindings_are_reclaimed)
{
	struct dhcpv6_server_binding *b;
	int64_t now = k_uptime_get();

	reset_server_ctx();

	/* Fill the pool with leases that have already expired. */
	for (int i = 0; i < CONFIG_NET_DHCPV6_SERVER_MAX_LEASES; i++) {
		b = dhcpv6_server_alloc_binding();
		zassert_not_null(b, NULL);
		b->expiry = now - 1;
	}

	zassert_is_null(dhcpv6_server_alloc_binding(), "Pool should be full");

	dhcpv6_server_reap_bindings();

	for (int i = 0; i < CONFIG_NET_DHCPV6_SERVER_MAX_LEASES; i++) {
		zassert_not_null(dhcpv6_server_alloc_binding(),
				 "Expired lease %d should have been reclaimed", i);
	}
}

ZTEST(dhcpv6_server, test_live_bindings_are_kept)
{
	struct dhcpv6_server_binding *b;
	uint8_t duid[] = { 0, 3, 0, 1, 1, 2, 3, 4, 5, 6 };

	reset_server_ctx();

	b = dhcpv6_server_alloc_binding();
	zassert_not_null(b, NULL);
	memcpy(b->duid, duid, sizeof(duid));
	b->duid_len = sizeof(duid);
	b->expiry = k_uptime_get() + 60 * MSEC_PER_SEC;

	dhcpv6_server_reap_bindings();

	zassert_equal_ptr(dhcpv6_server_find_binding(duid, sizeof(duid)), b,
			  "A lease that has not expired must be kept");
}

ZTEST(dhcpv6_server, test_advertised_binding_expires_early)
{
	struct dhcpv6_server_binding *offered;
	struct dhcpv6_server_binding *committed;
	struct dhcpv6_server_msg msg;
	int64_t now = k_uptime_get();

	reset_server_ctx();

	/* A binding that has only been advertised must not hold on to a lease
	 * for the whole valid lifetime, otherwise a burst of Solicits with
	 * random DUIDs would exhaust the pool.
	 */
	fill_msg(&msg, 1);
	offered = dhcpv6_server_bind(&msg, false);
	zassert_not_null(offered, NULL);
	zassert_true(offered->expiry <= now + DHCPV6_SERVER_OFFER_TIMEOUT_MS,
		     "Advertised binding must expire within the offer timeout");

	fill_msg(&msg, 2);
	committed = dhcpv6_server_bind(&msg, true);
	zassert_not_null(committed, NULL);
	zassert_true(committed->expiry >
		     now + (int64_t)DHCPV6_SERVER_OFFER_TIMEOUT_MS,
		     "Committed binding must use the valid lifetime");

	/* A Request for an offer that is already held keeps the same slot and
	 * upgrades it to the full lifetime.
	 */
	fill_msg(&msg, 1);
	zassert_equal_ptr(dhcpv6_server_bind(&msg, true), offered,
			  "Request must reuse the advertised binding");
	zassert_true(offered->expiry >
		     now + (int64_t)DHCPV6_SERVER_OFFER_TIMEOUT_MS,
		     "Request must extend the lease to the valid lifetime");
}

ZTEST(dhcpv6_server, test_foreach_skips_expired)
{
	struct dhcpv6_server_binding *a;
	struct foreach_ctx ctx = { 0 };
	int ret;

	reset_server_ctx();

	a = dhcpv6_server_alloc_binding();
	zassert_not_null(a, NULL);
	dhcpv6_server_assign_addr(a, 0x11111111);
	a->expiry = k_uptime_get() - 1;

	ret = net_dhcpv6_server_foreach_lease(NULL, foreach_test_cb, &ctx);
	zassert_equal(ret, 0, NULL);
	zassert_equal(ctx.count, 0, "Expired leases must not be reported");
}

ZTEST_SUITE(dhcpv6_server, NULL, NULL, NULL, NULL, NULL);
