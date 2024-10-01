/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <net/if.h>
#else
#include <zephyr/posix/net/if.h>
#endif

/**
 * @brief existence test for `<net/if.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/net_if.h.html">net/if.h</a>
 */
ZTEST(posix_headers, test_net_if_h)
{
	zassert_not_equal(-1, offsetof(struct if_nameindex, if_index));
	zassert_not_equal(-1, offsetof(struct if_nameindex, if_name));

	zassert_not_equal(-1, IF_NAMESIZE);

	if (IS_ENABLED(CONFIG_POSIX_NETWORKING)) {
		zassert_not_null(if_freenameindex);
		zassert_not_null(if_indextoname);
		zassert_not_null(if_nameindex);
		zassert_not_null(if_nametoindex);
	}
}
