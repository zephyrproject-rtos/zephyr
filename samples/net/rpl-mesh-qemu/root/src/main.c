/*
 * Copyright (c) 2017 CPqD Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <stdlib.h>
#include "rpl.h"

#define CURRENT_VERSION 0

void main(void)
{
	u8_t init_version = CURRENT_VERSION;

	char prefix_str[NET_IPV6_ADDR_LEN + 1];
	struct in6_addr prefix;
	u8_t prefix_len;

	printk("RPL border router starting\n");

	struct net_if *iface = net_if_get_default();

	if (!iface) {
		printk("Interface is NULL\n");
		printk("Failed to setup RPL root node\n");
		return;
	}

	/* Read prefix from KConfig and parse it */
	(void)memset(prefix_str, 0, sizeof(prefix_str));
	memcpy(prefix_str, CONFIG_NET_RPL_PREFIX,
			min(strlen(CONFIG_NET_RPL_PREFIX), NET_IPV6_ADDR_LEN));

	char *slash = strstr(prefix_str, "/");

	if (!slash) {
		prefix_len = 64;
	} else {
		*slash = '\0';
		prefix_len = atoi(slash + 1);
	}

	if (prefix_len == 0) {
		printk("Invalid prefix length %s", slash + 1);
		return;
	}

	if (net_addr_pton(AF_INET6, prefix_str, &prefix) < 0) {
		printk("Invalid IPv6 prefix %s", prefix_str);
		return;
	}

	/* Check the current version */
	if (CURRENT_VERSION == 0) {
		/* case 0 - call init */
		init_version = net_rpl_lollipop_init();
	} else if (CURRENT_VERSION > 0) {
		/* case > 0 - increment */
		net_rpl_lollipop_increment(&init_version);
	} else {
		printk("CURRENT_VERSION should be greater or eqaul to 0");
		return;
	}

	/* Setup the root node */
	struct net_rpl_dag *dag = net_rpl_set_root_with_version(
		iface, CONFIG_NET_RPL_DEFAULT_INSTANCE, &prefix, init_version);

	if (!dag) {
		printk("Cannot set root node");
		return;
	}

	bool ret = net_rpl_set_prefix(iface, dag, &prefix, prefix_len);

	if (!ret) {
		printk("Cannot set prefix %s/%d", prefix_str, prefix_len);
		return;
	}
}
