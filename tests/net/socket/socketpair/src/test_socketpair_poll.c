/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <string.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <net/socket.h>
#include <posix/unistd.h>
#include <sys/util.h>

#include <ztest_assert.h>

#undef read
#define read(fd, buf, len) zsock_recv(fd, buf, len, 0)

#undef write
#define write(fd, buf, len) zsock_send(fd, buf, len, 0)

#define STACK_SIZE 512
/* stack for the secondary thread */
static K_THREAD_STACK_DEFINE(th_stack, STACK_SIZE);
static struct k_thread th;
static k_tid_t tid;

/*
 * Timeout should work the same for blocking & non-blocking threads
 *
 *   - no bytes available to read after timeout, r: 0 (timeout)
 *   - no bytes available to write after timeout, r: 0 (timeout)
 */

static void test_socketpair_poll_timeout_common(int sv[2])
{
	int res;

	struct pollfd fds[1];

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sv[0];
	fds[0].events |= POLLIN;
	res = poll(fds, 1, 1);
	zassert_equal(res, 0, "poll: expected: 0 actual: %d", res);

	for (size_t i = 0; i < CONFIG_NET_SOCKETPAIR_BUFFER_SIZE; ++i) {
		res = write(sv[0], "x", 1);
		zassert_equal(res, 1, "write failed: %d", res);
	}

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sv[0];
	fds[0].events |= POLLOUT;
	res = poll(fds, 1, 1);
	zassert_equal(res, 0, "poll: expected: 0 actual: %d", res);

	close(sv[0]);
	close(sv[1]);
}

void test_socketpair_poll_timeout(void)
{
	int sv[2] = {-1, -1};
	int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

	zassert_not_equal(res, -1, "socketpair failed: %d", errno);

	test_socketpair_poll_timeout_common(sv);
}

/* O_NONBLOCK should have no affect on poll(2) */
void test_socketpair_poll_timeout_nonblocking(void)
{
	int sv[2] = {-1, -1};
	int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

	zassert_not_equal(res, -1, "socketpair failed: %d", errno);

	res = fcntl(sv[0], F_GETFL, 0);
	zassert_not_equal(res, -1, "fcntl failed: %d", errno);

	int flags = res;

	res = fcntl(sv[0], F_SETFL, O_NONBLOCK | flags);
	zassert_not_equal(res, -1, "fcntl failed: %d", errno);

	res = fcntl(sv[1], F_SETFL, O_NONBLOCK | flags);
	zassert_not_equal(res, -1, "fcntl failed: %d", errno);

	test_socketpair_poll_timeout_common(sv);
}

static void close_fun(void *arg1, void *arg2, void *arg3)
{
	(void)arg2;
	(void)arg3;

	const int *const fd = (const int *)arg1;

	k_sleep(K_MSEC(1000));

	LOG_DBG("about to close fd %d", *fd);
	close(*fd);
}

/*
 * Hangup should cause the following behaviour
 *   - close remote fd while the local fd is blocking in poll. r: 1,
 *     POLLIN, read -> r: 0, errno: 0 -> EOF
 *   - close remote fd while the local fd is blocking in poll. r: 1,
 *     POLLOUT, write -> r: -1, errno: EPIPE.
 */
void test_socketpair_poll_close_remote_end_POLLIN(void)
{
	int res;
	char c;
	struct pollfd fds[1];

	int sv[2] = {-1, -1};

	res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	zassert_not_equal(res, -1, "socketpair(2) failed: %d", errno);

	/*
	 * poll until there are bytes to read.
	 * But rather than writing, close the other end of the channel
	 */

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sv[0];
	fds[0].events |= POLLIN;

	tid = k_thread_create(&th, th_stack,
		K_THREAD_STACK_SIZEOF(th_stack), close_fun,
		&sv[1], NULL, NULL,
		CONFIG_MAIN_THREAD_PRIORITY + 1,
		K_USER, K_NO_WAIT);

	res = poll(fds, 1, -1);
	zassert_equal(res, 1, "poll(2) failed: %d", res);
	zassert_equal(fds[0].revents & POLLIN, POLLIN, "POLLIN not set");

	res = k_thread_join(&th, K_MSEC(5000));
	zassert_false(res < 0, "k_thread_join failed: %d", res);

	res = read(sv[0], &c, 1);
	zassert_equal(res, 0, "read did not return EOF");

	close(sv[0]);
}

void test_socketpair_poll_close_remote_end_POLLOUT(void)
{
	int res;
	struct pollfd fds[1];

	int sv[2] = {-1, -1};

	/*
	 * Fill up the remote q and then poll until write space is available.
	 * But rather than reading, close the other end of the channel
	 */

	res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	zassert_not_equal(res, -1, "socketpair(2) failed: %d", errno);

	for (size_t i = 0; i < CONFIG_NET_SOCKETPAIR_BUFFER_SIZE; ++i) {
		res = write(sv[0], "x", 1);
		zassert_equal(res, 1, "write failed: %d", res);
	}

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sv[0];
	fds[0].events |= POLLOUT;

	tid = k_thread_create(&th, th_stack,
		K_THREAD_STACK_SIZEOF(th_stack), close_fun,
		&sv[1], NULL, NULL,
		CONFIG_MAIN_THREAD_PRIORITY + 1,
		K_USER, K_NO_WAIT);

	res = poll(fds, 1, -1);
	zassert_equal(res, 1, "poll(2) failed: %d", res);
	zassert_equal(fds[0].revents & POLLHUP, POLLHUP, "POLLHUP not set");

	res = k_thread_join(&th, K_MSEC(5000));
	zassert_false(res < 0, "k_thread_join failed: %d", res);

	res = write(sv[0], "x", 1);
	zassert_equal(res, -1, "write(2): expected: -1 actual: %d", res);
	zassert_equal(errno, EPIPE, "errno: expected: EPIPE actual: %d", errno);

	close(sv[0]);
}

/*
 * Data available immediately
 *   - even with a timeout value of 0 us, poll should return immediately with
 *     a value of 1 (for either read or write cases)
 *   - even with a timeout value of 0us, poll should return immediately with
 *     a value of 2 if both read and write are available
 */
void test_socketpair_poll_immediate_data(void)
{
	int sv[2] = {-1, -1};
	int res;

	struct pollfd fds[2];

	res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	zassert_not_equal(res, -1, "socketpair(2) failed: %d", errno);

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sv[0];
	fds[0].events |= POLLOUT;
	res = poll(fds, 1, 0);
	zassert_not_equal(res, -1, "poll(2) failed: %d", errno);
	zassert_equal(res, 1, "poll(2): expected: 1 actual: %d", res);
	zassert_not_equal(fds[0].revents & POLLOUT, 0, "POLLOUT not set");

	res = write(sv[0], "x", 1);
	zassert_not_equal(res, -1, "write(2) failed: %d", errno);
	zassert_equal(res, 1, "write(2): expected: 1 actual: %d", res);

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sv[1];
	fds[0].events |= POLLIN;
	res = poll(fds, 1, 0);
	zassert_not_equal(res, -1, "poll(2) failed: %d", errno);
	zassert_equal(res, 1, "poll(2): expected: 1 actual: %d", res);
	zassert_not_equal(fds[0].revents & POLLIN, 0, "POLLIN not set");

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sv[0];
	fds[0].events |= POLLOUT;
	fds[1].fd = sv[1];
	fds[1].events |= POLLIN;
	res = poll(fds, 2, 0);
	zassert_not_equal(res, -1, "poll(2) failed: %d", errno);
	zassert_equal(res, 2, "poll(2): expected: 1 actual: %d", res);
	zassert_not_equal(fds[0].revents & POLLOUT, 0, "POLLOUT not set");
	zassert_not_equal(fds[1].revents & POLLIN, 0, "POLLIN not set");

	close(sv[0]);
	close(sv[1]);
}

static void rw_fun(void *arg1, void *arg2, void *arg3)
{
	(void)arg3;

	const bool *const should_write = (const bool *) arg1;
	const int *const fd = (const int *)arg2;

	int res;
	char c;

	k_sleep(K_MSEC(1000));

	if (*should_write) {
		LOG_DBG("about to write 1 byte");
		res = write(*fd, "x", 1);
		if (-1 == res) {
			LOG_DBG("write(2) failed: %d", errno);
		} else {
			LOG_DBG("wrote 1 byte");
		}
	} else {
		LOG_DBG("about to read 1 byte");
		res = read(*fd, &c, 1);
		if (-1 == res) {
			LOG_DBG("read(2) failed: %d", errno);
		} else {
			LOG_DBG("read 1 byte");
		}
	}
}

/*
 * Data only available but after some short period
 *   - say there is a timeout value of 5 s, poll should return immediately
 *     with the a value of 1 (for either read or write cases)
 */
void test_socketpair_poll_delayed_data(void)
{
	int sv[2] = {-1, -1};
	int res;

	bool should_write;

	struct pollfd fds[1];

	res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	zassert_not_equal(res, -1, "socketpair(2) failed: %d", errno);

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sv[0];
	fds[0].events |= POLLIN;
	should_write = true;

	tid = k_thread_create(&th, th_stack,
		K_THREAD_STACK_SIZEOF(th_stack), rw_fun,
		&should_write, &sv[1], NULL,
		CONFIG_MAIN_THREAD_PRIORITY + 1,
		K_USER, K_NO_WAIT);

	res = poll(fds, 1, 5000);
	zassert_not_equal(res, -1, "poll(2) failed: %d", errno);
	zassert_equal(res, 1, "poll(2): expected: 1 actual: %d", res);
	zassert_not_equal(fds[0].revents & POLLIN, 0, "POLLIN not set");
	k_thread_join(&th, K_FOREVER);

	for (size_t i = 0; i < CONFIG_NET_SOCKETPAIR_BUFFER_SIZE; ++i) {
		res = write(sv[0], "x", 1);
		zassert_equal(res, 1, "write failed: %d", res);
	}

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sv[0];
	fds[0].events |= POLLOUT;
	should_write = false;

	tid = k_thread_create(&th, th_stack,
		K_THREAD_STACK_SIZEOF(th_stack), rw_fun,
		&should_write, &sv[1], NULL,
		CONFIG_MAIN_THREAD_PRIORITY + 1,
		K_USER, K_NO_WAIT);

	res = poll(fds, 1, 5000);
	zassert_not_equal(res, -1, "poll(2) failed: %d", errno);
	zassert_equal(res, 1, "poll(2): expected: 1 actual: %d", res);
	zassert_not_equal(fds[0].revents & POLLOUT, 0, "POLLOUT was not set");
	k_thread_join(&th, K_FOREVER);

	close(sv[0]);
	close(sv[1]);
}
