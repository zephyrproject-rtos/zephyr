/* IEEE 802.15.4 settings header */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/ieee802154_mgmt.h>

#if defined(CONFIG_NET_L2_IEEE802154) && defined(CONFIG_NET_CONFIG_SETTINGS)
struct net_if;

int z_net_config_ieee802154_setup(struct net_if *iface,
				  uint16_t channel,
				  uint16_t pan_id,
				  int16_t tx_power,
				  struct ieee802154_security_params *sec_params);
#else
#define z_net_config_ieee802154_setup(...) 0
#endif
