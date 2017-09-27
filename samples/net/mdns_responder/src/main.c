/* main.c - mDNS responder */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "mdns-responder"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <net/net_core.h>

/* Note that this application does not do anything itself.
 * It is just a placeholder for waiting mDNS queries.
 */
void main(void)
{
	NET_INFO("Waiting mDNS queries...");
}
