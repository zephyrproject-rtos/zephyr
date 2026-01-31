/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_
#define ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/atomic.h>

#include "sx126x_hal.h"
#include "sx126x_regs.h"

#define SX126X_STATE_IDLE 0
#define SX126X_STATE_TX   1
#define SX126X_STATE_RX   2

struct sx126x_tx_result {
	int status;
};

struct sx126x_rx_result {
	int16_t rssi;
	int8_t snr;
	uint8_t len;
	int status;
};

struct sx126x_data {
	struct sx126x_hal_data hal;

	/* Current state (atomic for lock-free state transitions) */
	atomic_t state;
	struct k_mutex lock;

	/* Current configuration */
	struct lora_modem_config config;
	bool config_valid;

	/* TX completion via message queue */
	struct k_msgq tx_msgq;
	struct sx126x_tx_result tx_result;

	/* RX completion via message queue */
	struct k_msgq rx_msgq;
	struct sx126x_rx_result rx_result;

	/* RX data buffer (shared between IRQ handler and recv) */
	uint8_t rx_buf[SX126X_MAX_PAYLOAD_LEN];

	/* Async RX callback */
	lora_recv_cb rx_cb;
	void *rx_cb_user_data;

	/* Async TX signal */
	struct k_poll_signal *tx_async_signal;

	/* Deferred work for interrupt handling */
	struct k_work irq_work;
	const struct device *dev;
};

#endif /* ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_ */
