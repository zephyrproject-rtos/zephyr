/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CC1101_H__
#define __CC1101_H__

#include <device.h>

/* RF settings
 *
 * First 42 entries are for the 42 first registers from
 * address 0x04 to 0x2D included.
 * Next, the last 58 entries are for the 58 registers from
 * extended address 0x00 to 0x39 included
 *
 * If CONFIG_IEEE802154_CC1101_RF_PRESET is not used, one will need
 * no provide 'cc1101_rf_settings' with proper settings. These can
 * be generated through TI's SmartRF application.
 *
 */
struct cc1101_rf_registers_set {
	u16_t channel_limit;
	u8_t registers[44];
};

#ifndef CONFIG_IEEE802154_CC1101_RF_PRESET
extern const struct cc1101_rf_registers_set cc1101_rf_settings;
#endif

enum cc1101_gpio_index {
	CC1101_GPIO_IDX_GPIO0,
	CC1101_GPIO_IDX_GPIO1,
	CC1101_GPIO_IDX_MAX,
};

struct cc1101_gpio_configuration {
	struct device *dev;
	u32_t pin;
};

struct cc1101_gpio_configuration *cc1101_configure_gpios(void);

#endif /* __CC1101_H__ */
