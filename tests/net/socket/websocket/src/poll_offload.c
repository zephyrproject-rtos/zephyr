/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/ztest.h>

/* Transport mock whose poll_prepare returns -EXDEV, as an offloaded socket's
 * does. poll() on a websocket registered over it goes through the websocket
 * poll offload wrapper, which then calls poll_offload here.
 */

static bool offload_poll_called;
static int offload_poll_nfds;
static int ws_sock = -1;

static int offload_close(void *obj, int fd)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(fd);

	return 0;
}

static int offload_poll_prepare(void *obj, struct zvfs_pollfd *pfd, struct k_poll_event **pev,
				struct k_poll_event *pev_end)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(pfd);
	ARG_UNUSED(pev);
	ARG_UNUSED(pev_end);

	return -EXDEV;
}

static int offload_poll_offload(void *obj, struct zvfs_pollfd *fds, int nfds, int timeout)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(fds);
	ARG_UNUSED(timeout);

	offload_poll_nfds = nfds;
	offload_poll_called = true;

	return 0;
}

/* Accept the CLOSE frame sent when the websocket is closed. */
static ssize_t offload_sendmsg(void *obj, const struct net_msghdr *msg, int flags)
{
	ssize_t len = 0;

	ARG_UNUSED(obj);
	ARG_UNUSED(flags);

	for (size_t i = 0; i < msg->msg_iovlen; i++) {
		len += msg->msg_iov[i].iov_len;
	}

	return len;
}

static const struct socket_op_vtable offload_fd_op_vtable = {
	.fd_vtable = {
			.close2 = offload_close,
			.poll_prepare = offload_poll_prepare,
			.poll_offload = offload_poll_offload,
		},
	.sendmsg = offload_sendmsg,
};

static void prepare_fds(struct zsock_pollfd *fds, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		fds[i].fd = -1;
		fds[i].events = 0;
		fds[i].revents = 0;
	}

	fds[0].fd = ws_sock;
	fds[0].events = ZSOCK_POLLOUT;
}

/* Up to CONFIG_ZVFS_POLL_MAX entries reach the offloaded poll. */
ZTEST(net_websocket_poll_offload, test_poll_max_nfds)
{
	struct zsock_pollfd fds[CONFIG_ZVFS_POLL_MAX];
	int ret;

	prepare_fds(fds, ARRAY_SIZE(fds));

	ret = zsock_poll(fds, ARRAY_SIZE(fds), 0);
	zassert_equal(ret, 0, "poll() failed (%d, errno %d)", ret, errno);
	zassert_true(offload_poll_called, "Offloaded poll was not called");
	zassert_equal(offload_poll_nfds, ARRAY_SIZE(fds), "Wrong nfds passed to offload");
	zassert_equal(fds[0].fd, ws_sock, "Descriptor was not restored");
}

/* One entry more than CONFIG_ZVFS_POLL_MAX fails with EINVAL before the
 * offloaded poll is called or the caller's descriptors are touched.
 */
ZTEST(net_websocket_poll_offload, test_poll_nfds_too_large)
{
	struct zsock_pollfd fds[CONFIG_ZVFS_POLL_MAX + 1];
	int ret;

	prepare_fds(fds, ARRAY_SIZE(fds));

	ret = zsock_poll(fds, ARRAY_SIZE(fds), 0);
	zassert_equal(ret, -1, "poll() should fail");
	zassert_equal(errno, EINVAL, "Wrong errno (%d)", errno);
	zassert_false(offload_poll_called, "Offloaded poll should not be called");
	zassert_equal(fds[0].fd, ws_sock, "Descriptor was modified");
}

static void poll_offload_before(void *fixture)
{
	static int offload_obj;
	static uint8_t recv_buf[32];
	int sock;

	ARG_UNUSED(fixture);

	offload_poll_called = false;
	offload_poll_nfds = 0;

	sock = zvfs_reserve_fd();
	zassert_true(sock >= 0, "zvfs_reserve_fd() failed (%d)", sock);

	zvfs_finalize_typed_fd(sock, &offload_obj,
			       (const struct fd_op_vtable *)&offload_fd_op_vtable,
			       ZVFS_MODE_IFSOCK);

	ws_sock = websocket_register(sock, recv_buf, sizeof(recv_buf));
	if (ws_sock < 0) {
		(void)zsock_close(sock);
	}

	zassert_true(ws_sock >= 0, "websocket_register() failed (%d)", ws_sock);
}

static void poll_offload_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (ws_sock >= 0) {
		(void)websocket_unregister(ws_sock);
		ws_sock = -1;
	}
}

ZTEST_SUITE(net_websocket_poll_offload, NULL, NULL, poll_offload_before, poll_offload_after,
	    NULL);
