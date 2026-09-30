/*
 * Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_H_
#define ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/atomic.h>

#include "sx128x_hal.h"
#include "sx128x_regs.h"

enum sx128x_state {
	SX128X_STATE_STANDBY = 0,
	SX128X_STATE_TX,
	SX128X_STATE_RX,
	SX128X_STATE_CAD,
	SX128X_STATE_CONFIG,
};

struct sx128x_tx_result {
	int status;
};

struct sx128x_rx_result {
	int16_t rssi;
	int8_t snr;
	uint8_t len;
	int status;
	uint8_t payload[SX128X_MAX_PAYLOAD_LEN];
};

struct sx128x_data {
	struct sx128x_hal_data hal;

	atomic_t state;
	struct k_mutex lock;

	struct lora_modem_config config;
	bool config_valid;
	/* true after lora_config_gfsk(), false after lora_config() */
	struct lora_modem_config_gfsk gfsk_config;
	bool gfsk;

	/* What the radio was last told, so that back-to-back operations of
	 * the same kind skip the commands that would not change anything
	 */
	uint16_t irq_mask;
	bool irq_mask_valid;
	uint8_t packet_params_len;
	bool packet_params_valid;
	/* No interrupt left latched by an operation that did not finish */
	bool irq_clean;

	struct k_poll_signal *tx_signal;
	/* Chip buffer offset the frame on air was sent from, and its length */
	uint8_t tx_base;
	uint8_t tx_len;
	/* Frame taken by lora_send_async() while another one is on air, sent
	 * as soon as that one completes. When both fit in half of the chip
	 * buffer it is already written to the other half.
	 */
	struct {
		bool pending;
		bool in_chip;
		uint8_t len;
		uint8_t base;
		struct k_poll_signal *signal;
		uint8_t buf[SX128X_MAX_PAYLOAD_LEN];
	} tx_next;
	lora_recv_cb recv_cb;
	void *recv_user_data;
	bool rx_duty_cycle;
	uint8_t rx_duty_params[5];
	lora_cad_cb cad_cb;
	void *cad_user_data;
	struct k_sem cad_done;
	bool cad_detected;

	struct k_msgq tx_msgq;
	struct sx128x_tx_result tx_result;

	struct k_msgq rx_msgq;
	struct sx128x_rx_result rx_result;

	struct k_work irq_work;
	struct k_work_delayable irq_poll_work;
	struct k_work_q *irq_workq;
	const struct device *dev;
};

int sx128x_set_packet_type(const struct device *dev, uint8_t type);
int sx128x_config_begin(const struct device *dev);
void sx128x_config_end(const struct device *dev);
int sx128x_config_carrier(const struct device *dev, uint32_t frequency, int8_t tx_power);

#endif /* ZEPHYR_DRIVERS_LORA_NATIVE_SX128X_H_ */
