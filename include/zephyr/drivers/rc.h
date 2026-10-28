/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RC_H_

/**
 * @file
 * @ingroup rc_driver_interface
 * @brief Remote control (IR) driver API.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/rc/rc.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Remote control driver interface
 * @defgroup rc_driver_interface Remote Control Drivers
 * @ingroup rc_interface
 * @{
 */

/** @brief Driver roles, combined in rc_caps::roles. */
enum rc_driver_role {
	/** Receiver reporting raw pulse and space durations */
	RC_ROLE_RAW_RX = BIT(0),
	/** Transmitter of raw pulse and space durations */
	RC_ROLE_RAW_TX = BIT(1),
};

/** @brief Driver capabilities. */
struct rc_caps {
	/** rc_driver_role bits */
	uint32_t roles;
	/** Receive timing resolution in ns */
	uint32_t rx_resolution_ns;
	/** Minimum TX carrier frequency in Hz, 0 if fixed or none */
	uint32_t carrier_min_hz;
	/** Maximum TX carrier frequency in Hz, 0 if fixed or none */
	uint32_t carrier_max_hz;
	/** Maximum raw words per transmission, 0 if unbounded */
	uint32_t max_tx_words;
	/** Minimum receiver timeout in us */
	uint32_t rx_timeout_min_us;
	/** Maximum receiver timeout in us */
	uint32_t rx_timeout_max_us;
};

/** @brief Remote control driver API. */
__subsystem struct rc_driver_api {
	/** Get capabilities. Mandatory. */
	int (*get_caps)(const struct device *dev, struct rc_caps *caps);
	/** Start or stop reception. Mandatory for receivers. */
	int (*rx_enable)(const struct device *dev, bool enable);
	/** Set receiver timeout in us. Optional. */
	int (*set_rx_timeout)(const struct device *dev, uint32_t us);
	/** Set TX carrier frequency and duty cycle. Optional. */
	int (*set_carrier)(const struct device *dev, uint32_t hz, uint8_t duty_pct);
	/** Transmit raw words, blocking until the last word is sent. Mandatory for transmitters. */
	int (*tx)(const struct device *dev, const uint32_t *words, size_t count);
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RC_H_ */
