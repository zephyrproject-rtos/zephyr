/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <string.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <net/socket.h>
#include <sys/util.h>
#include <posix/unistd.h>

#include <ztest_assert.h>

#include "test_socketpair_thread.h"

#undef read
#define read(fd, buf, len) zsock_recv(fd, buf, len, 0)

#undef write
#define write(fd, buf, len) zsock_send(fd, buf, len, 0)

struct ctx {
	/* true if test is write_block(), false if test is read_block() */
	bool write;
	/* the secondary-side socket of the socketpair */
	int fd;
	/* the count of the main thread */
	atomic_t m;
};
static ZTEST_BMEM struct ctx ctx;
static ZTEST_BMEM struct k_work work;

static void work_handler(struct k_work *work)
{
	int res;
	char c = '\0';

	LOG_DBG("doing work");

	while (true) {
		if (ctx.write) {
			LOG_DBG("ctx.m: %u", ctx.m);
			if (atomic_get(&ctx.m)
				< CONFIG_NET_SOCKETPAIR_BUFFER_SIZE) {
				continue;
			}
			LOG_DBG("ready to read!");
		} else {
			LOG_DBG("sleeping for 100ms..");
			k_sleep(K_MSEC(100));
			LOG_DBG("ready to write!");
		}
		break;
	}

	LOG_DBG("%sing 1 byte %s fd %d", ctx.write ? "read" : "writ",
		ctx.write ? "from" : "to", ctx.fd);
	if (ctx.write) {
		res = read(ctx.fd, &c, 1);
	} else {
		res = write(ctx.fd, "x", 1);
	}
	if (-1 == res || 1 != res) {
		LOG_DBG("%s(2) failed: %d", ctx.write ? "read" : "write",
			errno);
	} else {
		LOG_DBG("%s 1 byte", ctx.write ? "read" : "wrote");
	}
}

void test_socketpair_write_block(void)
{
	int res;
	int sv[2] = {-1, -1};

	LOG_DBG("creating socketpair..");
	res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	zassert_equal(res, 0, "socketpair(2) failed: %d", errno);

	for (size_t i = 0; i < 2; ++i) {

		LOG_DBG("data direction %d -> %d", sv[i], sv[(!i) & 1]);

		LOG_DBG("setting up context");
		memset(&ctx, 0, sizeof(ctx));
		ctx.write = true;
		ctx.fd = sv[(!i) & 1];

		LOG_DBG("queueing work");
		k_work_init(&work, work_handler);
		res = k_work_submit_to_user_queue(&test_socketpair_work_q,
			&work);
		zassert_equal(res, 0,
			"k_work_submit_to_user_queue() failed: %d", res);

		/* fill up the buffer */
		for (ctx.m = 0; atomic_get(&ctx.m)
			< CONFIG_NET_SOCKETPAIR_BUFFER_SIZE;) {

			res = write(sv[i], "x", 1);
			zassert_not_equal(res, -1, "write(2) failed: %d",
				errno);
			zassert_equal(res, 1, "wrote %d bytes instead of 1",
				res);

			atomic_inc(&ctx.m);
			LOG_DBG("have written %u bytes", ctx.m);
		}

		/* try to write one more byte */
		LOG_DBG("writing to fd %d", sv[i]);
		res = write(sv[i], "x", 1);
		zassert_not_equal(res, -1, "write(2) failed: %d", errno);
		zassert_equal(res, 1, "wrote %d bytes instead of 1", res);

		LOG_DBG("success!");
	}

	close(sv[0]);
	close(sv[1]);
}

void test_socketpair_read_block(void)
{
	int res;
	char x;
	int sv[2] = {-1, -1};

	LOG_DBG("creating socketpair..");
	res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	zassert_equal(res, 0, "socketpair(2) failed: %d", errno);

	for (size_t i = 0; i < 2; ++i) {

		LOG_DBG("data direction %d <- %d", sv[i], sv[(!i) & 1]);

		LOG_DBG("setting up context");
		memset(&ctx, 0, sizeof(ctx));
		ctx.write = false;
		ctx.fd = sv[(!i) & 1];

		LOG_DBG("queueing work");
		k_work_init(&work, work_handler);
		res = k_work_submit_to_user_queue(&test_socketpair_work_q,
			&work);
		zassert_equal(res, 0,
			"k_work_submit_to_user_queue() failed: %d", res);

		/* try to read one byte */
		LOG_DBG("reading from fd %d", sv[i]);
		x = '\0';
		res = read(sv[i], &x, 1);
		zassert_not_equal(res, -1, "read(2) failed: %d", errno);
		zassert_equal(res, 1, "read %d bytes instead of 1", res);

		LOG_DBG("success!");
	}

	close(sv[0]);
	close(sv[1]);
}
