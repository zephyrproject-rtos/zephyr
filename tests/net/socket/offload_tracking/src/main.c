/*
 * Copyright 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

/* Minimal socket offload mock. Sockets it creates are "offloaded": they bind
 * their local port outside the net_context layer, so the tracking table is what
 * makes net_context_port_in_use() see them. Whether socket() picks this mock or
 * the native stack is decided by offload_is_supported(), toggled per test.
 */

static bool offload_enabled;
static bool offload_socket_created;
static int offload_type;
static int offload_proto;
static int offload_obj;
static int test_sock = -1;

static int offload_close(void *obj, int fd)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(fd);

	return 0;
}

static int offload_bind(void *obj, const struct net_sockaddr *addr, net_socklen_t addrlen)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	return 0;
}

static int offload_getsockopt(void *obj, int level, int optname, void *optval,
			      net_socklen_t *optlen)
{
	ARG_UNUSED(obj);

	/* Report the socket protocol/type so that offloaded port tracking can
	 * classify the socket, as a real offload engine would.
	 */
	if (level == ZSOCK_SOL_SOCKET && optval != NULL && optlen != NULL &&
	    *optlen >= sizeof(int)) {
		if (optname == ZSOCK_SO_PROTOCOL) {
			*(int *)optval = offload_proto;
			*optlen = sizeof(int);
		} else if (optname == ZSOCK_SO_TYPE) {
			*(int *)optval = offload_type;
			*optlen = sizeof(int);
		}
	}

	return 0;
}

static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	.fd_vtable =
		{
			.close2 = offload_close,
		},
	.bind = offload_bind,
	.getsockopt = offload_getsockopt,
};

static int offload_socket(int family, int type, int proto)
{
	int fd = zvfs_reserve_fd();

	if (fd < 0) {
		return -1;
	}

	zvfs_finalize_typed_fd(fd, &offload_obj,
			       (const struct fd_op_vtable *)&offload_socket_fd_op_vtable,
			       ZVFS_MODE_IFSOCK);

	offload_socket_created = true;
	offload_type = type;
	offload_proto = proto;

	return fd;
}

static bool offload_is_supported(int family, int type, int proto)
{
	ARG_UNUSED(family);
	ARG_UNUSED(type);

	/* TLS/DTLS wrappers stay with the native mbedTLS layer; only the plain
	 * transport underneath is offloaded.
	 */
	return offload_enabled && (proto == NET_IPPROTO_TCP || proto == NET_IPPROTO_UDP);
}

NET_SOCKET_OFFLOAD_REGISTER(offloaded, 10, NET_AF_UNSPEC, offload_is_supported, offload_socket);

/* Native interface, so a native socket can bind a real local port. */

static uint8_t lladdr[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
static struct net_in_addr in4addr_my = {{{192, 0, 2, 1}}};

static void dummy_native_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, lladdr, 6, NET_LINK_DUMMY);
	net_if_ipv4_addr_add(iface, &in4addr_my, NET_ADDR_MANUAL, 0);
}

static int dummy_native_dev_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static const struct dummy_api dummy_native_dev_api = {
	.iface_api.init = dummy_native_iface_init,
	.send = dummy_native_dev_send,
};

NET_DEVICE_INIT(dummy_native, "dummy_native", NULL, NULL, NULL, NULL, 0, &dummy_native_dev_api,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 1500);

static struct net_sockaddr_in test_local_addr(uint16_t port)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(port),
		.sin_addr = {{{192, 0, 2, 2}}},
	};

	return addr;
}

static void bind_offloaded_udp(uint16_t port, struct net_sockaddr_in *addr)
{
	*addr = test_local_addr(port);

	test_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(test_sock >= 0, "socket() failed");
	zassert_true(offload_socket_created, "Socket was not offloaded");
	zassert_ok(zsock_bind(test_sock, (struct net_sockaddr *)addr, sizeof(*addr)),
		   "bind() failed");
}

static void offload_tracking_before(void *dummy)
{
	ARG_UNUSED(dummy);

	offload_enabled = true;
	offload_socket_created = false;
	test_sock = -1;
}

static void offload_tracking_after(void *dummy)
{
	ARG_UNUSED(dummy);

	if (test_sock >= 0) {
		(void)zsock_close(test_sock);
		test_sock = -1;
	}

	offload_enabled = false;
}

/* A port bound through an offloaded socket is reported in use, while a
 * different port or a different protocol on the same port is not.
 */
ZTEST(net_socket_offload_tracking, test_offloaded_bind_tracked)
{
	struct net_sockaddr_in addr;
	struct net_sockaddr_in other = test_local_addr(4243);

	bind_offloaded_udp(4242, &addr);

	zassert_true(net_context_port_in_use(NET_IPPROTO_UDP, 4242, (struct net_sockaddr *)&addr),
		     "Bound offloaded port not reported in use");
	zassert_false(net_context_port_in_use(NET_IPPROTO_UDP, 4243, (struct net_sockaddr *)&other),
		      "Unbound port reported in use");
	zassert_false(net_context_port_in_use(NET_IPPROTO_TCP, 4242, (struct net_sockaddr *)&addr),
		      "Wrong protocol reported in use");
}

/* Closing the socket releases its tracked port. */
ZTEST(net_socket_offload_tracking, test_close_releases_port)
{
	struct net_sockaddr_in addr;

	bind_offloaded_udp(4244, &addr);
	zassert_true(net_context_port_in_use(NET_IPPROTO_UDP, 4244, (struct net_sockaddr *)&addr),
		     "Bound offloaded port not reported in use");

	zassert_ok(zsock_close(test_sock), "close() failed");
	test_sock = -1;

	zassert_false(net_context_port_in_use(NET_IPPROTO_UDP, 4244, (struct net_sockaddr *)&addr),
		      "Port still in use after close");
}

/* Re-binding the same socket moves its tracked entry to the new port. */
ZTEST(net_socket_offload_tracking, test_rebind_updates_entry)
{
	struct net_sockaddr_in first;
	struct net_sockaddr_in second = test_local_addr(4247);

	bind_offloaded_udp(4246, &first);
	zassert_true(net_context_port_in_use(NET_IPPROTO_UDP, 4246, (struct net_sockaddr *)&first),
		     "First port not reported in use");

	zassert_ok(zsock_bind(test_sock, (struct net_sockaddr *)&second, sizeof(second)),
		   "rebind() failed");

	zassert_false(net_context_port_in_use(NET_IPPROTO_UDP, 4246, (struct net_sockaddr *)&first),
		      "Old port not released on rebind");
	zassert_true(net_context_port_in_use(NET_IPPROTO_UDP, 4247, (struct net_sockaddr *)&second),
		     "New port not tracked on rebind");
}

/* A wildcard (unspecified address) binding matches a query for a specific
 * local address on that port.
 */
ZTEST(net_socket_offload_tracking, test_wildcard_addr_matches)
{
	struct net_sockaddr_in any = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(4248),
	};
	struct net_sockaddr_in specific = test_local_addr(4248);

	test_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(test_sock >= 0, "socket() failed");
	zassert_true(offload_socket_created, "Socket was not offloaded");
	zassert_ok(zsock_bind(test_sock, (struct net_sockaddr *)&any, sizeof(any)),
		   "bind() failed");

	zassert_true(
		net_context_port_in_use(NET_IPPROTO_UDP, 4248, (struct net_sockaddr *)&specific),
		"Wildcard-bound port not matched for specific address");
}

/* A native socket binding is visible to net_context directly and must not be
 * duplicated in the offloaded-port table.
 */
ZTEST(net_socket_offload_tracking, test_native_bind_not_offload_tracked)
{
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(4245),
		.sin_addr = in4addr_my,
	};

	offload_enabled = false;

	test_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(test_sock >= 0, "socket() failed");
	zassert_false(offload_socket_created, "Socket should have been native");
	zassert_ok(zsock_bind(test_sock, (struct net_sockaddr *)&addr, sizeof(addr)),
		   "bind() failed");

	zassert_false(net_socket_offloaded_port_in_use(NET_IPPROTO_UDP, 4245,
						       (struct net_sockaddr *)&addr),
		      "Native binding wrongly added to offloaded-port table");
}

/* Native TLS over an offloaded transport: the transport socket carries the
 * tracked port and the TLS wrapper fd is skipped, so the port is still
 * reported in use.
 */
ZTEST(net_socket_offload_tracking, test_tls_over_offloaded_tracked)
{
	struct net_sockaddr_in addr = test_local_addr(4249);

	test_sock = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);
	zassert_true(test_sock >= 0, "socket() failed");
	zassert_true(offload_socket_created, "TLS transport was not offloaded");
	zassert_ok(zsock_bind(test_sock, (struct net_sockaddr *)&addr, sizeof(addr)),
		   "bind() failed");

	zassert_true(net_context_port_in_use(NET_IPPROTO_TCP, 4249, (struct net_sockaddr *)&addr),
		     "TLS-over-offloaded port not tracked via transport");
}

/* Native and offloaded sockets bound at the same time: net_context_port_in_use()
 * reports both, and each port stays in exactly one of the two tables.
 */
ZTEST(net_socket_offload_tracking, test_native_and_offloaded_coexist)
{
	struct net_sockaddr_in off_addr = test_local_addr(4250);
	struct net_sockaddr_in nat_addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(4251),
		.sin_addr = in4addr_my,
	};
	int native_sock;

	offload_enabled = true;
	test_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(test_sock >= 0, "offloaded socket() failed");
	zassert_true(offload_socket_created, "Socket was not offloaded");
	zassert_ok(zsock_bind(test_sock, (struct net_sockaddr *)&off_addr, sizeof(off_addr)),
		   "offloaded bind() failed");

	offload_enabled = false;
	offload_socket_created = false;
	native_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(native_sock >= 0, "native socket() failed");
	zassert_false(offload_socket_created, "Socket should have been native");
	zassert_ok(zsock_bind(native_sock, (struct net_sockaddr *)&nat_addr, sizeof(nat_addr)),
		   "native bind() failed");

	zassert_true(
		net_context_port_in_use(NET_IPPROTO_UDP, 4250, (struct net_sockaddr *)&off_addr),
		"Offloaded port not reported in a mixed setup");
	zassert_true(
		net_context_port_in_use(NET_IPPROTO_UDP, 4251, (struct net_sockaddr *)&nat_addr),
		"Native port not reported in a mixed setup");

	zassert_true(net_socket_offloaded_port_in_use(NET_IPPROTO_UDP, 4250,
						      (struct net_sockaddr *)&off_addr),
		     "Offloaded port missing from offloaded table");
	zassert_false(net_socket_offloaded_port_in_use(NET_IPPROTO_UDP, 4251,
						       (struct net_sockaddr *)&nat_addr),
		      "Native port wrongly present in offloaded table");

	(void)zsock_close(native_sock);
}

ZTEST_SUITE(net_socket_offload_tracking, NULL, NULL, offload_tracking_before,
	    offload_tracking_after, NULL);
