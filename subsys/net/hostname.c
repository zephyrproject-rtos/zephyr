/** @file
 * @brief Hostname configuration
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_HOSTNAME)
#define SYS_LOG_DOMAIN "net/hostname"
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>

#include <net/net_core.h>

#if defined(CONFIG_NET_HOSTNAME_UNIQUE)
/* Allocate extra space to append MAC address to hostname */
#define EXTRA_SPACE (8 * 2)
#else
#define EXTRA_SPACE 0
#endif /* CONFIG_NET_HOSTNAME_UNIQUE */

static char hostname[sizeof(CONFIG_NET_HOSTNAME) + EXTRA_SPACE];

const char *net_hostname_get(void)
{
	return hostname;
}

#if defined(CONFIG_NET_HOSTNAME_UNIQUE)
int net_hostname_set_postfix(const u8_t *hostname_postfix,
			     int postfix_len)
{
	static bool postfix_set;
	int pos = 0;
	int i;

	if (postfix_set) {
		return -EALREADY;
	}

	NET_ASSERT(postfix_len > 0);

	/* Note that we convert the postfix to hex (2 chars / byte) */
	if ((postfix_len * 2) >
	    ((sizeof(hostname) - 1) - (sizeof(CONFIG_NET_HOSTNAME) - 1))) {
		return -EMSGSIZE;
	}

	for (i = 0; i < postfix_len; i++, pos += 2) {
		snprintk(&hostname[sizeof(CONFIG_NET_HOSTNAME) - 1 + pos],
			 2 + 1, "%02x", hostname_postfix[i]);
	}

	NET_DBG("New hostname %s", hostname);

	postfix_set = true;

	return 0;
}
#endif /* CONFIG_NET_HOSTNAME_UNIQUE */

void net_hostname_init(void)
{
	memcpy(hostname, CONFIG_NET_HOSTNAME, sizeof(CONFIG_NET_HOSTNAME) - 1);

	NET_DBG("Hostname set to %s", hostname);
}
