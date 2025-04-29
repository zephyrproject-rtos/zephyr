/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_UTILS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/linker/sections.h>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

struct net_addr_test_data {
	sa_family_t family;
	bool pton;

	struct {
		char c_addr[16];
		char c_verify[16];
		struct in_addr addr;
		struct in_addr verify;
	} ipv4;

	struct {
		char c_addr[46];
		char c_verify[46];
		struct in6_addr addr;
		struct in6_addr verify;
	} ipv6;
};

static ZTEST_DMEM struct net_addr_test_data ipv4_pton_1 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "192.0.0.1",
	.ipv4.verify.s4_addr = { 192, 0, 0, 1 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_pton_2 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "192.1.0.0",
	.ipv4.verify.s4_addr = { 192, 1, 0, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_pton_3 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "192.0.0.0",
	.ipv4.verify.s4_addr = { 192, 0, 0, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_pton_4 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "255.255.255.255",
	.ipv4.verify.s4_addr = { 255, 255, 255, 255 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_pton_5 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "0.0.0.0",
	.ipv4.verify.s4_addr = { 0, 0, 0, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_pton_6 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "0.0.0.1",
	.ipv4.verify.s4_addr = { 0, 0, 0, 1 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_pton_7 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "0.0.1.0",
	.ipv4.verify.s4_addr = { 0, 0, 1, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_pton_8 = {
	.family = AF_INET,
	.pton = true,
	.ipv4.c_addr = "0.1.0.0",
	.ipv4.verify.s4_addr = { 0, 1, 0, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_pton_1 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "ff08::",
	.ipv6.verify.s6_addr16 = { htons(0xff08), 0, 0, 0, 0, 0, 0, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_pton_2 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "::",
	.ipv6.verify.s6_addr16 = { 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_pton_3 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "ff08::1",
	.ipv6.verify.s6_addr16 = { htons(0xff08), 0, 0, 0, 0, 0, 0, htons(1) },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_pton_4 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "2001:db8::1",
	.ipv6.verify.s6_addr16 = { htons(0x2001), htons(0xdb8),
				   0, 0, 0, 0, 0, htons(1) },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_pton_5 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "2001:db8::2:1",
	.ipv6.verify.s6_addr16 = { htons(0x2001), htons(0xdb8),
				   0, 0, 0, 0, htons(2), htons(1) },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_pton_6 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "ff08:1122:3344:5566:7788:9900:aabb:ccdd",
	.ipv6.verify.s6_addr16 = { htons(0xff08), htons(0x1122),
				   htons(0x3344), htons(0x5566),
				   htons(0x7788), htons(0x9900),
				   htons(0xaabb), htons(0xccdd) },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_pton_7 = {
	.family = AF_INET6,
	.pton = true,
	.ipv6.c_addr = "0:ff08::",
	.ipv6.verify.s6_addr16 = { 0, htons(0xff08), 0, 0, 0, 0, 0, 0 },
};

/* net_addr_ntop test cases */
static ZTEST_DMEM struct net_addr_test_data ipv4_ntop_1 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "192.0.0.1",
	.ipv4.addr.s4_addr = { 192, 0, 0, 1 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_ntop_2 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "192.1.0.0",
	.ipv4.addr.s4_addr = { 192, 1, 0, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_ntop_3 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "192.0.0.0",
	.ipv4.addr.s4_addr = { 192, 0, 0, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_ntop_4 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "255.255.255.255",
	.ipv4.addr.s4_addr = { 255, 255, 255, 255 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_ntop_5 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "0.0.0.0",
	.ipv4.addr.s4_addr = { 0, 0, 0, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_ntop_6 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "0.0.0.1",
	.ipv4.addr.s4_addr = { 0, 0, 0, 1 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_ntop_7 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "0.0.1.0",
	.ipv4.addr.s4_addr = { 0, 0, 1, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv4_ntop_8 = {
	.family = AF_INET,
	.pton = false,
	.ipv4.c_verify = "0.1.0.0",
	.ipv4.addr.s4_addr = { 0, 1, 0, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_ntop_1 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "ff08::",
	.ipv6.addr.s6_addr16 = { htons(0xff08), 0, 0, 0, 0, 0, 0, 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_ntop_2 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "::",
	.ipv6.addr.s6_addr16 = { 0 },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_ntop_3 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "ff08::1",
	.ipv6.addr.s6_addr16 = { htons(0xff08), 0, 0, 0, 0, 0, 0, htons(1) },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_ntop_4 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "2001:db8::1",
	.ipv6.addr.s6_addr16 = { htons(0x2001), htons(0xdb8),
				 0, 0, 0, 0, 0, htons(1) },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_ntop_5 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "2001:db8::2:1",
	.ipv6.addr.s6_addr16 = { htons(0x2001), htons(0xdb8),
				 0, 0, 0, 0, htons(2), htons(1) },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_ntop_6 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "ff08:1122:3344:5566:7788:9900:aabb:ccdd",
	.ipv6.addr.s6_addr16 = { htons(0xff08), htons(0x1122),
				 htons(0x3344), htons(0x5566),
				 htons(0x7788), htons(0x9900),
				 htons(0xaabb), htons(0xccdd) },
};

static ZTEST_DMEM struct net_addr_test_data ipv6_ntop_7 = {
	.family = AF_INET6,
	.pton = false,
	.ipv6.c_verify = "0:ff08::",
	.ipv6.addr.s6_addr16 = { 0, htons(0xff08), 0, 0, 0, 0, 0, 0 },
};

static const struct {
	const char *name;
	struct net_addr_test_data *data;
} tests[] = {
	/* IPv4 net_addr_pton */
	{ "test_ipv4_pton_1", &ipv4_pton_1},
	{ "test_ipv4_pton_2", &ipv4_pton_2},
	{ "test_ipv4_pton_3", &ipv4_pton_3},
	{ "test_ipv4_pton_4", &ipv4_pton_4},
	{ "test_ipv4_pton_5", &ipv4_pton_5},
	{ "test_ipv4_pton_6", &ipv4_pton_6},
	{ "test_ipv4_pton_7", &ipv4_pton_7},
	{ "test_ipv4_pton_8", &ipv4_pton_8},

	/* IPv6 net_addr_pton */
	{ "test_ipv6_pton_1", &ipv6_pton_1},
	{ "test_ipv6_pton_2", &ipv6_pton_2},
	{ "test_ipv6_pton_3", &ipv6_pton_3},
	{ "test_ipv6_pton_4", &ipv6_pton_4},
	{ "test_ipv6_pton_5", &ipv6_pton_5},
	{ "test_ipv6_pton_6", &ipv6_pton_6},
	{ "test_ipv6_pton_7", &ipv6_pton_7},

	/* IPv4 net_addr_ntop */
	{ "test_ipv4_ntop_1", &ipv4_ntop_1},
	{ "test_ipv4_ntop_2", &ipv4_ntop_2},
	{ "test_ipv4_ntop_3", &ipv4_ntop_3},
	{ "test_ipv4_ntop_4", &ipv4_ntop_4},
	{ "test_ipv4_ntop_5", &ipv4_ntop_5},
	{ "test_ipv4_ntop_6", &ipv4_ntop_6},
	{ "test_ipv4_ntop_7", &ipv4_ntop_7},
	{ "test_ipv4_ntop_8", &ipv4_ntop_8},

	/* IPv6 net_addr_ntop */
	{ "test_ipv6_ntop_1", &ipv6_ntop_1},
	{ "test_ipv6_ntop_2", &ipv6_ntop_2},
	{ "test_ipv6_ntop_3", &ipv6_ntop_3},
	{ "test_ipv6_ntop_4", &ipv6_ntop_4},
	{ "test_ipv6_ntop_5", &ipv6_ntop_5},
	{ "test_ipv6_ntop_6", &ipv6_ntop_6},
	{ "test_ipv6_ntop_7", &ipv6_ntop_7},
};

static bool check_net_addr(struct net_addr_test_data *data)
{
	switch (data->family) {
	case AF_INET:
		if (data->pton) {
			if (net_addr_pton(AF_INET, (char *)data->ipv4.c_addr,
					  &data->ipv4.addr) < 0) {
				printk("Failed to convert %s\n",
				       data->ipv4.c_addr);

				return false;
			}

			if (!net_ipv4_addr_cmp(&data->ipv4.addr,
					       &data->ipv4.verify)) {
				printk("Failed to verify %s\n",
				       data->ipv4.c_addr);

				return false;
			}
		} else {
			if (!net_addr_ntop(AF_INET, &data->ipv4.addr,
					   data->ipv4.c_addr,
					   sizeof(data->ipv4.c_addr))) {
				printk("Failed to convert %s\n",
				       net_sprint_ipv4_addr(&data->ipv4.addr));

				return false;
			}

			if (strcmp(data->ipv4.c_addr, data->ipv4.c_verify)) {
				printk("Failed to verify %s\n",
				       data->ipv4.c_addr);
				printk("against %s\n",
				       data->ipv4.c_verify);

				return false;
			}
		}

		break;

	case AF_INET6:
		if (data->pton) {
			if (net_addr_pton(AF_INET6, (char *)data->ipv6.c_addr,
					  &data->ipv6.addr) < 0) {
				printk("Failed to convert %s\n",
				       data->ipv6.c_addr);

				return false;
			}

			if (!net_ipv6_addr_cmp(&data->ipv6.addr,
					       &data->ipv6.verify)) {
				printk("Failed to verify %s\n",
				       net_sprint_ipv6_addr(&data->ipv6.addr));
				printk("against %s\n",
				       net_sprint_ipv6_addr(
							&data->ipv6.verify));

				return false;
			}
		} else {
			if (!net_addr_ntop(AF_INET6, &data->ipv6.addr,
					   data->ipv6.c_addr,
					   sizeof(data->ipv6.c_addr))) {
				printk("Failed to convert %s\n",
				       net_sprint_ipv6_addr(&data->ipv6.addr));

				return false;
			}

			if (strcmp(data->ipv6.c_addr, data->ipv6.c_verify)) {
				printk("Failed to verify %s\n",
				       data->ipv6.c_addr);
				printk("against %s\n",
				       data->ipv6.c_verify);

				return false;
			}

		}

		break;
	}

	return true;
}

ZTEST(test_utils_fn, test_net_addr)
{
	int count, pass;

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_PRINT("Running test: %s: ", tests[count].name);

		if (check_net_addr(tests[count].data)) {
			TC_PRINT("passed\n");
			pass++;
		} else {
			TC_PRINT("failed\n");
		}
	}

	zassert_equal(pass, ARRAY_SIZE(tests), "check_net_addr error");
}

ZTEST(test_utils_fn, test_addr_parse)
{
	struct sockaddr addr;
	bool ret;
	int i;
#if defined(CONFIG_NET_IPV4)
	static const struct {
		const char *address;
		int len;
		struct sockaddr_in result;
		bool verdict;
	} parse_ipv4_entries[] = {
		{
			.address = "192.0.2.1:80",
			.len = sizeof("192.0.2.1:80") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = htons(80),
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 1
				}
			},
			.verdict = true
		},
		{
			.address = "192.0.2.2",
			.len = sizeof("192.0.2.2") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 2
				}
			},
			.verdict = true
		},
		{
			.address = "192.0.2.3/foobar",
			.len = sizeof("192.0.2.3/foobar") - 8,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 3
				}
			},
			.verdict = true
		},
		{
			.address = "255.255.255.255:0",
			.len = sizeof("255.255.255.255:0") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 255,
					.s4_addr[1] = 255,
					.s4_addr[2] = 255,
					.s4_addr[3] = 255
				}
			},
			.verdict = true
		},
		{
			.address = "127.0.0.42:65535",
			.len = sizeof("127.0.0.42:65535") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = htons(65535),
				.sin_addr = {
					.s4_addr[0] = 127,
					.s4_addr[1] = 0,
					.s4_addr[2] = 0,
					.s4_addr[3] = 42
				}
			},
			.verdict = true
		},
		{
			.address = "192.0.2.3:80/foobar",
			.len = sizeof("192.0.2.3:80/foobar") - 1,
			.verdict = false
		},
		{
			.address = "192.168.1.1:65536/foobar",
			.len = sizeof("192.168.1.1:65536") - 1,
			.verdict = false
		},
		{
			.address = "192.0.2.3:80/foobar",
			.len = sizeof("192.0.2.3") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 3
				}
			},
			.verdict = true
		},
		{
			.address = "192.0.2.3:80/foobar",
			.len = sizeof("192.0.2.3:80") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = htons(80),
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 3
				}
			},
			.verdict = true
		},
		{
			.address = "192.0.2.3/foobar",
			.len = sizeof("192.0.2.3/foobar") - 1,
			.verdict = false
		},
		{
			.address = "192.0.2.3:80:80",
			.len = sizeof("192.0.2.3:80:80") - 1,
			.verdict = false
		},
		{
			.address = "192.0.2.1:80000",
			.len = sizeof("192.0.2.1:80000") - 1,
			.verdict = false
		},
		{
			.address = "192.168.0.1",
			.len = sizeof("192.168.0.1:80000") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 168,
					.s4_addr[2] = 0,
					.s4_addr[3] = 1
				}
			},
			.verdict = true
		},
		{
			.address = "a.b.c.d",
			.verdict = false
		},
	};
#endif
#if defined(CONFIG_NET_IPV6)
	static const struct {
		const char *address;
		int len;
		struct sockaddr_in6 result;
		bool verdict;
	} parse_ipv6_entries[] = {
		{
			.address = "[2001:db8::2]:80",
			.len = sizeof("[2001:db8::2]:80") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = htons(80),
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(2)
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8::a]/barfoo",
			.len = sizeof("[2001:db8::a]/barfoo") - 8,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0xa)
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8::a]",
			.len = sizeof("[2001:db8::a]") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0xa)
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8:3:4:5:6:7:8]:65535",
			.len = sizeof("[2001:db8:3:4:5:6:7:8]:65535") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 65535,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[2] = ntohs(3),
					.s6_addr16[3] = ntohs(4),
					.s6_addr16[4] = ntohs(5),
					.s6_addr16[5] = ntohs(6),
					.s6_addr16[6] = ntohs(7),
					.s6_addr16[7] = ntohs(8),
				}
			},
			.verdict = true
		},
		{
			.address = "[::]:0",
			.len = sizeof("[::]:0") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = 0,
					.s6_addr16[1] = 0,
					.s6_addr16[2] = 0,
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = 0,
				}
			},
			.verdict = true
		},
		{
			.address = "2001:db8::42",
			.len = sizeof("2001:db8::42") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0x42)
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8::192.0.2.1]:80000",
			.len = sizeof("[2001:db8::192.0.2.1]:80000") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:80",
			.len = sizeof("[2001:db8::1") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:65536",
			.len = sizeof("[2001:db8::1]:65536") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:80",
			.len = sizeof("2001:db8::1") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:a",
			.len = sizeof("[2001:db8::1]:a") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:10-12",
			.len = sizeof("[2001:db8::1]:10-12") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::]:80/url/continues",
			.len = sizeof("[2001:db8::]") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = 0,
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8::200]:080",
			.len = sizeof("[2001:db8:433:2]:80000") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = htons(80),
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0x200)
				}
			},
			.verdict = true
		},
		{
			.address = "[2001:db8::]:8080/another/url",
			.len = sizeof("[2001:db8::]:8080/another/url") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1",
			.len = sizeof("[2001:db8::1") - 1,
			.verdict = false
		},
		{
			.address = "[2001:db8::1]:-1",
			.len = sizeof("[2001:db8::1]:-1") - 1,
			.verdict = false
		},
		{
			/* Valid although user probably did not mean this */
			.address = "2001:db8::1:80",
			.len = sizeof("2001:db8::1:80") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = ntohs(0x01),
					.s6_addr16[7] = ntohs(0x80)
				}
			},
			.verdict = true
		},
	};
#endif

#if defined(CONFIG_NET_IPV4)
	for (i = 0; i < ARRAY_SIZE(parse_ipv4_entries) - 1; i++) {
		(void)memset(&addr, 0, sizeof(addr));

		ret = net_ipaddr_parse(
			parse_ipv4_entries[i].address,
			parse_ipv4_entries[i].len,
			&addr);
		if (ret != parse_ipv4_entries[i].verdict) {
			printk("IPv4 entry [%d] \"%s\" failed\n", i,
				parse_ipv4_entries[i].address);
			zassert_true(false, "failure");
		}

		if (ret == true) {
			zassert_true(
				net_ipv4_addr_cmp(
				      &net_sin(&addr)->sin_addr,
				      &parse_ipv4_entries[i].result.sin_addr),
				parse_ipv4_entries[i].address);
			zassert_true(net_sin(&addr)->sin_port ==
				     parse_ipv4_entries[i].result.sin_port,
				     "IPv4 port");
			zassert_true(net_sin(&addr)->sin_family ==
				     parse_ipv4_entries[i].result.sin_family,
				     "IPv4 family");
		}
	}
#endif
#if defined(CONFIG_NET_IPV6)
	for (i = 0; i < ARRAY_SIZE(parse_ipv6_entries) - 1; i++) {
		(void)memset(&addr, 0, sizeof(addr));

		ret = net_ipaddr_parse(
			parse_ipv6_entries[i].address,
			parse_ipv6_entries[i].len,
			&addr);
		if (ret != parse_ipv6_entries[i].verdict) {
			printk("IPv6 entry [%d] \"%s\" failed\n", i,
			       parse_ipv6_entries[i].address);
			zassert_true(false, "failure");
		}

		if (ret == true) {
			zassert_true(
				net_ipv6_addr_cmp(
				      &net_sin6(&addr)->sin6_addr,
				      &parse_ipv6_entries[i].result.sin6_addr),
				parse_ipv6_entries[i].address);
			zassert_true(net_sin6(&addr)->sin6_port ==
				     parse_ipv6_entries[i].result.sin6_port,
				     "IPv6 port");
			zassert_true(net_sin6(&addr)->sin6_family ==
				     parse_ipv6_entries[i].result.sin6_family,
				     "IPv6 family");
		}
	}
#endif
}

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
static const char *check_ipaddr(const char *addresses)
{
	do {
		char addr_str[NET_IPV6_ADDR_LEN + 4 + 1];
		char expecting[NET_IPV6_ADDR_LEN + 4 + 1];
		const char *orig = addresses;
		struct sockaddr_storage addr;
		struct sockaddr_storage mask;
		uint8_t mask_len;
		int ret;

		memset(&addr, 0, sizeof(addr));
		mask_len = 0;

		memset(addr_str, 0, sizeof(addr_str));
		memset(expecting, 0, sizeof(expecting));

		addresses = net_ipaddr_parse_mask(addresses, strlen(addresses),
						  (struct sockaddr *)&addr, &mask_len);
		zassert_not_null(addresses, "Invalid parse, expecting \"%s\"", orig);

		strncpy(expecting, orig,
			*addresses == '\0' ? strlen(orig) : addresses - orig - 1);

		(void)net_addr_ntop(addr.ss_family,
				    &net_sin((struct sockaddr *)&addr)->sin_addr,
				    addr_str, sizeof(addr_str));

		ret = net_mask_len_to_netmask(addr.ss_family, mask_len,
					      (struct sockaddr *)&mask);
		zassert_equal(ret, 0, "Failed to convert mask %d", mask_len);

		ret = net_netmask_to_mask_len(addr.ss_family,
					      (struct sockaddr *)&mask,
					      &mask_len);
		zassert_equal(ret, 0, "Failed to convert mask %s",
			      net_sprint_addr(addr.ss_family,
					      (const void *)&net_sin(
						      ((struct sockaddr *)&mask))->sin_addr));

		if (net_sin((struct sockaddr *)&mask)->sin_addr.s_addr != 0) {
			int addr_len = strlen(addr_str);

			snprintk(addr_str + addr_len,
				 sizeof(addr_str) - addr_len,
				 "/%d", mask_len);
		}

		zassert_mem_equal(addr_str, expecting,
				  *addresses == '\0' ? strlen(orig) : addresses - orig - 1,
				  "Address mismatch, expecing %s, got %s (len %d)\n",
				  expecting, addr_str, addresses - orig - 1);
	} while (addresses != NULL && *addresses != '\0');

	return addresses;
}
#endif /* CONFIG_NET_IPV4 && CONFIG_NET_IPV6 */

ZTEST(test_utils_fn, test_addr_parse_mask)
{
	struct sockaddr addr;
	uint8_t mask_len;
	const char *next;
	int i;
#if defined(CONFIG_NET_IPV4)
	static const struct {
		const char *address;
		int len;
		struct sockaddr_in result;
		uint8_t mask_len;
		const char *verdict;
	} parse_ipv4_entries[] = {
		{
			.address = "192.0.2.1:80",
			.len = sizeof("192.0.2.1:80") - 1,
			.verdict = NULL,
		},
		{
			.address = "192.0.2.2",
			.len = sizeof("192.0.2.2") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 2
				}
			},
			.verdict = "",
		},
		{
			.address = "192.0.2.3/foobar",
			.len = sizeof("192.0.2.3/foobar") - 1,
			.verdict = NULL,
		},
		{
			.address = "127.0.0.42,1.2.3.4",
			.len = sizeof("127.0.0.42,1.2.3.4") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_addr = {
					.s4_addr[0] = 127,
					.s4_addr[1] = 0,
					.s4_addr[2] = 0,
					.s4_addr[3] = 42
				}
			},
			.mask_len = 32,
			.verdict = &"127.0.0.42,1.2.3.4"[11],
		},
		{
			.address = "127.0.0.42/8,1.2.3.4",
			.len = sizeof("127.0.0.42/8,1.2.3.4") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_addr = {
					.s4_addr[0] = 127,
					.s4_addr[1] = 0,
					.s4_addr[2] = 0,
					.s4_addr[3] = 42
				}
			},
			.mask_len = 8,
			.verdict = &"127.0.0.42/8,1.2.3.4"[13],
		},
		{
			.address = "192.0.2.3:80/foobar",
			.len = sizeof("192.0.2.3:80/foobar") - 1,
			.verdict = false
		},
		{
			.address = "192.168.1.1:65536/foobar",
			.len = sizeof("192.168.1.1:65536") - 1,
			.verdict = false
		},
		{
			.address = "192.0.2.3:80/foobar",
			.len = sizeof("192.0.2.3") - 1,
			.result = {
				.sin_family = AF_INET,
				.sin_port = 0,
				.sin_addr = {
					.s4_addr[0] = 192,
					.s4_addr[1] = 0,
					.s4_addr[2] = 2,
					.s4_addr[3] = 3
				}
			},
			.verdict = "",
		},
		{
			.address = "192.0.2.3:80/foobar",
			.len = sizeof("192.0.2.3:80") - 1,
			.verdict = NULL,
		},
		{
			.address = "a.b.c.d",
			.verdict = false
		},
	};
#endif
#if defined(CONFIG_NET_IPV6)
	static const struct {
		const char *address;
		int len;
		struct sockaddr_in6 result;
		uint8_t mask_len;
		const char *verdict;
	} parse_ipv6_entries[] = {
		{
			.address = "2001:db8::2",
			.len = sizeof("2001:db8::2") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(2)
				}
			},
			.verdict = "",
		},
		{
			.address = "2001:db8::a/barfoo",
			.len = sizeof("2001:db8::a/barfoo") - 8,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0xa)
				}
			},
			.verdict = "",
		},
		{
			.address = "2001:db8::a",
			.len = sizeof("2001:db8::a") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0xa)
				}
			},
			.verdict = "",
		},
		{
			.address = "[2001:db8:3:4:5:6:7:8]:65535",
			.len = sizeof("[2001:db8:3:4:5:6:7:8]:65535") - 1,
			.verdict = NULL,
		},
		{
			.address = "[::]:0",
			.len = sizeof("[::]:0") - 1,
			.verdict = NULL,
		},
		{
			.address = "2001:db8::42",
			.len = sizeof("2001:db8::42") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_port = 0,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0x42)
				}
			},
			.verdict = "",
		},
		{
			.address = "[2001:db8::192.0.2.1]:80000",
			.len = sizeof("[2001:db8::192.0.2.1]:80000") - 1,
			.verdict = NULL,
		},
		{
			.address = "[2001:db8::1]:80",
			.len = sizeof("[2001:db8::1") - 1,
			.verdict = NULL,
		},
		{
			.address = "[2001:db8::1]:65536",
			.len = sizeof("[2001:db8::1]:65536") - 1,
			.verdict = NULL,
		},
		{
			.address = "[2001:db8::1]:80",
			.len = sizeof("2001:db8::1") - 1,
			.verdict = NULL,
		},
		{
			.address = "[2001:db8::1]:a",
			.len = sizeof("[2001:db8::1]:a") - 1,
			.verdict = NULL,
		},
		{
			.address = "[2001:db8::1]:10-12",
			.len = sizeof("[2001:db8::1]:10-12") - 1,
			.verdict = NULL,
		},
		{
			.address = "[2001:db8::]:80/url/continues",
			.len = sizeof("[2001:db8::]") - 1,
			.verdict = NULL,
		},
		{
			.address = "[2001:db8::200]:080",
			.len = sizeof("[2001:db8:433:2]:80000") - 1,
			.verdict = NULL,
		},
		{
			.address = "[2001:db8::]:8080/another/url",
			.len = sizeof("[2001:db8::]:8080/another/url") - 1,
			.verdict = NULL,
		},
		{
			.address = "[2001:db8::1",
			.len = sizeof("[2001:db8::1") - 1,
			.verdict = NULL,
		},
		{
			.address = "[2001:db8::1]:-1",
			.len = sizeof("[2001:db8::1]:-1") - 1,
			.verdict = NULL,
		},
		{
			/* Valid although user probably did not mean this */
			.address = "2001:db8::1:80",
			.len = sizeof("2001:db8::1:80") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = ntohs(0x01),
					.s6_addr16[7] = ntohs(0x80)
				}
			},
			.verdict = "",
		},
		{
			.address = "2001:db8::1/64,2001:db8::2",
			.len = sizeof("2001:db8::1/64,2001:db8::2") - 1,
			.result = {
				.sin6_family = AF_INET6,
				.sin6_addr = {
					.s6_addr16[0] = ntohs(0x2001),
					.s6_addr16[1] = ntohs(0xdb8),
					.s6_addr16[2] = 0,
					.s6_addr16[3] = 0,
					.s6_addr16[4] = 0,
					.s6_addr16[5] = 0,
					.s6_addr16[6] = 0,
					.s6_addr16[7] = ntohs(0x01)
				}
			},
			.mask_len = 64,
			.verdict = &"2001:db8::1/64,2001:db8::2"[14],
		},
	};
#endif

#if defined(CONFIG_NET_IPV4)
	for (i = 0; i < ARRAY_SIZE(parse_ipv4_entries) - 1; i++) {
		(void)memset(&addr, 0, sizeof(addr));
		mask_len = 0;

		next = net_ipaddr_parse_mask(
			parse_ipv4_entries[i].address,
			parse_ipv4_entries[i].len,
			&addr, &mask_len);
		if (next != parse_ipv4_entries[i].verdict) {
			printk("IPv4 entry [%d] \"%s\" failed\n", i,
				parse_ipv4_entries[i].address);
			printk("Points to \"%s\" but should point to \"%s\"\n",
			       next == NULL ? "NULL" : next,
			       parse_ipv4_entries[i].verdict == NULL ?
			       "NULL" : parse_ipv4_entries[i].verdict);
			zassert_true(false, "failure");
		}

		if (next != NULL && *next == '\0') {
			zassert_true(
				net_ipv4_addr_cmp(
				      &net_sin(&addr)->sin_addr,
				      &parse_ipv4_entries[i].result.sin_addr),
				parse_ipv4_entries[i].address);
			zassert_true(net_sin(&addr)->sin_port == 0,
				     "IPv4 port");
			zassert_true(net_sin(&addr)->sin_family ==
				     parse_ipv4_entries[i].result.sin_family,
				     "IPv4 family");
		}
	}
#endif
#if defined(CONFIG_NET_IPV6)
	for (i = 0; i < ARRAY_SIZE(parse_ipv6_entries) - 1; i++) {
		(void)memset(&addr, 0, sizeof(addr));
		mask_len = 0;

		next = net_ipaddr_parse_mask(
			parse_ipv6_entries[i].address,
			parse_ipv6_entries[i].len,
			&addr, &mask_len);
		if (next != parse_ipv6_entries[i].verdict) {
			printk("IPv6 entry [%d] \"%s\" failed\n", i,
			       parse_ipv6_entries[i].address);
			zassert_true(false, "failure");
		}

		if (next != NULL && *next == '\0') {
			zassert_true(
				net_ipv6_addr_cmp(
				      &net_sin6(&addr)->sin6_addr,
				      &parse_ipv6_entries[i].result.sin6_addr),
				parse_ipv6_entries[i].address);
			zassert_true(net_sin6(&addr)->sin6_port == 0,
				     "IPv6 port");
			zassert_true(net_sin6(&addr)->sin6_family ==
				     parse_ipv6_entries[i].result.sin6_family,
				     "IPv6 family");
		}
	}
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
	static const char * const addresses[] = {
		"2001:db8::1/64,192.0.2.1,2001:db8::2,192.0.2.2/24",
		"2001:db8::1/64 192.0.2.1 2001:db8::2 192.0.2.2/24",
		"2001:db8::1/64 192.0.2.1,2001:db8::2 192.0.2.2/24",
		NULL
	};

	i = 0;

	while (addresses[i] != NULL) {
		next = check_ipaddr(addresses[i]);
		zassert_true(next != NULL && *next == '\0',
			     "Invalid parse, expecting \"\", got %p\n", next);
		i++;
	}
#endif /* CONFIG_NET_IPV4 && CONFIG_NET_IPV6 */
}

static uint16_t calc_chksum_ref(uint16_t sum, const uint8_t *data, size_t len)
{
	const uint8_t *end;
	uint16_t tmp;

	end = data + len - 1;

	while (data < end) {
		tmp = (data[0] << 8) + data[1];
		sum += tmp;
		if (sum < tmp) {
			sum++;
		}

		data += 2;
	}

	if (data == end) {
		tmp = data[0] << 8;
		sum += tmp;
		if (sum < tmp) {
			sum++;
		}
	}

	return sum;
}

#define CHECKSUM_TEST_LENGTH 1500

uint8_t testdata[CHECKSUM_TEST_LENGTH];

ZTEST(test_utils_fn, test_ip_checksum)
{
	uint16_t sum_got;
	uint16_t sum_exp;

	/* Simple test dataset */
	for (int i = 0; i < CHECKSUM_TEST_LENGTH; i++) {
		testdata[i] = (uint8_t)i;
	}

	for (int i = 1; i <= CHECKSUM_TEST_LENGTH; i++) {
		sum_got = calc_chksum_ref(i ^ 0x1f13, testdata, i);
		sum_exp = calc_chksum(i ^ 0x1f13, testdata, i);

		zassert_equal(sum_got, sum_exp,
			      "Mismatch between reference and calculated checksum 1\n");
	}

	/* Create a different patten in the data */
	for (int i = 0; i < CHECKSUM_TEST_LENGTH; i++) {
		testdata[i] = (uint8_t)(i + 13) * 17;
	}

	for (int i = 1; i <= CHECKSUM_TEST_LENGTH; i++) {
		sum_got = calc_chksum_ref(i ^ 0x1f13, testdata + (CHECKSUM_TEST_LENGTH - i), i);
		sum_exp = calc_chksum(i ^ 0x1f13, testdata + (CHECKSUM_TEST_LENGTH - i), i);

		zassert_equal(sum_got, sum_exp,
			      "Mismatch between reference and calculated checksum 2\n");
	}

	/* Work across all possible combination so offset and length */
	for (int offset = 0; offset < 7; offset++) {
		for (int length = 1; length < 32; length++) {
			sum_got = calc_chksum_ref(offset ^ 0x8e72, testdata + offset, length);
			sum_exp = calc_chksum(offset ^ 0x8e72, testdata + offset, length);

			zassert_equal(sum_got, sum_exp,
				      "Mismatch between reference and calculated checksum 3\n");
		}
	}
}

/* Verify that the net_pkt pointer to the received link layer address
 * is correct.
 */
ZTEST(test_utils_fn, test_linkaddr_handling)
{
	/* A simple Ethernet frame with IPv4 and UDP headers */
	static const uint8_t udp[] = {
		0x18, 0xfd, 0x74, 0x09, 0xcb, 0x62, 0xac, 0x91,  /* 0000 */
		0xa1, 0x8f, 0x9d, 0xf8, 0x08, 0x00, 0x45, 0x00,  /* 0008 */
		0x00, 0x4c, 0x48, 0x8e, 0x00, 0x00, 0x40, 0x11,  /* 0010 */
		0x57, 0x34, 0xc0, 0xa8, 0x58, 0x29, 0xc1, 0xe5,  /* 0018 */
		0x00, 0x28, 0xba, 0xf0, 0x00, 0x35, 0x00, 0x38,  /* 0020 */
		0xdb, 0x28,
	};

	/* Create net_pkt from the above data, then check the link layer
	 * addresses are properly set even if we pull the data like how
	 * the network stack would do in ethernet.c
	 */
	const uint8_t *dst = &udp[0];
	const uint8_t *src = &udp[NET_ETH_ADDR_LEN];
	uint8_t hdr_len = sizeof(struct net_eth_hdr);
	struct net_linkaddr *lladdr;
	struct net_eth_hdr *hdr;
	struct net_pkt *pkt, *pkt2;
	int ret;

	pkt = net_pkt_rx_alloc_with_buffer(net_if_get_default(),
					   sizeof(udp), AF_UNSPEC,
					   0, K_NO_WAIT);
	zassert_not_null(pkt, "Cannot allocate pkt");

	ret = net_pkt_write(pkt, udp, sizeof(udp));
	zassert_equal(ret, 0, "Cannot write data to pkt");

	hdr = NET_ETH_HDR(pkt);

	/* Set the pointers to ll src and dst addresses */
	lladdr = net_pkt_lladdr_src(pkt);
	memcpy(lladdr->addr, hdr->src.addr, sizeof(struct net_eth_addr));
	lladdr->len = sizeof(struct net_eth_addr);
	lladdr->type = NET_LINK_ETHERNET;

	lladdr = net_pkt_lladdr_dst(pkt);
	memcpy(lladdr->addr, hdr->dst.addr, sizeof(struct net_eth_addr));
	lladdr->len = sizeof(struct net_eth_addr);
	lladdr->type = NET_LINK_ETHERNET;

	zassert_mem_equal(net_pkt_lladdr_src(pkt)->addr,
			  src, NET_ETH_ADDR_LEN,
			  "Source address mismatch");
	zassert_mem_equal(net_pkt_lladdr_dst(pkt)->addr,
			  dst, NET_ETH_ADDR_LEN,
			  "Destination address mismatch");

	pkt2 = net_pkt_clone(pkt, K_NO_WAIT);
	zassert_not_null(pkt2, "Cannot clone pkt");

	/* Make sure we still point to the correct addresses after cloning */
	zassert_mem_equal(net_pkt_lladdr_src(pkt2)->addr,
			  src, NET_ETH_ADDR_LEN,
			  "Source address mismatch");
	zassert_mem_equal(net_pkt_lladdr_dst(pkt2)->addr,
			  dst, NET_ETH_ADDR_LEN,
			  "Destination address mismatch");

	net_pkt_unref(pkt2);

	/* Get rid of the Ethernet header. */
	net_buf_pull(pkt->frags, hdr_len);

	/* Make sure we still point to the correct addresses after pulling
	 * the Ethernet header.
	 */
	zassert_mem_equal(net_pkt_lladdr_src(pkt)->addr,
			  src, NET_ETH_ADDR_LEN,
			  "Source address mismatch");
	zassert_mem_equal(net_pkt_lladdr_dst(pkt)->addr,
			  dst, NET_ETH_ADDR_LEN,
			  "Destination address mismatch");

	/* Clone the packet and check that the link layer addresses are
	 * still correct.
	 */

	pkt2 = net_pkt_clone(pkt, K_NO_WAIT);
	zassert_not_null(pkt2, "Cannot clone pkt");

	zassert_not_equal(net_pkt_lladdr_src(pkt2)->addr,
			  net_pkt_lladdr_src(pkt)->addr,
			  "Source address should not be the same");

	zassert_mem_equal(net_pkt_lladdr_src(pkt2)->addr,
			  src, NET_ETH_ADDR_LEN,
			  "Source address mismatch");
	zassert_mem_equal(net_pkt_lladdr_dst(pkt2)->addr,
			  dst, NET_ETH_ADDR_LEN,
			  "Destination address mismatch");

	net_pkt_unref(pkt);
	net_pkt_unref(pkt2);
}

ZTEST_SUITE(test_utils_fn, NULL, NULL, NULL, NULL, NULL);
