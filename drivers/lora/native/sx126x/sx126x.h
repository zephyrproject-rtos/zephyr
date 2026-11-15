/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_
#define ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>

#include "lora_radio.h"
#include "sx126x_hal.h"
#include "sx126x_regs.h"

struct sx126x_data {
	/* Must be first */
	struct lora_radio_data common;

	struct sx126x_hal_data hal;

	/* Current configuration */
	struct lora_modem_config config;
	/* Set when the GFSK configuration, rather than the LoRa one,
	 * describes the radio
	 */
	struct lora_modem_config_gfsk gfsk_config;
	bool gfsk;
};

int sx126x_set_packet_type(const struct device *dev, uint8_t type);
int sx126x_set_rx_gain(const struct device *dev, bool boosted);
int sx126x_config_carrier(const struct device *dev, uint32_t frequency, int8_t tx_power);

#endif /* ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_ */
