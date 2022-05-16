/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_IEEE802154_CC1200_H_
#define ZEPHYR_INCLUDE_DRIVERS_IEEE802154_CC1200_H_

#include <zephyr/device.h>

/* RF settings
 *
 * First 42 entries are for the 42 first registers from
 * address 0x04 to 0x2D included.
 * Next, the last 58 entries are for the 58 registers from
 * extended address 0x00 to 0x39 included
 *
 * If CONFIG_IEEE802154_CC1200_RF_PRESET is not used, one will need
 * to provide 'cc1200_rf_settings' with proper settings. These can
 * be generated through TI's SmartRF application.
 *
 */
struct cc1200_rf_registers_set {
	uint32_t chan_center_freq0;
	uint16_t channel_limit;
	/* to fit in uint16_t, spacing is a multiple of 100 Hz,
	 * 12.5KHz for instance will be 125.
	 */
	uint16_t channel_spacing;
	uint8_t registers[100];
};

#ifndef CONFIG_IEEE802154_CC1200_RF_PRESET
extern const struct cc1200_rf_registers_set cc1200_rf_settings;
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_IEEE802154_CC1200_H_ */
