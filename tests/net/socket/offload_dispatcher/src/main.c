/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <sockets_internal.h>
#include <zephyr/sys/fdtable.h>
#include <ztest.h>


LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

/* Generic test offload API */

#define OFFLOAD_1 0
#define OFFLOAD_2 1
#define OFFLOAD_COUNT 2

static struct test_socket_calls {
	bool socket_called;
	bool close_called;
	bool ioctl_called;
	bool shutdown_called;
	bool bind_called;
	bool connect_called;
	bool listen_called;
	bool accept_called;
	bool sendto_called;
	bool recvfrom_called;
	bool getsockopt_called;
	bool setsockopt_called;
	bool sendmsg_called;
	bool getsockname_called;
	bool getpeername_called;
} test_socket_ctx[OFFLOAD_COUNT];

static int test_sock = -1;

static ssize_t offload_read(void *obj, void *buffer, size_t count)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(buffer);
	ARG_UNUSED(count);

	return 0;
}

static ssize_t offload_write(void *obj, const void *buffer, size_t count)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(buffer);
	ARG_UNUSED(count);

	return 0;
}

static int offload_close(void *obj)
{
	struct test_socket_calls *ctx = obj;

	ctx->close_called = true;

	return 0;
}

static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(request);
	ARG_UNUSED(args);

	ctx->ioctl_called = true;

	return 0;
}

static int offload_shutdown(void *obj, int how)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(how);

	ctx->shutdown_called = true;

	return 0;
}

static int offload_bind(void *obj, const struct sockaddr *addr,
			socklen_t addrlen)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	ctx->bind_called = true;

	return 0;
}

static int offload_connect(void *obj, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	ctx->connect_called = true;

	return 0;
}

static int offload_listen(void *obj, int backlog)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(backlog);

	ctx->listen_called = true;

	return 0;
}

static int offload_accept(void *obj, struct sockaddr *addr, socklen_t *addrlen)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	ctx->accept_called = true;

	return 0;
}

static ssize_t offload_sendto(void *obj, const void *buf, size_t len,
			      int flags, const struct sockaddr *dest_addr,
			      socklen_t addrlen)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(flags);
	ARG_UNUSED(dest_addr);
	ARG_UNUSED(addrlen);

	ctx->sendto_called = true;

	return len;
}

static ssize_t offload_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(msg);
	ARG_UNUSED(flags);

	ctx->sendmsg_called = true;

	return 0;
}

static ssize_t offload_recvfrom(void *obj, void *buf, size_t max_len,
				int flags, struct sockaddr *src_addr,
				socklen_t *addrlen)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(buf);
	ARG_UNUSED(max_len);
	ARG_UNUSED(flags);
	ARG_UNUSED(src_addr);
	ARG_UNUSED(addrlen);

	ctx->recvfrom_called = true;

	return 0;
}

static int offload_getsockopt(void *obj, int level, int optname,
			      void *optval, socklen_t *optlen)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(level);
	ARG_UNUSED(optname);
	ARG_UNUSED(optval);
	ARG_UNUSED(optlen);

	ctx->getsockopt_called = true;

	return 0;
}

static int offload_setsockopt(void *obj, int level, int optname,
			      const void *optval, socklen_t optlen)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(level);
	ARG_UNUSED(optname);
	ARG_UNUSED(optval);
	ARG_UNUSED(optlen);

	ctx->setsockopt_called = true;

	return 0;
}

static int offload_getpeername(void *obj, struct sockaddr *addr,
			       socklen_t *addrlen)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	ctx->getpeername_called = true;

	return 0;
}

static int offload_getsockname(void *obj, struct sockaddr *addr,
			       socklen_t *addrlen)
{
	struct test_socket_calls *ctx = obj;

	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	ctx->getsockname_called = true;

	return 0;
}

/* Offloaded interface 1 - high priority */

#define SOCKET_OFFLOAD_PRIO_HIGH 10

static const struct socket_op_vtable offload_1_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = offload_read,
		.write = offload_write,
		.close = offload_close,
		.ioctl = offload_ioctl,
	},
	.shutdown = offload_shutdown,
	.bind = offload_bind,
	.connect = offload_connect,
	.listen = offload_listen,
	.accept = offload_accept,
	.sendto = offload_sendto,
	.recvfrom = offload_recvfrom,
	.getsockopt = offload_getsockopt,
	.setsockopt = offload_setsockopt,
	.sendmsg = offload_sendmsg,
	.getsockname = offload_getsockname,
	.getpeername = offload_getpeername,
};

int offload_1_socket(int family, int type, int proto)
{
	int fd = z_reserve_fd();

	if (fd < 0) {
		return -1;
	}

	z_finalize_fd(fd, &test_socket_ctx[OFFLOAD_1],
		      (const struct fd_op_vtable *)
					&offload_1_socket_fd_op_vtable);

	test_socket_ctx[OFFLOAD_1].socket_called = true;

	return fd;
}

static bool offload_1_is_supported(int family, int type, int proto)
{
	return true;
}

NET_SOCKET_OFFLOAD_REGISTER(offloaded_1, SOCKET_OFFLOAD_PRIO_HIGH, AF_UNSPEC,
			    offload_1_is_supported, offload_1_socket);


static int offloaded_1_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static void offloaded_1_iface_init(struct net_if *iface)
{
	net_if_socket_offload_set(iface, offload_1_socket);
}

static struct net_if_api offloaded_1_if_api = {
	.init = offloaded_1_iface_init,
};

NET_DEVICE_OFFLOAD_INIT(offloaded_1, "offloaded_1", offloaded_1_init, NULL,
			NULL, NULL, 0, &offloaded_1_if_api, 1500);

/* Offloaded interface 2 - low priority */

#define SOCKET_OFFLOAD_PRIO_LOW 20

static const struct socket_op_vtable offload_2_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = offload_read,
		.write = offload_write,
		.close = offload_close,
		.ioctl = offload_ioctl,
	},
	.shutdown = offload_shutdown,
	.bind = offload_bind,
	.connect = offload_connect,
	.listen = offload_listen,
	.accept = offload_accept,
	.sendto = offload_sendto,
	.recvfrom = offload_recvfrom,
	.getsockopt = offload_getsockopt,
	.setsockopt = offload_setsockopt,
	.sendmsg = offload_sendmsg,
	.getsockname = offload_getsockname,
	.getpeername = offload_getpeername,
};

int offload_2_socket(int family, int type, int proto)
{
	int fd = z_reserve_fd();

	if (fd < 0) {
		return -1;
	}

	z_finalize_fd(fd, &test_socket_ctx[OFFLOAD_2],
		      (const struct fd_op_vtable *)
					&offload_2_socket_fd_op_vtable);

	test_socket_ctx[OFFLOAD_2].socket_called = true;

	return fd;
}

static bool offload_2_is_supported(int family, int type, int proto)
{
	return true;
}

NET_SOCKET_OFFLOAD_REGISTER(offloaded_2, SOCKET_OFFLOAD_PRIO_HIGH, AF_UNSPEC,
			    offload_2_is_supported, offload_2_socket);


static int offloaded_2_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static void offloaded_2_iface_init(struct net_if *iface)
{
	net_if_socket_offload_set(iface, offload_2_socket);
}

static struct net_if_api offloaded_2_if_api = {
	.init = offloaded_2_iface_init,
};

NET_DEVICE_OFFLOAD_INIT(offloaded_2, "offloaded_2", offloaded_2_init, NULL,
			NULL, NULL, 0, &offloaded_2_if_api, 1500);


/* Native dummy interface */

static uint8_t lladdr[] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
static struct in_addr in4addr_my = { { { 192, 0, 2, 1 } } };
static K_SEM_DEFINE(test_native_send_called, 0, 1);

static void dummy_native_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, lladdr, 6, NET_LINK_DUMMY);
	net_if_ipv4_addr_add(iface, &in4addr_my, NET_ADDR_MANUAL, 0);
}

static int dummy_native_dev_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	k_sem_give(&test_native_send_called);

	return 0;
}

int dummy_native_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct dummy_api dummy_native_dev_api = {
	.iface_api.init = dummy_native_iface_init,
	.send = dummy_native_dev_send,
};

NET_DEVICE_INIT(dummy_native, "dummy_native", dummy_native_dev_init, NULL, NULL,
		NULL, 0, &dummy_native_dev_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 1500);

/* Actual tests */

static const struct sockaddr_in test_peer_addr = {
	.sin_family = AF_INET,
	.sin_addr = { { { 192, 0, 0, 2 } } },
	.sin_port = 1234
};

static void test_result_reset(void)
{
	memset(test_socket_ctx, 0, sizeof(test_socket_ctx));
	k_sem_reset(&test_native_send_called);
}

static void test_socket_setup_udp(void)
{
	test_result_reset();

	test_sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(test_sock >= 0, "Failed to create socket");
	zassert_false(test_socket_ctx[OFFLOAD_1].socket_called,
		      "Socket should'nt have been dispatched yet");
}

static void test_socket_setup_tls(void)
{
	test_result_reset();

	test_sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
	zassert_true(test_sock >= 0, "Failed to create socket");
	zassert_false(test_socket_ctx[OFFLOAD_1].socket_called,
		      "Socket should'nt have been dispatched yet");
}

static void test_socket_teardown(void)
{
	int ret = zsock_close(test_sock);

	test_sock = -1;

	zassert_equal(0, ret, "close() failed");
}

/* Verify that socket is not dispatched when close() is called immediately after
 * creating dispatcher socket.
 */
static void test_close_not_bound(void)
{
	int ret =  zsock_close(test_sock);

	test_sock = -1;

	zassert_equal(0, ret, "close() failed");
	zassert_false(test_socket_ctx[OFFLOAD_1].socket_called,
		      "Socket should'nt have been dispatched");
	zassert_false(test_socket_ctx[OFFLOAD_1].close_called,
		      "close() should'nt have been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on ioctl() call, if not bound.
 */
static void test_fcntl_not_bound(void)
{
	int ret;

	ret = zsock_fcntl(test_sock, F_SETFL, 0);
	zassert_equal(0, ret, "fcntl() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].ioctl_called,
		     "fcntl() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on shutdown() call, if not bound.
 */
static void test_shutdown_not_bound(void)
{
	int ret;

	ret = zsock_shutdown(test_sock, ZSOCK_SHUT_RD);
	zassert_equal(0, ret, "shutdown() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].shutdown_called,
		     "shutdown() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on bind() call, if not bound.
 */
static void test_bind_not_bound(void)
{
	int ret;
	struct sockaddr_in addr = {
		.sin_family = AF_INET
	};

	ret = zsock_bind(test_sock, (struct sockaddr *)&addr, sizeof(addr));
	zassert_equal(0, ret, "bind() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].bind_called,
		     "bind() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on connect() call, if not bound.
 */
static void test_connect_not_bound(void)
{
	int ret;
	struct sockaddr_in addr = test_peer_addr;

	ret = zsock_connect(test_sock, (struct sockaddr *)&addr, sizeof(addr));
	zassert_equal(0, ret, "connect() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].connect_called,
		     "connect() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on listen() call, if not bound.
 */
static void test_listen_not_bound(void)
{
	int ret;

	ret = zsock_listen(test_sock, 1);
	zassert_equal(0, ret, "listen() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].listen_called,
		     "listen() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on accept() call, if not bound.
 */
static void test_accept_not_bound(void)
{
	int ret;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	ret = zsock_accept(test_sock, (struct sockaddr *)&addr, &addrlen);
	zassert_equal(0, ret, "accept() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].accept_called,
		     "accept() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on sendto() call, if not bound.
 */
static void test_sendto_not_bound(void)
{
	int ret;
	uint8_t dummy_data = 0;
	struct sockaddr_in addr = test_peer_addr;

	ret = zsock_sendto(test_sock, &dummy_data, 1, 0,
			   (struct sockaddr *)&addr, sizeof(addr));
	zassert_equal(1, ret, "sendto() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].sendto_called,
		     "sendto() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on recvfrom() call, if not bound.
 */
static void test_recvfrom_not_bound(void)
{
	int ret;
	uint8_t dummy_data = 0;

	ret = zsock_recvfrom(test_sock, &dummy_data, 1, 0, NULL, 0);
	zassert_equal(0, ret, "recvfrom() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].recvfrom_called,
		     "recvfrom() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on getsockopt() call, if not bound.
 */
static void test_getsockopt_not_bound(void)
{
	int ret;
	struct timeval optval = { 0 };
	socklen_t optlen = sizeof(optval);

	ret = zsock_getsockopt(test_sock, SOL_SOCKET, SO_RCVTIMEO,
			       &optval, &optlen);
	zassert_equal(0, ret, "getsockopt() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].getsockopt_called,
		     "getsockopt() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on setsockopt() call, if not bound.
 */
static void test_setsockopt_not_bound(void)
{
	int ret;
	struct timeval optval = { 0 };

	ret = zsock_setsockopt(test_sock, SOL_SOCKET, SO_RCVTIMEO,
			       &optval, sizeof(optval));
	zassert_equal(0, ret, "setsockopt() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].setsockopt_called,
		     "setsockopt() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on sendmsg() call, if not bound.
 */
static void test_sendmsg_not_bound(void)
{
	int ret;
	struct msghdr dummy_msg = { 0 };

	ret = zsock_sendmsg(test_sock, &dummy_msg, 0);
	zassert_equal(0, ret, "sendmsg() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].sendmsg_called,
		     "sendmsg() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on getpeername() call, if not bound.
 */
static void test_getpeername_not_bound(void)
{
	int ret;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	ret = zsock_getpeername(test_sock, (struct sockaddr *)&addr, &addrlen);
	zassert_equal(0, ret, "getpeername() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].getpeername_called,
		     "getpeername() should've been dispatched");
}

/* Verify that socket is automatically dispatched to a default socket
 * implementation on getsockname() call, if not bound.
 */
static void test_getsockname_not_bound(void)
{
	int ret;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	ret = zsock_getsockname(test_sock, (struct sockaddr *)&addr, &addrlen);
	zassert_equal(0, ret, "getsockname() failed");
	zassert_true(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket should've been dispatched");
	zassert_true(test_socket_ctx[OFFLOAD_1].getsockname_called,
		     "getsockname() should've been dispatched");
}

/* Verify that socket is dispatched to a proper offloaded socket implementation
 * if the socket is bound to an offloaded interface.
 */
static void test_so_bindtodevice_iface_offloaded(void)
{
	int ret;
	uint8_t dummy_data = 0;
	struct ifreq ifreq = {
		.ifr_name = "offloaded_2"
	};
	struct sockaddr_in addr = {
		.sin_family = AF_INET
	};

	ret = zsock_setsockopt(test_sock, SOL_SOCKET, SO_BINDTODEVICE,
			       &ifreq, sizeof(ifreq));
	zassert_equal(0, ret, "setsockopt() failed");
	zassert_false(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket dispatched to wrong iface");
	zassert_true(test_socket_ctx[OFFLOAD_2].socket_called,
		     "Socket should've been dispatched to offloaded iface 2");
	zassert_true(test_socket_ctx[OFFLOAD_2].setsockopt_called,
		     "setsockopt() should've been dispatched");

	ret = zsock_sendto(test_sock, &dummy_data, 1, 0,
			   (struct sockaddr *)&addr, sizeof(addr));
	zassert_equal(1, ret, "sendto() failed");
	zassert_true(test_socket_ctx[OFFLOAD_2].sendto_called,
		     "sendto() should've been dispatched");
}

/* Verify that socket is dispatched to a native socket implementation
 * if the socket is bound to a native interface.
 */
static void test_so_bindtodevice_iface_native(void)
{
	int ret;
	uint8_t dummy_data = 0;
	struct ifreq ifreq = {
		.ifr_name = "dummy_native"
	};
	struct sockaddr_in addr = test_peer_addr;

	ret = zsock_setsockopt(test_sock, SOL_SOCKET, SO_BINDTODEVICE,
			       &ifreq, sizeof(ifreq));
	zassert_equal(0, ret, "setsockopt() failed");
	zassert_false(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Socket dispatched to wrong iface");
	zassert_false(test_socket_ctx[OFFLOAD_2].socket_called,
		     "Socket dispatched to wrong iface");

	ret = zsock_sendto(test_sock, &dummy_data, 1, 0,
			   (struct sockaddr *)&addr, sizeof(addr));
	zassert_equal(1, ret, "sendto() failed %d", errno);

	ret = k_sem_take(&test_native_send_called, K_MSEC(200));
	zassert_equal(0, ret, "sendto() should've been dispatched to native iface");
}

/* Verify that the underlying socket is dispatched to a proper offloaded socket
 * implementation if native TLS is used and the socket is bound to an offloaded
 * interface.
 */
static void test_tls_native_iface_offloaded(void)
{
	int ret;
	const struct fd_op_vtable *vtable;
	void *obj;
	struct ifreq ifreq = {
		.ifr_name = "offloaded_2"
	};
	int tls_native = 1;
	struct sockaddr_in addr = test_peer_addr;

	ret = zsock_setsockopt(test_sock, SOL_TLS, TLS_NATIVE,
			       &tls_native, sizeof(tls_native));
	zassert_equal(0, ret, "setsockopt() failed");
	zassert_false(test_socket_ctx[OFFLOAD_1].socket_called,
		     "TLS socket dispatched to wrong iface");
	zassert_false(test_socket_ctx[OFFLOAD_2].socket_called,
		     "TLS socket dispatched to wrong iface");

	obj = z_get_fd_obj_and_vtable(test_sock, &vtable, NULL);
	zassert_not_null(obj, "No obj found");
	zassert_true(net_socket_is_tls(obj), "Socket is not a native TLS sock");

	ret = zsock_setsockopt(test_sock, SOL_SOCKET, SO_BINDTODEVICE,
			       &ifreq, sizeof(ifreq));
	zassert_equal(0, ret, "setsockopt() failed");
	zassert_false(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Underlying socket dispatched to wrong iface");
	zassert_true(test_socket_ctx[OFFLOAD_2].socket_called,
		     "Underlying socket dispatched to wrong iface");

	/* Ignore connect result as it will fail anyway. Just verify the
	 * call/packets were forwarded to a valid iface.
	 */
	ret = zsock_connect(test_sock, (struct sockaddr *)&addr, sizeof(addr));
	zassert_true(test_socket_ctx[OFFLOAD_2].connect_called,
		     "connect() should've been dispatched to offloaded_2 iface");
}

/* Verify that the underlying socket is dispatched to a native socket
 * implementation if native TLS is used and the socket is bound to a native
 * interface.
 */
static void test_tls_native_iface_native(void)
{
	int ret;
	const struct fd_op_vtable *vtable;
	void *obj;
	struct ifreq ifreq = {
		.ifr_name = "dummy_native"
	};
	int tls_native = 1;
	struct sockaddr_in addr = test_peer_addr;

	ret = zsock_setsockopt(test_sock, SOL_TLS, TLS_NATIVE,
			       &tls_native, sizeof(tls_native));
	zassert_equal(0, ret, "setsockopt() failed");
	zassert_false(test_socket_ctx[OFFLOAD_1].socket_called,
		     "TLS socket dispatched to wrong iface");
	zassert_false(test_socket_ctx[OFFLOAD_2].socket_called,
		     "TLS socket dispatched to wrong iface");

	obj = z_get_fd_obj_and_vtable(test_sock, &vtable, NULL);
	zassert_not_null(obj, "No obj found");
	zassert_true(net_socket_is_tls(obj), "Socket is not a native TLS sock");

	ret = zsock_setsockopt(test_sock, SOL_SOCKET, SO_BINDTODEVICE,
			       &ifreq, sizeof(ifreq));
	zassert_equal(0, ret, "setsockopt() failed");
	zassert_false(test_socket_ctx[OFFLOAD_1].socket_called,
		     "Underlying socket dispatched to wrong iface");
	zassert_false(test_socket_ctx[OFFLOAD_2].socket_called,
		     "Underlying socket dispatched to wrong iface");

	/* Ignore connect result as it will fail anyway. Just verify the
	 * call/packets were forwarded to a valid iface.
	 */
	(void)zsock_connect(test_sock, (struct sockaddr *)&addr, sizeof(addr));

	ret = k_sem_take(&test_native_send_called, K_MSEC(200));
	zassert_equal(0, ret, "sendto() should've been dispatched to native iface");
}

void test_main(void)
{
	ztest_test_suite(
		socket_offload_dispatcher,
		ztest_unit_test_setup_teardown(test_close_not_bound,
					       test_socket_setup_udp,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_fcntl_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_shutdown_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_bind_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_connect_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_listen_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_accept_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_sendto_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_recvfrom_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_getsockopt_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_setsockopt_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_sendmsg_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_getpeername_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_getsockname_not_bound,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_so_bindtodevice_iface_offloaded,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_so_bindtodevice_iface_native,
					       test_socket_setup_udp,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_tls_native_iface_offloaded,
					       test_socket_setup_tls,
					       test_socket_teardown),
		ztest_unit_test_setup_teardown(test_tls_native_iface_native,
					       test_socket_setup_tls,
					       test_socket_teardown)
	);

	ztest_run_test_suite(socket_offload_dispatcher);
}
