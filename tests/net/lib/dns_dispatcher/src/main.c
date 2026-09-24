/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_DNS_RESOLVER_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>

#include <zephyr/ztest.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"
#include "ipv6.h"
#include "udp_internal.h"
#include "dns_dispatcher_test.h"

#if defined(CONFIG_DNS_RESOLVER_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

extern void dns_dispatcher_svc_handler(struct net_socket_service_event *pev);

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(test_pair_svc, dns_dispatcher_svc_handler, 1);

static int test_dispatch_cb(struct dns_socket_dispatcher *ctx, int sock,
			    struct net_sockaddr *addr, size_t addrlen,
			    struct net_buf *buf, size_t data_len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(sock);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);
	ARG_UNUSED(buf);
	ARG_UNUSED(data_len);

	return 0;
}

#define NAME4 "4.zephyr.test"
#define NAME6 "6.zephyr.test"
#define NAME_IPV4 "192.0.2.1"
#define NAME_IPV6 "2001:db8::1"

#define DNS_NAME_IPV4 "192.0.2.4"
#define DNS2_NAME_IPV4 "192.0.2.5"
#define DNS_NAME_IPV6 "2001:db8::4"

#define DNS_TIMEOUT 500 /* ms */

#if defined(CONFIG_NET_IPV6) && !DNS_DISPATCHER_MULTI_IFACE_TEST
/* Interface 1 addresses */
static struct net_in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0x1 } } };
#endif

#if defined(CONFIG_NET_IPV4) && !DNS_DISPATCHER_MULTI_IFACE_TEST
/* Interface 1 addresses */
static struct net_in_addr my_addr2 = { { { 192, 0, 2, 1 } } };
#endif

#if !DNS_DISPATCHER_MULTI_IFACE_TEST

static struct net_if *iface1;

#endif

/* this must be higher that the DNS_TIMEOUT */
#define WAIT_TIME K_MSEC((DNS_TIMEOUT + 300) * 3)

struct net_if_test {
	uint8_t idx;
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
};

#if !DNS_DISPATCHER_MULTI_IFACE_TEST

static uint8_t *net_iface_get_mac(const struct device *dev)
{
	struct net_if_test *data = dev->data;

	if (data->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		data->mac_addr[0] = 0x00;
		data->mac_addr[1] = 0x00;
		data->mac_addr[2] = 0x5E;
		data->mac_addr[3] = 0x00;
		data->mac_addr[4] = 0x53;
		data->mac_addr[5] = sys_rand8_get();
	}

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	return 0;
}

struct net_if_test net_iface1_data;

static struct dummy_api net_iface_api = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 NULL,
			 NULL,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

#endif /* !DNS_DISPATCHER_MULTI_IFACE_TEST */

static void *test_init(void)
{
#if DNS_DISPATCHER_MULTI_IFACE_TEST
	return NULL;
#else
	struct net_if_addr *ifaddr;

	iface1 = net_if_get_by_index(0);
	zassert_is_null(iface1, "iface1");

	iface1 = net_if_get_by_index(1);

	((struct net_if_test *) net_if_get_device(iface1)->data)->idx =
		net_if_get_by_iface(iface1);

#if defined(CONFIG_NET_IPV6)
	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");

		return NULL;
	}

	/* For testing purposes we need to set the addresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;
#endif

#if defined(CONFIG_NET_IPV4)
	ifaddr = net_if_ipv4_addr_add(iface1, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv4 address %s\n",
		       net_sprint_ipv4_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2");

		return NULL;
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;
#endif

	net_if_up(iface1);

	return NULL;
#endif
}

ZTEST(dns_dispatcher, test_dns_dispatcher)
{
	struct dns_resolve_context *ctx;
	int ret, sock1, sock2 = -1;

#if DNS_DISPATCHER_MULTI_IFACE_TEST
	ztest_test_skip();
#endif

#if IS_ENABLED(CONFIG_MDNS_RESOLVER)
	ztest_test_skip();
#endif

	ctx = dns_resolve_get_default();

	dns_resolve_close(ctx);

	ret = dns_resolve_init_default(ctx);
	zassert_equal(ret, 0, "Cannot initialize DNS resolver (%d)", ret);

	sock1 = ctx->servers[0].sock;

	for (int i = 0; i < ctx->servers[0].dispatcher.fds_len; i++) {
		if (ctx->servers[0].dispatcher.fds[i].fd == sock1) {
			sock2 = i;
			break;
		}
	}

	zassert_not_equal(sock2, -1, "Cannot find socket");

	k_sleep(K_MSEC(10));

	dns_resolve_close(ctx);

	zassert_equal(ctx->servers[0].dispatcher.fds[sock2].fd, -1, "Socket not closed");
	zassert_equal(ctx->servers[0].dispatcher.sock, -1, "Dispatcher still registered");
}

/* Register a responder and a resolver on the same family/port so the
 * dispatcher pairs them (responder->pair points at the resolver). Closing the
 * resolver must clear that back-reference, otherwise the surviving responder
 * would later delegate traffic to an unregistered context.
 */
ZTEST(dns_dispatcher, test_dispatcher_pair_cleanup)
{
	static struct dns_socket_dispatcher responder;
	static struct dns_socket_dispatcher resolver;
	static struct zsock_pollfd responder_fds[1];
	static struct zsock_pollfd resolver_fds[1];
	struct net_sockaddr_in local = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(65123),
	};
	int responder_sock, resolver_sock;

	responder_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(responder_sock >= 0, "Cannot create responder socket");

	resolver_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(resolver_sock >= 0, "Cannot create resolver socket");

	responder_fds[0].fd = responder_sock;
	resolver_fds[0].fd = resolver_sock;

	responder.type = DNS_SOCKET_RESPONDER;
	responder.cb = test_dispatch_cb;
	responder.fds = responder_fds;
	responder.fds_len = 1;
	responder.sock = responder_sock;
	responder.svc = &test_pair_svc;
	memcpy(&responder.local_addr_storage, &local, sizeof(local));

	resolver.type = DNS_SOCKET_RESOLVER;
	resolver.cb = test_dispatch_cb;
	resolver.fds = resolver_fds;
	resolver.fds_len = 1;
	resolver.sock = resolver_sock;
	resolver.svc = &test_pair_svc;
	memcpy(&resolver.local_addr_storage, &local, sizeof(local));

	zassert_ok(dns_dispatcher_register(&responder), "Cannot register responder");
	zassert_ok(dns_dispatcher_register(&resolver), "Cannot register resolver");

	zassert_equal(responder.pair, &resolver, "Dispatchers were not paired");

	zassert_ok(dns_dispatcher_unregister(&resolver), "Cannot unregister resolver");
	zassert_is_null(responder.pair, "Pair back-reference was not cleared");

	zassert_ok(dns_dispatcher_unregister(&responder), "Cannot unregister responder");

	(void)zsock_close(responder_sock);
	(void)zsock_close(resolver_sock);
}

/* A poll event may still be in flight for a socket whose dispatch slot was
 * cleared concurrently (e.g. the server was just closed). Emulate that by
 * handing the handler a dispatch table whose slot is NULL and verify the event
 * is dropped instead of dereferencing a NULL dispatcher.
 */
ZTEST(dns_dispatcher, test_dispatcher_null_slot_dropped)
{
	/* dispatch_table entries start with the dispatcher pointer, so a plain
	 * NULL pointer slot is layout-compatible for index 0.
	 */
	struct dns_socket_dispatcher *table[1] = { NULL };
	struct net_socket_service_event pev = {
		.event = {
			.fd = 0,
			.revents = ZSOCK_POLLIN,
		},
		.user_data = table,
	};

	/* Prior to the NULL guard this dereferenced a NULL dispatcher and
	 * crashed; reaching the next statement proves the event was dropped.
	 */
	dns_dispatcher_svc_handler(&pev);
}

ZTEST(dns_dispatcher, test_dns_dispatcher_ephemeral_ports)
{
	static const char * const servers[] = { DNS_NAME_IPV4, DNS2_NAME_IPV4, NULL };
	struct dns_resolve_context *ctx;
	uint16_t port0, port1;
	int ret;

	ctx = dns_resolve_get_default();

	dns_resolve_close(ctx);

	/* Two DNS servers of the same address family, no explicit interface
	 * and the default local port 0. Each resolver socket bind() picks a
	 * distinct OS-assigned ephemeral port, but the dispatcher matches and
	 * deduplicates registrations by the stored local port. Unless the
	 * dispatcher reads back the bound port with getsockname(), both
	 * sockets look like the same port-0 resolver socket, and the second
	 * registration is rejected as a duplicate (swallowed by resolve.c as
	 * -EALREADY), leaving its socket undispatched.
	 */
	ret = dns_resolve_init(ctx, (const char **)servers, NULL);
	zassert_equal(ret, 0, "Cannot initialize DNS resolver (%d)", ret);

	zassert_true(ctx->servers[0].sock >= 0, "First server socket not open");
	zassert_true(ctx->servers[1].sock >= 0, "Second server socket not open");

	port0 = net_sin(net_sad(&ctx->servers[0].dispatcher.local_addr_storage))->sin_port;
	port1 = net_sin(net_sad(&ctx->servers[1].dispatcher.local_addr_storage))->sin_port;

	/* Both registrations must have captured their real ephemeral port. */
	zassert_not_equal(port0, 0, "First dispatcher port not resolved");
	zassert_not_equal(port1, 0, "Second dispatcher port not resolved");

	/* Distinct sockets must end up with distinct ports so that neither is
	 * treated as a duplicate of the other.
	 */
	zassert_not_equal(port0, port1,
			  "Both dispatchers share the same local port");

	dns_resolve_close(ctx);
}

#if !DNS_DISPATCHER_MULTI_IFACE_TEST

/* Single-interface host: an unscoped (ifindex 0) resolver must pair with a
 * scoped responder on that interface.
 */

#define TEST_DNS_PAIR_PORT 5353

NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(pair_svc, dns_dispatcher_svc_handler, 2);

static struct dns_socket_dispatcher pair_resp;
static struct dns_socket_dispatcher pair_resv;
static struct zsock_pollfd pair_resp_fd;
static struct zsock_pollfd pair_resv_fd;

static int pair_mock_cb(struct dns_socket_dispatcher *ctx, int sock,
			struct net_sockaddr *addr, size_t addrlen,
			struct net_buf *buf, size_t len)
{
	ARG_UNUSED(ctx); ARG_UNUSED(sock); ARG_UNUSED(addr);
	ARG_UNUSED(addrlen); ARG_UNUSED(buf); ARG_UNUSED(len);
	return 0;
}

static int pair_create_socket(void)
{
	struct net_ifreq ifreq = { 0 };
	const struct device *dev = net_if_get_device(iface1);
	int reuse = 1;
	int sock;

	sock = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
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

static void pair_setup(struct dns_socket_dispatcher *disp, enum dns_socket_type type,
		       int ifindex, int sock, struct zsock_pollfd *pfd)
{
	struct net_sockaddr_in6 *local = net_sin6(&disp->local_addr);

	memset(disp, 0, sizeof(*disp));
	disp->type = type;
	disp->ifindex = ifindex;
	disp->sock = sock;
	disp->cb = pair_mock_cb;
	disp->fds = pfd;
	disp->fds_len = 1;
	disp->svc = &pair_svc;
	pfd->fd = sock;
	pfd->events = ZSOCK_POLLIN;

	local->sin6_family = NET_AF_INET6;
	local->sin6_port = net_htons(TEST_DNS_PAIR_PORT);
}

static void pair_teardown(struct dns_socket_dispatcher *disp, struct zsock_pollfd *pfd)
{
	int fd = disp->sock;

	if (disp->pair != NULL) {
		struct dns_socket_dispatcher *p = disp->pair;

		if (p->sock >= 0) {
			(void)zsock_close(p->sock);
		}
		p->sock = -1;
		p->pair = NULL;
		if (p->fds != NULL && p->fds_len > 0) {
			p->fds[0].fd = -1;
		}
		disp->pair = NULL;
	}

	if (fd >= 0) {
		(void)dns_dispatcher_unregister(disp);
		(void)zsock_close(fd);
	}
	memset(disp, 0, sizeof(*disp));
	disp->sock = -1;
	pfd->fd = -1;
}

ZTEST(dns_dispatcher, test_dispatcher_unscoped_pairs_single_iface)
{
	int iface_idx = net_if_get_by_iface(iface1);
	int ret;

	pair_setup(&pair_resp, DNS_SOCKET_RESPONDER, iface_idx,
		   pair_create_socket(), &pair_resp_fd);
	ret = dns_dispatcher_register(&pair_resp);
	zassert_ok(ret, "scoped responder register failed (%d)", ret);

	pair_setup(&pair_resv, DNS_SOCKET_RESOLVER, 0,
		   pair_create_socket(), &pair_resv_fd);
	ret = dns_dispatcher_register(&pair_resv);
	zassert_ok(ret, "unscoped resolver register failed (%d)", ret);
	zassert_equal(pair_resp.pair, &pair_resv,
		      "unscoped resolver must pair with scoped responder on single-iface host");

	pair_teardown(&pair_resp, &pair_resp_fd);
	pair_resv_fd.fd = -1;
	memset(&pair_resv, 0, sizeof(pair_resv));
	pair_resv.sock = -1;
}


/* A resolver paired with a responder gets the responder socket's answers
 * through the pair pointer, called with only the responder's lock held.
 * Unregistering the resolver must wait for such a call to finish, or the
 * caller can reuse the context while the callback is still running in it.
 */
static K_SEM_DEFINE(paired_cb_entered, 0, 1);
static K_SEM_DEFINE(paired_cb_release, 0, 1);
static atomic_t paired_cb_done;
static atomic_t paired_unreg_returned;

static K_THREAD_STACK_DEFINE(paired_unreg_stack, 2048);
static struct k_thread paired_unreg_thread;

static int pair_blocking_cb(struct dns_socket_dispatcher *ctx, int sock,
			    struct net_sockaddr *addr, size_t addrlen,
			    struct net_buf *buf, size_t len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(sock);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	k_sem_give(&paired_cb_entered);
	(void)k_sem_take(&paired_cb_release, K_SECONDS(5));
	atomic_set(&paired_cb_done, 1);

	return 0;
}

static void paired_unreg_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)dns_dispatcher_unregister(p1);
	atomic_set(&paired_unreg_returned, 1);
}

/* Deliver a minimal DNS answer (QR set, rcode 0) to the pair port on
 * iface1. The responder socket receives it, and an answer is not the
 * responder's to handle, so the dispatcher delegates it to the pair.
 */
static void inject_dns_answer(void)
{
	static const uint8_t answer[] = { 0x12, 0x34, 0x81, 0x80, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct net_in6_addr src = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 2 } } };
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(iface1, sizeof(answer), NET_AF_INET6, NET_IPPROTO_UDP,
					K_FOREVER);
	zassert_not_null(pkt, "cannot allocate the answer");
	zassert_ok(net_ipv6_create(pkt, &src, &my_addr1), "cannot create the IPv6 header");
	zassert_ok(net_udp_create(pkt, net_htons(TEST_DNS_PAIR_PORT + 1),
				  net_htons(TEST_DNS_PAIR_PORT)),
		   "cannot create the UDP header");
	zassert_ok(net_pkt_write(pkt, answer, sizeof(answer)), "cannot write the answer");
	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, NET_IPPROTO_UDP);
	zassert_ok(net_recv_data(iface1, pkt), "cannot deliver the answer");
}

ZTEST(dns_dispatcher, test_unregister_waits_for_paired_dispatch)
{
	int iface_idx = net_if_get_by_iface(iface1);
	int sock = pair_create_socket();
	int ret;

	/* One socket for both: the paired context has no socket service of
	 * its own, so everything arrives through the responder's.
	 */
	pair_setup(&pair_resp, DNS_SOCKET_RESPONDER, iface_idx, sock, &pair_resp_fd);
	ret = dns_dispatcher_register(&pair_resp);
	zassert_ok(ret, "responder register failed (%d)", ret);

	pair_setup(&pair_resv, DNS_SOCKET_RESOLVER, 0, sock, &pair_resv_fd);
	pair_resv.cb = pair_blocking_cb;
	ret = dns_dispatcher_register(&pair_resv);
	zassert_ok(ret, "resolver register failed (%d)", ret);
	zassert_equal(pair_resp.pair, &pair_resv, "resolver did not pair with the responder");

	atomic_set(&paired_cb_done, 0);
	atomic_set(&paired_unreg_returned, 0);
	k_sem_reset(&paired_cb_entered);
	k_sem_reset(&paired_cb_release);

	inject_dns_answer();
	zassert_ok(k_sem_take(&paired_cb_entered, K_SECONDS(2)),
		   "the answer was not delegated to the resolver");

	/* The resolver's callback is now blocked inside a dispatch on the
	 * responder. Unregistering the resolver must not return before it.
	 */
	k_thread_create(&paired_unreg_thread, paired_unreg_stack,
			K_THREAD_STACK_SIZEOF(paired_unreg_stack), paired_unreg_fn, &pair_resv,
			NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_sleep(K_MSEC(200));
	zassert_equal(atomic_get(&paired_unreg_returned), 0,
		      "unregister returned while the delegated dispatch was still running");

	k_sem_give(&paired_cb_release);
	zassert_ok(k_thread_join(&paired_unreg_thread, K_SECONDS(2)),
		   "unregister did not return after the dispatch finished");
	zassert_equal(atomic_get(&paired_cb_done), 1, "the dispatch did not finish");
	zassert_is_null(pair_resp.pair, "responder still points at the unregistered resolver");

	ret = dns_dispatcher_unregister(&pair_resp);
	zassert_ok(ret, "responder unregister failed (%d)", ret);
	(void)zsock_close(sock);
	pair_resp_fd.fd = -1;
	pair_resv_fd.fd = -1;
	memset(&pair_resp, 0, sizeof(pair_resp));
	memset(&pair_resv, 0, sizeof(pair_resv));
	pair_resp.sock = -1;
	pair_resv.sock = -1;
}

/* A dispatch delegated to the resolver may re-enter the dispatcher API
 * before it returns, for example when the application closes the resolver
 * from its result callback, and then needs the global lock while it still
 * holds the responder's. Unregistering the resolver from another thread in
 * the meantime must not wait for that dispatch with the global lock held,
 * or the two block each other forever.
 */
static K_SEM_DEFINE(reentrant_cb_go, 0, 1);
static struct dns_socket_dispatcher reentrant_third;
static struct zsock_pollfd reentrant_third_fd;
static atomic_t reentrant_cb_ret;

static int pair_reentrant_cb(struct dns_socket_dispatcher *ctx, int sock,
			     struct net_sockaddr *addr, size_t addrlen,
			     struct net_buf *buf, size_t len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(sock);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	k_sem_give(&paired_cb_entered);
	(void)k_sem_take(&reentrant_cb_go, K_SECONDS(5));

	/* Needs the global lock, with the responder's lock held */
	atomic_set(&reentrant_cb_ret, dns_dispatcher_register(&reentrant_third));
	atomic_set(&paired_cb_done, 1);

	return 0;
}

ZTEST(dns_dispatcher, test_unregister_with_reentrant_paired_dispatch)
{
	int iface_idx = net_if_get_by_iface(iface1);
	int sock = pair_create_socket();
	int third_sock = pair_create_socket();
	int ret;

	pair_setup(&pair_resp, DNS_SOCKET_RESPONDER, iface_idx, sock, &pair_resp_fd);
	ret = dns_dispatcher_register(&pair_resp);
	zassert_ok(ret, "responder register failed (%d)", ret);

	pair_setup(&pair_resv, DNS_SOCKET_RESOLVER, 0, sock, &pair_resv_fd);
	pair_resv.cb = pair_reentrant_cb;
	ret = dns_dispatcher_register(&pair_resv);
	zassert_ok(ret, "resolver register failed (%d)", ret);
	zassert_equal(pair_resp.pair, &pair_resv, "resolver did not pair with the responder");

	/* An unrelated context the callback registers, on its own port */
	pair_setup(&reentrant_third, DNS_SOCKET_RESOLVER, iface_idx, third_sock,
		   &reentrant_third_fd);
	net_sin6(&reentrant_third.local_addr)->sin6_port = net_htons(TEST_DNS_PAIR_PORT + 1);

	atomic_set(&paired_cb_done, 0);
	atomic_set(&paired_unreg_returned, 0);
	atomic_set(&reentrant_cb_ret, -1);
	k_sem_reset(&paired_cb_entered);
	k_sem_reset(&reentrant_cb_go);

	inject_dns_answer();
	zassert_ok(k_sem_take(&paired_cb_entered, K_SECONDS(2)),
		   "the answer was not delegated to the resolver");

	/* Unregister blocks waiting for the delegated dispatch. */
	k_thread_create(&paired_unreg_thread, paired_unreg_stack,
			K_THREAD_STACK_SIZEOF(paired_unreg_stack), paired_unreg_fn, &pair_resv,
			NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_sleep(K_MSEC(200));
	zassert_equal(atomic_get(&paired_unreg_returned), 0,
		      "unregister returned while the delegated dispatch was still running");

	/* Let the dispatch re-enter the dispatcher; it must get through. */
	k_sem_give(&reentrant_cb_go);
	zassert_ok(k_thread_join(&paired_unreg_thread, K_SECONDS(2)),
		   "unregister and the re-entrant dispatch block each other");
	zassert_equal(atomic_get(&paired_cb_done), 1, "the dispatch did not finish");
	zassert_ok(atomic_get(&reentrant_cb_ret), "register from the callback failed (%d)",
		   (int)atomic_get(&reentrant_cb_ret));
	zassert_is_null(pair_resp.pair, "responder still points at the unregistered resolver");

	ret = dns_dispatcher_unregister(&reentrant_third);
	zassert_ok(ret, "third unregister failed (%d)", ret);
	(void)zsock_close(third_sock);
	ret = dns_dispatcher_unregister(&pair_resp);
	zassert_ok(ret, "responder unregister failed (%d)", ret);
	(void)zsock_close(sock);
	pair_resp_fd.fd = -1;
	pair_resv_fd.fd = -1;
	reentrant_third_fd.fd = -1;
	memset(&pair_resp, 0, sizeof(pair_resp));
	memset(&pair_resv, 0, sizeof(pair_resv));
	memset(&reentrant_third, 0, sizeof(reentrant_third));
	pair_resp.sock = -1;
	pair_resv.sock = -1;
	reentrant_third.sock = -1;
}


/* Both halves of a pairing may be torn down at the same time, the primary
 * first. Its unregister takes it off the list and waits for the dispatch;
 * the pair's unregister must then still wait for the callback running in
 * it, although no listed context points at it any more.
 */
static K_THREAD_STACK_DEFINE(paired_unreg2_stack, 2048);
static struct k_thread paired_unreg2_thread;
static atomic_t paired_unreg2_returned;

static void paired_unreg2_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)dns_dispatcher_unregister(p1);
	atomic_set(&paired_unreg2_returned, 1);
}

ZTEST(dns_dispatcher, test_unregister_both_during_paired_dispatch)
{
	int iface_idx = net_if_get_by_iface(iface1);
	int sock = pair_create_socket();
	int ret;

	pair_setup(&pair_resp, DNS_SOCKET_RESPONDER, iface_idx, sock, &pair_resp_fd);
	ret = dns_dispatcher_register(&pair_resp);
	zassert_ok(ret, "responder register failed (%d)", ret);

	pair_setup(&pair_resv, DNS_SOCKET_RESOLVER, 0, sock, &pair_resv_fd);
	pair_resv.cb = pair_blocking_cb;
	ret = dns_dispatcher_register(&pair_resv);
	zassert_ok(ret, "resolver register failed (%d)", ret);
	zassert_equal(pair_resp.pair, &pair_resv, "resolver did not pair with the responder");

	atomic_set(&paired_cb_done, 0);
	atomic_set(&paired_unreg_returned, 0);
	atomic_set(&paired_unreg2_returned, 0);
	k_sem_reset(&paired_cb_entered);
	k_sem_reset(&paired_cb_release);

	inject_dns_answer();
	zassert_ok(k_sem_take(&paired_cb_entered, K_SECONDS(2)),
		   "the answer was not delegated to the resolver");

	/* Primary first, then the pair while the primary is still waiting */
	k_thread_create(&paired_unreg_thread, paired_unreg_stack,
			K_THREAD_STACK_SIZEOF(paired_unreg_stack), paired_unreg_fn, &pair_resp,
			NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_sleep(K_MSEC(100));
	k_thread_create(&paired_unreg2_thread, paired_unreg2_stack,
			K_THREAD_STACK_SIZEOF(paired_unreg2_stack), paired_unreg2_fn, &pair_resv,
			NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_sleep(K_MSEC(200));
	zassert_equal(atomic_get(&paired_unreg_returned), 0,
		      "primary unregister returned while the delegated dispatch was running");
	zassert_equal(atomic_get(&paired_unreg2_returned), 0,
		      "pair unregister returned while its callback was still running");

	k_sem_give(&paired_cb_release);
	zassert_ok(k_thread_join(&paired_unreg_thread, K_SECONDS(2)),
		   "primary unregister did not return");
	zassert_ok(k_thread_join(&paired_unreg2_thread, K_SECONDS(2)),
		   "pair unregister did not return");
	zassert_equal(atomic_get(&paired_cb_done), 1, "the dispatch did not finish");

	(void)zsock_close(sock);
	pair_resp_fd.fd = -1;
	pair_resv_fd.fd = -1;
	memset(&pair_resp, 0, sizeof(pair_resp));
	memset(&pair_resv, 0, sizeof(pair_resv));
	pair_resp.sock = -1;
	pair_resv.sock = -1;
}


/* A paired context is not on the registration list. Registering it a
 * second time must fail without re-initializing its lock, which a
 * delegated dispatch may be holding.
 */
ZTEST(dns_dispatcher, test_register_paired_twice)
{
	int iface_idx = net_if_get_by_iface(iface1);
	int sock = pair_create_socket();
	int ret;

	pair_setup(&pair_resp, DNS_SOCKET_RESPONDER, iface_idx, sock, &pair_resp_fd);
	ret = dns_dispatcher_register(&pair_resp);
	zassert_ok(ret, "responder register failed (%d)", ret);

	pair_setup(&pair_resv, DNS_SOCKET_RESOLVER, 0, sock, &pair_resv_fd);
	ret = dns_dispatcher_register(&pair_resv);
	zassert_ok(ret, "resolver register failed (%d)", ret);
	zassert_equal(pair_resp.pair, &pair_resv, "resolver did not pair with the responder");

	/* Stand in for a delegated dispatch holding the lock */
	zassert_ok(k_mutex_lock(&pair_resv.lock, K_NO_WAIT), "cannot take the resolver lock");

	ret = dns_dispatcher_register(&pair_resv);
	zassert_equal(ret, -EALREADY, "second register of a paired context (%d)", ret);
	zassert_equal(pair_resv.lock.lock_count, 1U, "the held lock was re-initialized");
	zassert_equal(pair_resv.lock.owner, k_current_get(), "the held lock lost its owner");

	zassert_ok(k_mutex_unlock(&pair_resv.lock), "cannot release the resolver lock");

	ret = dns_dispatcher_unregister(&pair_resv);
	zassert_ok(ret, "resolver unregister failed (%d)", ret);
	ret = dns_dispatcher_unregister(&pair_resp);
	zassert_ok(ret, "responder unregister failed (%d)", ret);
	(void)zsock_close(sock);
	pair_resp_fd.fd = -1;
	pair_resv_fd.fd = -1;
	memset(&pair_resp, 0, sizeof(pair_resp));
	memset(&pair_resv, 0, sizeof(pair_resv));
	pair_resp.sock = -1;
	pair_resv.sock = -1;
}


/* Registration claims a dispatch table slot for every descriptor in the
 * context's fds array, as the resolver's shared array makes it do for
 * more than its own socket. Unregistering must release all of them: a
 * later registration with the same array keeps the descriptor polled,
 * and its datagrams must not reach the unregistered context.
 */
static struct dns_socket_dispatcher *slot_last_cb_ctx;
static struct dns_socket_dispatcher slot_a;
static struct dns_socket_dispatcher slot_b;
static struct zsock_pollfd slot_fds[2];
static K_SEM_DEFINE(slot_cb_sem, 0, 1);

static int slot_cb(struct dns_socket_dispatcher *ctx, int sock, struct net_sockaddr *addr,
		   size_t addrlen, struct net_buf *buf, size_t len)
{
	ARG_UNUSED(sock);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	slot_last_cb_ctx = ctx;
	k_sem_give(&slot_cb_sem);

	return 0;
}

static void inject_dns_query(uint16_t port)
{
	static const uint8_t query[] = { 0x12, 0x34, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct net_in6_addr src = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 2 } } };
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(iface1, sizeof(query), NET_AF_INET6, NET_IPPROTO_UDP,
					K_FOREVER);
	zassert_not_null(pkt, "cannot allocate the query");
	zassert_ok(net_ipv6_create(pkt, &src, &my_addr1), "cannot create the IPv6 header");
	zassert_ok(net_udp_create(pkt, net_htons(port + 100), net_htons(port)),
		   "cannot create the UDP header");
	zassert_ok(net_pkt_write(pkt, query, sizeof(query)), "cannot write the query");
	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, NET_IPPROTO_UDP);
	zassert_ok(net_recv_data(iface1, pkt), "cannot deliver the query");
}

static void slot_setup(struct dns_socket_dispatcher *d, int sock, uint16_t port)
{
	memset(d, 0, sizeof(*d));
	d->type = DNS_SOCKET_RESPONDER;
	d->ifindex = net_if_get_by_iface(iface1);
	d->sock = sock;
	d->cb = slot_cb;
	d->fds = slot_fds;
	d->fds_len = ARRAY_SIZE(slot_fds);
	d->svc = &pair_svc;
	net_sin6(&d->local_addr)->sin6_family = NET_AF_INET6;
	net_sin6(&d->local_addr)->sin6_port = net_htons(port);
}

ZTEST(dns_dispatcher, test_unregister_releases_every_slot)
{
	struct net_sockaddr_in6 xaddr = { .sin6_family = NET_AF_INET6,
					  .sin6_port = net_htons(TEST_DNS_PAIR_PORT + 10) };
	int sock_a = pair_create_socket();
	int sock_x = pair_create_socket();
	int sock_b = pair_create_socket();
	int ret;

	/* X is a second socket of the same owner, bound by the owner and
	 * present in the shared array, like another resolver server.
	 */
	zassert_ok(zsock_bind(sock_x, (struct net_sockaddr *)&xaddr, sizeof(xaddr)),
		   "cannot bind the extra socket");

	slot_fds[0].fd = sock_a;
	slot_fds[0].events = ZSOCK_POLLIN;
	slot_fds[1].fd = sock_x;
	slot_fds[1].events = ZSOCK_POLLIN;

	slot_setup(&slot_a, sock_a, TEST_DNS_PAIR_PORT + 11);
	ret = dns_dispatcher_register(&slot_a);
	zassert_ok(ret, "register A failed (%d)", ret);

	/* While A is registered, X's datagrams are dispatched to A */
	slot_last_cb_ctx = NULL;
	k_sem_reset(&slot_cb_sem);
	inject_dns_query(TEST_DNS_PAIR_PORT + 10);
	zassert_ok(k_sem_take(&slot_cb_sem, K_SECONDS(2)), "no dispatch on X");
	zassert_equal(slot_last_cb_ctx, &slot_a, "X was not dispatched to A");

	ret = dns_dispatcher_unregister(&slot_a);
	zassert_ok(ret, "unregister A failed (%d)", ret);

	/* B takes over the array with its own socket in A's place */
	slot_fds[0].fd = sock_b;
	slot_setup(&slot_b, sock_b, TEST_DNS_PAIR_PORT + 12);
	ret = dns_dispatcher_register(&slot_b);
	zassert_ok(ret, "register B failed (%d)", ret);

	slot_last_cb_ctx = NULL;
	k_sem_reset(&slot_cb_sem);
	inject_dns_query(TEST_DNS_PAIR_PORT + 10);
	zassert_ok(k_sem_take(&slot_cb_sem, K_SECONDS(2)),
		   "no dispatch on X after re-registration");
	zassert_not_equal(slot_last_cb_ctx, &slot_a,
			  "datagram on X was dispatched to the unregistered context A");
	zassert_equal(slot_last_cb_ctx, &slot_b, "datagram on X was not dispatched to B");

	ret = dns_dispatcher_unregister(&slot_b);
	zassert_ok(ret, "unregister B failed (%d)", ret);
	(void)zsock_close(sock_a);
	(void)zsock_close(sock_x);
	(void)zsock_close(sock_b);
	slot_fds[0].fd = -1;
	slot_fds[1].fd = -1;
}


/* Unregistering a context releases the slots of every descriptor in its
 * array, but a descriptor stays polled while another context's copy of
 * the array holds it. A datagram on such a descriptor has no owner; it
 * must still be consumed, or the socket service reports it again at once.
 */
ZTEST(dns_dispatcher, test_datagram_without_owner_is_consumed)
{
	struct net_sockaddr_in6 xaddr = { .sin6_family = NET_AF_INET6,
					  .sin6_port = net_htons(TEST_DNS_PAIR_PORT + 30) };
	int sock_a = pair_create_socket();
	int sock_x = pair_create_socket();
	int sock_b = pair_create_socket();
	uint8_t buf[16];
	int ret = -1;

	zassert_ok(zsock_bind(sock_x, (struct net_sockaddr *)&xaddr, sizeof(xaddr)),
		   "cannot bind the shared socket");

	slot_fds[0].fd = sock_a;
	slot_fds[0].events = ZSOCK_POLLIN;
	slot_fds[1].fd = sock_x;
	slot_fds[1].events = ZSOCK_POLLIN;

	/* A owns both slots; B shares the array and the service but owns none */
	slot_setup(&slot_a, sock_a, TEST_DNS_PAIR_PORT + 31);
	ret = dns_dispatcher_register(&slot_a);
	zassert_ok(ret, "register A failed (%d)", ret);
	slot_setup(&slot_b, sock_b, TEST_DNS_PAIR_PORT + 32);
	ret = dns_dispatcher_register(&slot_b);
	zassert_ok(ret, "register B failed (%d)", ret);

	/* Unregistering A releases X's slot, but B keeps X polled */
	ret = dns_dispatcher_unregister(&slot_a);
	zassert_ok(ret, "unregister A failed (%d)", ret);

	slot_last_cb_ctx = NULL;
	k_sem_reset(&slot_cb_sem);
	inject_dns_query(TEST_DNS_PAIR_PORT + 30);

	/* Nobody is dispatched, and the datagram must be gone from the socket.
	 * Peek so that this thread does not consume it itself.
	 */
	for (int i = 0; i < 50; i++) {
		k_msleep(20);
		ret = zsock_recv(sock_x, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT | ZSOCK_MSG_PEEK);
		if (ret < 0 && errno == EAGAIN) {
			break;
		}
	}

	zassert_true(ret < 0 && errno == EAGAIN,
		     "datagram on the unowned descriptor was left in the socket (%d)", ret);
	zassert_is_null(slot_last_cb_ctx, "datagram without owner was dispatched");

	ret = dns_dispatcher_unregister(&slot_b);
	zassert_ok(ret, "unregister B failed (%d)", ret);
	(void)zsock_close(sock_a);
	(void)zsock_close(sock_x);
	(void)zsock_close(sock_b);
	slot_fds[0].fd = -1;
	slot_fds[1].fd = -1;
}

/* A registration whose socket service cannot take its descriptor array
 * fails. It must leave no dispatch-table slot behind: a slot it kept would
 * make a later registration sharing the descriptor skip claiming it, and
 * that descriptor's datagrams would go to the failed context.
 */
static struct dns_socket_dispatcher svcfail_ctx;
static struct zsock_pollfd svcfail_fds[3];

ZTEST(dns_dispatcher, test_failed_register_claims_no_slot)
{
	struct net_sockaddr_in6 xaddr = { .sin6_family = NET_AF_INET6,
					  .sin6_port = net_htons(TEST_DNS_PAIR_PORT + 20) };
	int sock_f = pair_create_socket();
	int sock_x = pair_create_socket();
	int sock_b = pair_create_socket();
	int ret;

	zassert_ok(zsock_bind(sock_x, (struct net_sockaddr *)&xaddr, sizeof(xaddr)),
		   "cannot bind the shared socket");

	/* Three descriptors for a service that polls two: registering the
	 * service fails with -ENOMEM after the socket has been bound.
	 */
	memset(&svcfail_ctx, 0, sizeof(svcfail_ctx));
	svcfail_ctx.type = DNS_SOCKET_RESPONDER;
	svcfail_ctx.ifindex = net_if_get_by_iface(iface1);
	svcfail_ctx.sock = sock_f;
	svcfail_ctx.cb = slot_cb;
	svcfail_ctx.fds = svcfail_fds;
	svcfail_ctx.fds_len = ARRAY_SIZE(svcfail_fds);
	svcfail_ctx.svc = &pair_svc;
	net_sin6(&svcfail_ctx.local_addr)->sin6_family = NET_AF_INET6;
	net_sin6(&svcfail_ctx.local_addr)->sin6_port = net_htons(TEST_DNS_PAIR_PORT + 21);
	svcfail_fds[0].fd = sock_f;
	svcfail_fds[0].events = ZSOCK_POLLIN;
	svcfail_fds[1].fd = sock_x;
	svcfail_fds[1].events = ZSOCK_POLLIN;
	svcfail_fds[2].fd = -1;

	ret = dns_dispatcher_register(&svcfail_ctx);
	zassert_equal(ret, -ENOMEM, "register with too many descriptors (%d)", ret);

	/* B shares the descriptor X and must get its datagrams */
	slot_fds[0].fd = sock_b;
	slot_fds[0].events = ZSOCK_POLLIN;
	slot_fds[1].fd = sock_x;
	slot_fds[1].events = ZSOCK_POLLIN;
	slot_setup(&slot_b, sock_b, TEST_DNS_PAIR_PORT + 22);
	ret = dns_dispatcher_register(&slot_b);
	zassert_ok(ret, "register B failed (%d)", ret);

	slot_last_cb_ctx = NULL;
	k_sem_reset(&slot_cb_sem);
	inject_dns_query(TEST_DNS_PAIR_PORT + 20);
	zassert_ok(k_sem_take(&slot_cb_sem, K_SECONDS(2)), "no dispatch on X");
	zassert_not_equal(slot_last_cb_ctx, &svcfail_ctx,
			  "datagram on X was dispatched to the failed registration");
	zassert_equal(slot_last_cb_ctx, &slot_b, "datagram on X was not dispatched to B");

	ret = dns_dispatcher_unregister(&slot_b);
	zassert_ok(ret, "unregister B failed (%d)", ret);
	(void)zsock_close(sock_f);
	(void)zsock_close(sock_x);
	(void)zsock_close(sock_b);
	slot_fds[0].fd = -1;
	slot_fds[1].fd = -1;
}

#endif /* !DNS_DISPATCHER_MULTI_IFACE_TEST */

ZTEST_SUITE(dns_dispatcher, NULL, test_init, NULL, NULL, NULL);
