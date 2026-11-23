/*
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>

#include "lora_radio.h"
#include "sx128x_hal.h"
#include "sx128x_regs.h"

struct sx128x_data {
	/* The common operation state must be first. */
	struct lora_radio_data radio;
	struct sx128x_hal_data hal;

	struct lora_modem_config config;
	struct lora_modem_config_gfsk gfsk_config;
	bool gfsk;

	uint16_t irq_mask;
	bool irq_mask_valid;
	uint8_t packet_params_len;
	bool packet_params_valid;
	bool irq_clean;
	bool aborted;
	/* Uptime until which the timer of a stopped duty cycle may still fire */
	int64_t timer_idle_at;
	uint8_t tx_base;

	struct k_work_delayable irq_poll_work;
};

int sx128x_set_packet_type(const struct device *dev, uint8_t type);
int sx128x_config_carrier(const struct device *dev, uint32_t frequency, int8_t tx_power);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_H_ */
