/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_LR11XX_LR11XX_INTERNAL_H_
#define ZEPHYR_DRIVERS_LORA_LR11XX_LR11XX_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/atomic.h>

#include "lr11xx_hal.h"
#include "lr11xx_regs.h"

enum lr11xx_state {
	LR11XX_STATE_SLEEP = 0,
	LR11XX_STATE_IDLE,
	LR11XX_STATE_TX,
	LR11XX_STATE_RX,
	LR11XX_STATE_RX_DUTY_CYCLE,
};

struct lr11xx_tx_result {
	int status;
};

struct lr11xx_rx_result {
	int16_t rssi;
	int8_t snr;
	uint8_t len;
	int status;
};

struct lr11xx_data {
	struct lr11xx_hal_data hal;

	/* Current state (atomic for lock-free state transitions) */
	atomic_t state;
	struct k_mutex lock;

	/* Current configuration */
	struct lora_modem_config config;
	bool config_valid;

	/* TX completion via message queue */
	struct k_msgq tx_msgq;
	struct lr11xx_tx_result tx_result;

	/* RX completion via message queue */
	struct k_msgq rx_msgq;
	struct lr11xx_rx_result rx_result;

	/* RX data buffer (shared between IRQ handler and recv) */
	uint8_t rx_buf[LR11XX_MAX_PAYLOAD_LEN];

	/* Async RX callback */
	lora_recv_cb rx_cb;
	void *rx_cb_user_data;

	/* RX duty cycle parameters (in LR11xx timeout ticks) */
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

#endif /* ZEPHYR_DRIVERS_LORA_LR11XX_LR11XX_INTERNAL_H_ */
