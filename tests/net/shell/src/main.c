/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_UDP_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/udp.h>
#include <zephyr/random/random.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

#include "ipv4.h"
#include "ipv6.h"

#include <zephyr/ztest.h>

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#include "udp_internal.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define NET_LOG_ENABLED 1
#endif
#include "net_private.h"

static bool test_failed;
static struct k_sem recv_lock;

struct net_udp_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_udp_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint8_t *net_udp_get_mac(const struct device *dev)
{
	struct net_udp_context *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = sys_rand8_get();
	}

	return context->mac_addr;
}

static void net_udp_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_udp_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static int send_status = -EINVAL;

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	DBG("Data was sent successfully\n");

	send_status = 0;

	return 0;
}

struct net_udp_context net_udp_context_data;

static DEVICE_API(dummy, net_udp_if_api) = {
	.iface_api.init = net_udp_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_udp_test, "net_udp_test",
		net_udp_dev_init, NULL,
		&net_udp_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_udp_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

static void *test_setup(void)
{
	struct net_if *iface;
	struct net_if_addr *ifaddr;

	struct net_sockaddr_in6 any_addr6;
	const struct net_in6_addr in6addr_anyaddr = NET_IN6ADDR_ANY_INIT;

	struct net_sockaddr_in6 my_addr6;
	struct net_in6_addr in6addr_my = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					   0, 0, 0, 0, 0, 0, 0, 0x1 } } };

	struct net_sockaddr_in6 peer_addr6;
	struct net_in6_addr in6addr_peer = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0x4e, 0x11, 0, 0, 0x2 } } };

	struct net_sockaddr_in any_addr4;
	const struct net_in_addr in4addr_any = { { { 0 } } };

	struct net_sockaddr_in my_addr4;
	struct net_in_addr in4addr_my = { { { 192, 0, 2, 1 } } };

	struct net_sockaddr_in peer_addr4;
	struct net_in_addr in4addr_peer = { { { 192, 0, 2, 9 } } };

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	test_failed = false;

	net_ipaddr_copy(&any_addr6.sin6_addr, &in6addr_anyaddr);
	any_addr6.sin6_family = NET_AF_INET6;

	net_ipaddr_copy(&my_addr6.sin6_addr, &in6addr_my);
	my_addr6.sin6_family = NET_AF_INET6;

	net_ipaddr_copy(&peer_addr6.sin6_addr, &in6addr_peer);
	peer_addr6.sin6_family = NET_AF_INET6;

	net_ipaddr_copy(&any_addr4.sin_addr, &in4addr_any);
	any_addr4.sin_family = NET_AF_INET;

	net_ipaddr_copy(&my_addr4.sin_addr, &in4addr_my);
	my_addr4.sin_family = NET_AF_INET;

	net_ipaddr_copy(&peer_addr4.sin_addr, &in4addr_peer);
	peer_addr4.sin_family = NET_AF_INET;

	k_sem_init(&recv_lock, 0, UINT_MAX);

	ifaddr = net_if_ipv6_addr_add(iface, &in6addr_my, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv6_addr(&in6addr_my), iface);
		zassert_true(0, "exiting");
	}

	ifaddr = net_if_ipv4_addr_add(iface, &in4addr_my, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		printk("Cannot add %s to interface %p\n",
		       net_sprint_ipv4_addr(&in4addr_my), iface);
		zassert_true(0, "exiting");
	}

	/* The output of a command is captured only once the shell runs. */
	zassert_true(WAIT_FOR(shell_ready(shell_backend_dummy_get_ptr()), 1000000, k_msleep(10)),
		     "Dummy shell backend not ready");

	return NULL;
}

ZTEST(net_shell_test_suite, test_net_shell)
{
	int ret;

	/* Test that command exists */
	ret = shell_execute_cmd(NULL, "net iface");
	zassert_equal(ret, 0, "");

	/* There is no foobar command */
	ret = shell_execute_cmd(NULL, "net foobar");
	zassert_equal(ret, -ENOEXEC, "");

	/* The cache flushing commands are registered even when the
	 * corresponding cache is not built in.
	 */
	ret = shell_execute_cmd(NULL, "net nbr flush");
	zexpect_equal(ret, 0, "");

	ret = shell_execute_cmd(NULL, "net arp flush");
	zexpect_equal(ret, 0, "");
}

/* Return value of a command called with "--help": the shell prints the help
 * itself, unless help option parsing is off and the option reaches the handler.
 */
#define HELP_RET (IS_ENABLED(CONFIG_SHELL_HELP_OPT_PARSE) ? SHELL_CMD_HELP_PRINTED : -ENOEXEC)

static int exec_cmd(const char *cmd)
{
	shell_backend_dummy_clear_output(shell_backend_dummy_get_ptr());

	return shell_execute_cmd(NULL, cmd);
}

static bool output_has(const char *str)
{
	size_t len;
	const char *out = shell_backend_dummy_get_output(shell_backend_dummy_get_ptr(), &len);

	return strstr(out, str) != NULL;
}

ZTEST(net_shell_test_suite, test_net_shell_pkt)
{
	int ret;

	ret = exec_cmd("net pkt --help");
	zexpect_equal(ret, HELP_RET, "net pkt --help (%d)", ret);
	zexpect_true(output_has(IS_ENABLED(CONFIG_SHELL_HELP_OPT_PARSE) ? "Usage: pkt"
									: "Invalid ptr value"),
		     "");

	ret = exec_cmd("net pkt");
	zexpect_equal(ret, -EINVAL, "net pkt (%d)", ret);

	ret = exec_cmd("net pkt xyz");
	zexpect_equal(ret, -ENOEXEC, "net pkt xyz (%d)", ret);
	zexpect_true(output_has("Invalid ptr value"), "");

	ret = exec_cmd("net pkt 0x10");
	zexpect_equal(ret, -ENOEXEC, "net pkt 0x10 (%d)", ret);
	zexpect_true(output_has("not recognized as net_pkt"), "");

	/* A real packet is found, with or without the "0x" prefix, whatever
	 * the width of a pointer.
	 */
	static const char *const formats[] = {"net pkt 0x%lx", "net pkt %lx"};
	struct net_pkt *pkt = net_pkt_alloc(K_NO_WAIT);
	char cmd[32];

	zassert_not_null(pkt, "Cannot allocate a net_pkt");

	ARRAY_FOR_EACH(formats, i) {
		snprintk(cmd, sizeof(cmd), formats[i], (unsigned long)(uintptr_t)pkt);
		ret = exec_cmd(cmd);
		zexpect_equal(ret, 0, "%s (%d)", cmd, ret);
		zexpect_true(output_has("buffer chain"), "%s", cmd);
	}

	net_pkt_unref(pkt);
}

static void expect_ping_error(const char *cmd, const char *msg)
{
	int ret = exec_cmd(cmd);

	zexpect_equal(ret, -ENOEXEC, "%s (%d)", cmd, ret);
	zexpect_true(output_has(msg), "%s: no \"%s\"", cmd, msg);
}

ZTEST(net_shell_test_suite, test_net_shell_ping)
{
	int ret;

	/* "ping" is a host argument, not a subcommand. */
	expect_ping_error("net ping ping", "Invalid IP address");
	expect_ping_error("net ping -c 3 ping", "Invalid IP address");
	expect_ping_error("net ping 999.1.1.1", "Invalid IP address");

	ret = exec_cmd("net ping --help");
	zexpect_equal(ret, HELP_RET, "net ping --help (%d)", ret);
	zexpect_true(output_has(IS_ENABLED(CONFIG_SHELL_HELP_OPT_PARSE) ? "Usage: ping"
									: "Unrecognized argument"),
		     "");

	ret = exec_cmd("net ping");
	zexpect_equal(ret, -EINVAL, "net ping (%d)", ret);

	expect_ping_error("net ping -q", "Target host missing");
	expect_ping_error("net ping -x 192.0.2.9", "Unrecognized argument");
	expect_ping_error("net ping -c", "Parse error");
	expect_ping_error("net ping -I 99 192.0.2.9", "Parse error");
}

ZTEST(net_shell_test_suite, test_net_shell_http_get_url_too_long)
{
	static char cmd[CONFIG_SHELL_CMD_BUFF_SIZE];
	static const char prefix[] = "net http get http://a/p";
	int ret;

	if (!IS_ENABLED(CONFIG_HTTP_CLIENT)) {
		ztest_test_skip();
	}

	/* A path that does not fit into the URL buffer must be rejected instead
	 * of overflowing it. The fragment is not part of the request target.
	 */
	memcpy(cmd, prefix, sizeof(prefix) - 1);
	memset(cmd + sizeof(prefix) - 1, 'x', sizeof(cmd) - sizeof(prefix));
	cmd[sizeof(cmd) - 1] = '\0';

	ret = shell_execute_cmd(NULL, cmd);
	zassert_equal(ret, -ENOBUFS, "Long path accepted (%d)", ret);

	/* A fragment of the same length is not part of the request target. */
	cmd[sizeof(prefix) - 1] = '#';

	ret = shell_execute_cmd(NULL, cmd);
	zassert_not_equal(ret, -ENOBUFS, "Long fragment rejected (%d)", ret);
}

ZTEST(net_shell_test_suite, test_net_shell_http_get_query_too_long)
{
	static char cmd[CONFIG_SHELL_CMD_BUFF_SIZE];
	static const char prefix[] = "net http get http://a?";
	int ret;

	if (!IS_ENABLED(CONFIG_HTTP_CLIENT) || IS_ENABLED(CONFIG_SHELL_WILDCARD)) {
		ztest_test_skip();
	}

	/* Without a path the query is still part of the request target, so a
	 * query that does not fit into the URL buffer must be rejected.
	 */
	memcpy(cmd, prefix, sizeof(prefix) - 1);
	memset(cmd + sizeof(prefix) - 1, 'x', sizeof(cmd) - sizeof(prefix));
	cmd[sizeof(cmd) - 1] = '\0';

	ret = shell_execute_cmd(NULL, cmd);
	zexpect_equal(ret, -ENOBUFS, "Long query accepted (%d)", ret);
}

ZTEST_SUITE(net_shell_test_suite, NULL, test_setup, NULL, NULL, NULL);
