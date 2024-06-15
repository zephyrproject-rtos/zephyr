/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <arpa/inet.h>
#else
#include <zephyr/posix/arpa/inet.h>
#endif

/**
 * @brief existence test for `<arpa/inet.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/arpa_inet.h.html">arpa/inet.h</a>
 */
ZTEST(posix_headers, test_arpa_inet_h)
{
	zassert_not_equal(-1, htonl(0));
	zassert_not_equal(-1, htons(0));
	zassert_not_equal(-1, ntohl(0));
	zassert_not_equal(-1, ntohs(0));

	if (IS_ENABLED(CONFIG_POSIX_NETWORKING)) {
		/* zassert_not_null(inet_addr); */ /* not implemented */
		/* zassert_not_null(inet_ntoa); */ /* not implemented */
		zassert_not_null(inet_ntop);
		zassert_not_null(inet_pton);
	}
}
