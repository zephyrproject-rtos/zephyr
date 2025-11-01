/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#include <netinet/tcp.h>

/**
 * @brief existence test for `<netinet/tcp.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/netinet_tcp.h.html">netinet/tcp.h</a>
 */
ZTEST(posix_headers, test_netinet_tcp_h)
{
	zassert_not_equal(-1, TCP_NODELAY);
}
