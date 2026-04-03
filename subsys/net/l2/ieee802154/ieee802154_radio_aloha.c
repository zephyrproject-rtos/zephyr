/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * All references to the spec refer to IEEE 802.15.4-2020.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_aloha, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <zephyr/net/net_if.h>

#include "ieee802154_priv.h"

static inline int aloha_channel_access(struct net_if *iface)
{
	ARG_UNUSED(iface);

	/* CCA Mode 4: ALOHA. CCA shall always report an idle medium, see section 10.2.8. */
	return 0;
}

/* Declare the public channel access algorithm function used by L2. */
FUNC_ALIAS(aloha_channel_access, ieee802154_wait_for_clear_channel, int);
