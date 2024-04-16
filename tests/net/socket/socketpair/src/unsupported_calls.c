/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

ZTEST_USER_F(net_socketpair, test_unsupported_calls)
{
	int res;
	struct sockaddr_un addr = {
		.sun_family = AF_UNIX,
	};
	socklen_t len = sizeof(addr);

	for (size_t i = 0; i < 2; ++i) {

		res = bind(fixture->sv[i], (struct sockaddr *)&addr, len);
		zassert_equal(res, -1,
			"bind should fail on a socketpair endpoint");
		zassert_equal(errno, EISCONN,
			"bind should set errno to EISCONN");

		res = connect(fixture->sv[i], (struct sockaddr *)&addr, len);
		zassert_equal(res, -1,
			"connect should fail on a socketpair endpoint");
		zassert_equal(errno, EISCONN,
			"connect should set errno to EISCONN");

		res = listen(fixture->sv[i], 1);
		zassert_equal(res, -1,
			"listen should fail on a socketpair endpoint");
		zassert_equal(errno, EINVAL,
			"listen should set errno to EINVAL");

		res = accept(fixture->sv[i], (struct sockaddr *)&addr, &len);
		zassert_equal(res, -1,
			"accept should fail on a socketpair endpoint");
		zassert_equal(errno, EOPNOTSUPP,
			"accept should set errno to EOPNOTSUPP");
	}
}
