/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_
#define ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>

#include "sx126x_hal.h"
#include "sx126x_regs.h"

/* What the radio is claimed for */
enum sx126x_op {
	SX126X_OP_NONE = 0,
	SX126X_OP_CONFIG,
	SX126X_OP_TX,
	SX126X_OP_CW,
	SX126X_OP_RX,
	SX126X_OP_RX_DUTY_CYCLE,
};

/* A caller blocked on an operation, and where its result goes */
struct sx126x_waiter {
	struct k_sem done;
	int status;
	uint8_t *buf;
	uint8_t size;
	int16_t *rssi;
	int8_t *snr;
};

struct sx126x_data {
	struct sx126x_hal_data hal;

	/* Guards the operation state and every exchange with the radio */
	struct k_mutex lock;
	enum sx126x_op op;

	/* Current configuration */
	struct lora_modem_config config;
	bool config_valid;
	/* Set when the GFSK configuration, rather than the LoRa one,
	 * describes the radio
	 */
	struct lora_modem_config_gfsk gfsk_config;
	bool gfsk;

	struct sx126x_waiter *waiter;

	/* RX data buffer */
	uint8_t rx_buf[SX126X_MAX_PAYLOAD_LEN];

	/* Async RX callback */
	lora_recv_cb rx_cb;
	void *rx_cb_user_data;

	/* RX duty cycle parameters (in SX126x timeout ticks) */
	struct {
		uint32_t rx_period;
		uint32_t sleep_period;
	} duty_cycle;

	/* Async TX signal */
	struct k_poll_signal *tx_async_signal;

	/* Deferred work for interrupt handling */
	struct k_work irq_work;
	const struct device *dev;
};

int sx126x_set_packet_type(const struct device *dev, uint8_t type);
int sx126x_set_rx_gain(const struct device *dev, bool boosted);
int sx126x_config_begin(const struct device *dev);
void sx126x_config_end(const struct device *dev);
int sx126x_config_carrier(const struct device *dev, uint32_t frequency, int8_t tx_power);

#endif /* ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_ */
