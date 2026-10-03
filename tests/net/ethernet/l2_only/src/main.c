/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Axonne Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_ip.h>
#include <zephyr/sys/util.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_NET_L2_ETHERNET));
BUILD_ASSERT(!IS_ENABLED(CONFIG_NET_IP));
BUILD_ASSERT(!IS_ENABLED(CONFIG_NET_CONTEXT));

int main(void)
{
	return 0;
}
