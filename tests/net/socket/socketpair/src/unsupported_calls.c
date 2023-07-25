/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

ZTEST_USER(net_socketpair, test_unsupported_calls)
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
