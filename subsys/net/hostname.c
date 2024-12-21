/** @file
 * @brief Hostname configuration
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_hostname, CONFIG_NET_HOSTNAME_LOG_LEVEL);

#include <zephyr/kernel.h>

#include <zephyr/net/hostname.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/logging/log_backend_net.h>

static char hostname[NET_HOSTNAME_SIZE];

static void trigger_net_event(void)
{
	if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_INFO)) {
		struct net_event_l4_hostname info;

		memcpy(info.hostname, hostname, sizeof(hostname));
		net_mgmt_event_notify_with_info(NET_EVENT_HOSTNAME_CHANGED, NULL,
						&info, sizeof(info));
	} else {
		net_mgmt_event_notify(NET_EVENT_HOSTNAME_CHANGED, NULL);
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_NET)) {
		log_backend_net_hostname_set(hostname, sizeof(hostname));
	}
}

const char *net_hostname_get(void)
{
	return hostname;
}

#if defined(CONFIG_NET_HOSTNAME_DYNAMIC)
int net_hostname_set(char *host, size_t len)
{
	if (len > NET_HOSTNAME_MAX_LEN) {
		return -ENOMEM;
	}

	memcpy(hostname, host, len);
	hostname[len] = 0;

	NET_DBG("New hostname %s", hostname);
	trigger_net_event();

	return 0;
}
#endif

#if defined(CONFIG_NET_HOSTNAME_UNIQUE)
int net_hostname_set_postfix(const uint8_t *hostname_postfix,
			     int postfix_len)
{
#if !defined(CONFIG_NET_HOSTNAME_UNIQUE_UPDATE)
	static bool postfix_set;
#endif
	int pos = 0;
	int i;

#if !defined(CONFIG_NET_HOSTNAME_UNIQUE_UPDATE)
	if (postfix_set) {
		return -EALREADY;
	}
#endif

	NET_ASSERT(postfix_len > 0);

	/* Note that we convert the postfix to hex (2 chars / byte) */
	if ((postfix_len * 2) >
	    (NET_HOSTNAME_MAX_LEN - (sizeof(CONFIG_NET_HOSTNAME) - 1))) {
		return -EMSGSIZE;
	}

	for (i = 0; i < postfix_len; i++, pos += 2) {
		snprintk(&hostname[sizeof(CONFIG_NET_HOSTNAME) - 1 + pos], 2 + 1, "%02x",
			 hostname_postfix[i]);
	}

	NET_DBG("New hostname %s", hostname);

#if !defined(CONFIG_NET_HOSTNAME_UNIQUE_UPDATE)
	postfix_set = true;
#endif
	trigger_net_event();

	return 0;
}

int net_hostname_set_postfix_str(const uint8_t *hostname_postfix,
			     int postfix_len)
{
#if !defined(CONFIG_NET_HOSTNAME_UNIQUE_UPDATE)
	static bool postfix_set;
#endif
	int net_hostname_len = sizeof(CONFIG_NET_HOSTNAME) - 1;
	int hostname_len_remain = (NET_HOSTNAME_MAX_LEN - net_hostname_len) + 1;

#if !defined(CONFIG_NET_HOSTNAME_UNIQUE_UPDATE)
	if (postfix_set) {
		return -EALREADY;
	}
#endif

	NET_ASSERT(postfix_len > 0);

	if (hostname_len_remain < postfix_len) {
		NET_DBG("Hostname postfix length %d is exceeding limit of %d", postfix_len,
			hostname_len_remain);
		return -EMSGSIZE;
	}

	snprintk(&hostname[net_hostname_len], hostname_len_remain, "%s", hostname_postfix);

	NET_DBG("New Unique hostname: %s", hostname);

#if !defined(CONFIG_NET_HOSTNAME_UNIQUE_UPDATE)
	postfix_set = true;
#endif
	trigger_net_event();

	return 0;
}
#endif /* CONFIG_NET_HOSTNAME_UNIQUE */

void net_hostname_init(void)
{
	memcpy(hostname, CONFIG_NET_HOSTNAME, sizeof(CONFIG_NET_HOSTNAME) - 1);

	NET_DBG("Hostname set to %s", CONFIG_NET_HOSTNAME);
	trigger_net_event();
}
