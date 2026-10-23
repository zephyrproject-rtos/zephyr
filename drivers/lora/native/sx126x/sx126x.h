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

enum sx126x_state {
	SX126X_STATE_SLEEP = 0,
	SX126X_STATE_IDLE,
	SX126X_STATE_TX,
	SX126X_STATE_RX,
	SX126X_STATE_RX_DUTY_CYCLE,
	SX126X_STATE_CONFIG,
};

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

	/* Radio state and operation ownership, protected by lock */
	atomic_t state;
	struct k_mutex lock;
	bool sync_pending;

	/* Current configuration */
	struct lora_modem_config config;
	bool config_valid;
	/* Set when the GFSK configuration, rather than the LoRa one,
	 * describes the radio
	 */
	struct lora_modem_config_gfsk gfsk_config;
	bool gfsk;

	/* TX completion via message queue */
	struct k_msgq tx_msgq;
	struct sx126x_tx_result tx_result;

	/* RX completion via message queue */
	struct k_msgq rx_msgq;
	struct sx126x_rx_result rx_result;

	/* RX data buffer (shared between IRQ handler and recv) */
	uint8_t rx_buf[SX126X_MAX_PAYLOAD_LEN];

	/* Async RX callback and generation, protected by lock */
	uint32_t rx_generation;
	lora_recv_cb rx_cb;
	void *rx_cb_user_data;
	/* Thread running rx_cb without the lock, signalled on rx_cb_done */
	k_tid_t rx_cb_thread;
	struct k_condvar rx_cb_done;

	/* RX duty cycle parameters (in SX126x timeout ticks) */
	struct {
		uint32_t rx_period;
		uint32_t sleep_period;
	} duty_cycle;

	/* Async TX signal */
	struct k_poll_signal *tx_async_signal;

	/* Deferred work for interrupt handling */
	atomic_t irq_pending;
	struct k_work irq_work;
	const struct device *dev;
};

int sx126x_set_packet_type(const struct device *dev, uint8_t type);
int sx126x_set_rx_gain(const struct device *dev, bool boosted);
int sx126x_finish(const struct device *dev, int status);
int sx126x_config_begin(const struct device *dev);
int sx126x_config_carrier(const struct device *dev, uint32_t frequency, int8_t tx_power);

#endif /* ZEPHYR_DRIVERS_LORA_SX126X_SX126X_INTERNAL_H_ */
