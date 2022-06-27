/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <string.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/util.h>
#include <zephyr/posix/unistd.h>

#include <ztest_assert.h>

#undef read
#define read(fd, buf, len) zsock_recv(fd, buf, len, 0)

#undef write
#define write(fd, buf, len) zsock_send(fd, buf, len, 0)

void test_socketpair_unsupported_calls(void)
{
	int res;
	int sv[2] = {-1, -1};
	struct sockaddr_un addr = {
		.sun_family = AF_UNIX,
	};
	socklen_t len = sizeof(addr);

	res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	zassert_equal(res, 0,
		"socketpair(AF_UNIX, SOCK_STREAM, 0, sv) failed");


	for (size_t i = 0; i < 2; ++i) {

		res = bind(sv[i], (struct sockaddr *)&addr, len);
		zassert_equal(res, -1,
			"bind should fail on a socketpair endpoint");
		zassert_equal(errno, EISCONN,
			"bind should set errno to EISCONN");

		res = connect(sv[i], (struct sockaddr *)&addr, len);
		zassert_equal(res, -1,
			"connect should fail on a socketpair endpoint");
		zassert_equal(errno, EISCONN,
			"connect should set errno to EISCONN");

		res = listen(sv[i], 1);
		zassert_equal(res, -1,
			"listen should fail on a socketpair endpoint");
		zassert_equal(errno, EINVAL,
			"listen should set errno to EINVAL");

		res = accept(sv[i], (struct sockaddr *)&addr, &len);
		zassert_equal(res, -1,
			"accept should fail on a socketpair endpoint");
		zassert_equal(errno, EOPNOTSUPP,
			"accept should set errno to EOPNOTSUPP");
	}

	res = close(sv[0]);
	zassert_equal(res, 0, "close failed");

	res = close(sv[1]);
	zassert_equal(res, 0, "close failed");
}
