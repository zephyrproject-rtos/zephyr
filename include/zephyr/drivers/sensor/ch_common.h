/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_SENSOR_CH_COMMON_H
#define ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_SENSOR_CH_COMMON_H

/**
 * Sensor operating modes
 */
enum ch_mode {
	/**
	 * @brief Idle mode
	 *
	 * Low power sleep, no sensing is enabled.
	 */
	CH_MODE_IDLE = 0x00,
	/**
	 * @brief Free running mode
	 *
	 * Sensor uses internal clock to wake and measure.
	 */
	CH_MODE_FREERUN = 0x02,
	/**
	 * @brief Triggered transmit/receive mode
	 *
	 * Transmits and receives when INT line triggered.
	 */
	CH_MODE_TRIGGERED_TX_RX = 0x10,
	/**
	 * @brief Triggered receive-only mode
	 *
	 * Used for pitch/catch operation with another sensor.
	 */
	CH_MODE_TRIGGERED_RX_ONLY = 0x20,
};

struct ch_firmware {
	uint16_t init_addr;
	uint16_t init_size;
	uint16_t fw_size;
	const uint8_t * init_fw;
	const uint8_t *fw;
};

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_SENSOR_CH_COMMON_H */
