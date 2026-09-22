/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_ip.h>

int inet_pton(int family, const char *src, void *dst)
{
	return net_addr_pton(family, src, dst) == 0 ? 1 : 0;
}
