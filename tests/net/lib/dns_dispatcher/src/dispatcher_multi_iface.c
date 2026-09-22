/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* DNS socket dispatcher pairing on a multi-interface host (no mDNS):
 * resolver and responder share UDP/5353 per interface. Cross-interface
 * registration must not pair (eth-sink class bug without can_pair()).
 */

#include "dns_dispatcher_test.h"

#if DNS_DISPATCHER_MULTI_IFACE_TEST

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/ztest.h>

#define TEST_DNS_PORT 5353

extern void dns_dispatcher_svc_handler(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(test_dns_svc, dns_dispatcher_svc_handler, 4);

static struct net_if *iface1;
static struct net_if *iface2;

/* Mock dispatcher registrations (resolver/responder on UDP/5353).
 * resp* are scoped to iface1/iface2; resv2 is a scoped resolver reused
 * across same- and cross-iface cases; resv_any is an unscoped resolver
 * (ifindex 0) for test_dispatcher_unscoped_pairs_multi_iface().
 */
static struct dns_socket_dispatcher resp1;
static struct dns_socket_dispatcher resp2;
static struct dns_socket_dispatcher resv2;
static struct dns_socket_dispatcher resv_any;
static struct zsock_pollfd resp1_fd;
static struct zsock_pollfd resp2_fd;
static struct zsock_pollfd resv2_fd;
static struct zsock_pollfd resv_any_fd;

struct multi_if_test {
	uint8_t idx;
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
};

static struct multi_if_test if1_data;
static struct multi_if_test if2_data;

static int multi_if_dummy_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static uint8_t *multi_if_mac(struct multi_if_test *data, uint8_t idx)
{
	if (data->mac_addr[2] == 0x00) {
		data->mac_addr[0] = 0x00;
		data->mac_addr[1] = 0x00;
		data->mac_addr[2] = 0x5E;
		data->mac_addr[3] = 0x00;
		data->mac_addr[4] = 0x53;
		data->mac_addr[5] = idx;
	}

	data->idx = idx;

	return data->mac_addr;
}

static void if1_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, multi_if_mac(&if1_data, 1),
			     sizeof(struct net_eth_addr), NET_LINK_ETHERNET);
}

static void if2_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, multi_if_mac(&if2_data, 2),
			     sizeof(struct net_eth_addr), NET_LINK_ETHERNET);
}

#define _MULTI_L2 DUMMY_L2
#define _MULTI_L2_CTX NET_L2_GET_CTX_TYPE(DUMMY_L2)

static struct dummy_api if1_api = {
	.iface_api.init = if1_init,
	.send = multi_if_dummy_send,
};

static struct dummy_api if2_api = {
	.iface_api.init = if2_init,
	.send = multi_if_dummy_send,
};

NET_DEVICE_INIT_INSTANCE(disp_if1_test, "iface1", iface1, NULL, NULL,
			 &if1_data, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &if1_api, _MULTI_L2, _MULTI_L2_CTX, 127);

NET_DEVICE_INIT_INSTANCE(disp_if2_test, "iface2", iface2, NULL, NULL,
			 &if2_data, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &if2_api, _MULTI_L2, _MULTI_L2_CTX, 127);

static int mock_dispatcher_cb(struct dns_socket_dispatcher *ctx, int sock,
			      struct net_sockaddr *addr, size_t addrlen,
			      struct net_buf *buf, size_t len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(sock);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	return 0;
}

static int create_socket(struct net_if *iface)
{
	struct net_ifreq ifreq = { 0 };
	const struct device *dev = net_if_get_device(iface);
	int reuse = 1;
	int sock;

	sock = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_not_null(dev, "iface device");
	zassert_true(sock >= 0, "socket failed");

	if (IS_ENABLED(CONFIG_NET_CONTEXT_REUSEPORT)) {
		(void)zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEPORT,
				       &reuse, sizeof(reuse));
	}

	strncpy(ifreq.ifr_name, dev->name, sizeof(ifreq.ifr_name) - 1);
	(void)zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE,
			       &ifreq, sizeof(ifreq));

	return sock;
}

static void setup_dispatcher(struct dns_socket_dispatcher *disp,
			     enum dns_socket_type type, struct net_if *iface,
			     int sock, struct zsock_pollfd *pfd)
{
	struct net_sockaddr_in6 *local = net_sin6(&disp->local_addr);

	memset(disp, 0, sizeof(*disp));
	disp->type = type;
	disp->ifindex = net_if_get_by_iface(iface);
	disp->sock = sock;
	disp->cb = mock_dispatcher_cb;
	disp->fds = pfd;
	disp->fds_len = 1;
	disp->svc = &test_dns_svc;
	pfd->fd = sock;
	pfd->events = ZSOCK_POLLIN;

	local->sin6_family = NET_AF_INET6;
	local->sin6_port = net_htons(TEST_DNS_PORT);
}

static void close_paired(struct dns_socket_dispatcher *primary,
			 struct dns_socket_dispatcher *paired)
{
	if (paired->sock >= 0) {
		(void)zsock_close(paired->sock);
	}

	paired->sock = -1;
	paired->pair = NULL;
	if (paired->fds != NULL && paired->fds_len > 0) {
		paired->fds[0].fd = -1;
	}

	if (primary->pair == paired) {
		primary->pair = NULL;
	}
}

static void teardown_primary(struct dns_socket_dispatcher *primary,
			     struct zsock_pollfd *pfd)
{
	int fd;

	if (primary->pair != NULL) {
		close_paired(primary, primary->pair);
	}

	fd = primary->sock;

	if (fd >= 0) {
		(void)dns_dispatcher_unregister(primary);
		(void)zsock_close(fd);
	}

	memset(primary, 0, sizeof(*primary));
	primary->sock = -1;
	pfd->fd = -1;
}

static void *multi_iface_setup(void)
{
	iface1 = net_if_get_by_index(1);
	iface2 = net_if_get_by_index(2);

	zassert_not_null(iface1, "iface1 missing");
	zassert_not_null(iface2, "iface2 missing");

	net_if_up(iface1);
	net_if_up(iface2);

	resp1.sock = -1;
	resp2.sock = -1;
	resv2.sock = -1;
	resv_any.sock = -1;
	resp1_fd.fd = -1;
	resp2_fd.fd = -1;
	resv2_fd.fd = -1;
	resv_any_fd.fd = -1;

	return NULL;
}

ZTEST(dns_dispatch_pair, test_dispatcher_iface_pairing)
{
	int ret;

	/* Same interface: resolver pairs with responder. */
	setup_dispatcher(&resp1, DNS_SOCKET_RESPONDER, iface1,
			 create_socket(iface1), &resp1_fd);
	ret = dns_dispatcher_register(&resp1);
	zassert_ok(ret, "iface1 responder register failed (%d)", ret);
	zassert_is_null(resp1.pair, "responder not paired yet");

	setup_dispatcher(&resv2, DNS_SOCKET_RESOLVER, iface1,
			 create_socket(iface1), &resv2_fd);
	ret = dns_dispatcher_register(&resv2);
	zassert_ok(ret, "iface1 resolver register failed (%d)", ret);
	zassert_equal(resp1.pair, &resv2, "same-iface resolver must pair");

	teardown_primary(&resp1, &resp1_fd);
	memset(&resv2, 0, sizeof(resv2));
	resv2.sock = -1;
	resv2_fd.fd = -1;

	/* Cross interface: must not pair (needs dns_dispatcher_can_pair()). */
	setup_dispatcher(&resp1, DNS_SOCKET_RESPONDER, iface1,
			 create_socket(iface1), &resp1_fd);
	ret = dns_dispatcher_register(&resp1);
	zassert_ok(ret, "iface1 responder re-register failed (%d)", ret);

	setup_dispatcher(&resv2, DNS_SOCKET_RESOLVER, iface2,
			 create_socket(iface2), &resv2_fd);
	ret = dns_dispatcher_register(&resv2);
	zassert_ok(ret, "iface2 resolver register failed (%d)", ret);
	zassert_is_null(resp1.pair, "cross-iface must not pair with iface1 responder");
	zassert_is_null(resv2.pair, "cross-iface resolver must stay unpaired");

	setup_dispatcher(&resp2, DNS_SOCKET_RESPONDER, iface2,
			 create_socket(iface2), &resp2_fd);
	ret = dns_dispatcher_register(&resp2);
	zassert_ok(ret, "iface2 responder register failed (%d)", ret);
	zassert_equal(resv2.pair, &resp2, "iface2 responder pairs locally");

	teardown_primary(&resp2, &resp2_fd);
	teardown_primary(&resv2, &resv2_fd);
	teardown_primary(&resp1, &resp1_fd);
}

/* Multi-interface host: an unscoped (ifindex 0) resolver still pairs with a
 * scoped responder. Which interface's responder it gets is a known
 * limitation, but refusing would leave the resolver unable to bind the
 * shared port (no SO_REUSEPORT on the real sockets) and an unpaired
 * responder drops responses, so registration must keep working.
 */
ZTEST(dns_dispatch_pair, test_dispatcher_unscoped_pairs_multi_iface)
{
	int ret;

	setup_dispatcher(&resp1, DNS_SOCKET_RESPONDER, iface1,
			 create_socket(iface1), &resp1_fd);
	ret = dns_dispatcher_register(&resp1);
	zassert_ok(ret, "iface1 responder register failed (%d)", ret);

	setup_dispatcher(&resv_any, DNS_SOCKET_RESOLVER, iface1,
			 create_socket(iface1), &resv_any_fd);
	resv_any.ifindex = 0;
	ret = dns_dispatcher_register(&resv_any);
	zassert_ok(ret, "unscoped resolver register failed (%d)", ret);
	zassert_equal(resp1.pair, &resv_any,
		      "unscoped resolver must pair on multi-iface too");
	zassert_is_null(resv_any.pair, "later registrant's pair stays unset");

	teardown_primary(&resp1, &resp1_fd);
	memset(&resv_any, 0, sizeof(resv_any));
	resv_any.sock = -1;
	resv_any_fd.fd = -1;
}

ZTEST_SUITE(dns_dispatch_pair, NULL, multi_iface_setup, NULL, NULL, NULL);

#endif /* DNS_DISPATCHER_MULTI_IFACE_TEST */
