/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC1101_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC1101_H_


struct cc1101_context {
	struct net_if *iface;
	uint8_t mac_addr[8];
	const struct device rf_dev;
};


#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_CC1101_H_ */
