/*
 * Copyright (c) 2024, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/posix/net/if.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

ZTEST(net, test_if_indextoname)
{
	char *name;
	size_t n = 0;
	struct net_if *iface;
	char a[IF_NAMESIZE];
	char b[IF_NAMESIZE];

	NET_IFACE_COUNT(&n);
	TC_PRINT("%zu interfaces\n", n);
	for (size_t i = 0; i < n; i++) {
		memset(a, 0, sizeof(a));
		memset(b, 0, sizeof(b));
		name = if_indextoname(i + 1, a);
		zassert_equal(name, a);
		TC_PRINT("interface %zu: %s\n", i + 1, name);
		iface = net_if_get_by_index(i + 1);
		zassert_not_null(iface);
		zassert_true(net_if_get_name(iface, b, IF_NAMESIZE) >= 0);
		zassert_mem_equal(a, b, IF_NAMESIZE);
	}
}

ZTEST(net, test_if_freenameindex)
{
	if_freenameindex(NULL);
	if_freenameindex(if_nameindex());
}

ZTEST(net, test_if_nameindex)
{
	size_t n = 0;
	struct if_nameindex *ni;

	NET_IFACE_COUNT(&n);
	TC_PRINT("%zu interfaces\n", n);

	ni = if_nameindex();
	if (ni == NULL) {
		zassert_equal(errno, ENOBUFS);
		return;
	}

	for (size_t i = 0; i < n; i++) {
		zassert_equal(i + 1, ni[i].if_index);
		zassert_not_null(ni[i].if_name);
		TC_PRINT("interface %zu: %s\n", i + 1, ni[i].if_name);
		zassert_equal(0, ni[n].if_index);
		zassert_is_null(ni[n].if_name);
	}

	if_freenameindex(ni);
}

ZTEST(net, test_if_nametoindex)
{
	size_t n = 0;
	char buf[IF_NAMESIZE];

	NET_IFACE_COUNT(&n);
	TC_PRINT("%zu interfaces\n", n);
	for (size_t i = 0; i < n; i++) {
		memset(buf, 0, sizeof(buf));
		zassert_not_null(if_indextoname(i + 1, buf));
		TC_PRINT("interface %zu: %s\n", i + 1, buf);
		zassert_equal(i + 1, if_nametoindex(buf));
	}
}
