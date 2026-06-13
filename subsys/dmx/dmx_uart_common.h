/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Shared declarations for DMX512 UART backends (ISR and DMA).
 */

#ifndef ZEPHYR_SUBSYS_DMX_DMX_UART_COMMON_H_
#define ZEPHYR_SUBSYS_DMX_DMX_UART_COMMON_H_

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#if defined(CONFIG_GPIO)
#include <zephyr/drivers/gpio.h>
#endif
#include <zephyr/dmx/dmx.h>
#include "dmx_internal.h"

/** RX state machine states. */
enum dmx_uart_rx_state {
	/** Waiting for the first BREAK. */
	DMX_UART_RX_IDLE,
	/** First BREAK seen; discard this frame, wait for second BREAK. */
	DMX_UART_RX_SYNC,
	/** BREAK detected, next clean byte is the start code. */
	DMX_UART_RX_BREAK,
	/** Receiving frame data. */
	DMX_UART_RX_DATA,
};

/**
 * Minimum break-to-break time for receivers (ANSI E1.11 Table 7). Spec value is 1196 us; rounded
 * down to 1 ms due to k_uptime millisecond resolution.
 */
#define DMX_MIN_BREAK_TO_BREAK_MS 1

/** Maximum break-to-break time for receivers (ANSI E1.11 Table 7). */
#define DMX_MAX_BREAK_TO_BREAK_MS 1250

struct dmx_uart_data {
	/** Common data, must be first. */
	struct dmx_data common;
	/** Zero-copy frame assembly state. */
	struct dmx_rx_state rx;
	/** RX state machine. */
	enum dmx_uart_rx_state rx_state;
	/** k_uptime_get_32() value when the most recent BREAK was detected. */
	uint32_t last_break_uptime;
	/** Signal-loss timer. Restarted on every valid break. Expires after
	 *  DMX_MAX_BREAK_TO_BREAK_MS to reset state to IDLE.
	 */
	struct k_timer rx_timeout;
};

struct dmx_uart_config {
	/** Underlying UART device. */
	const struct device *uart;
#if defined(CONFIG_GPIO)
	/** RS-485 driver-enable pin (active high), or {0} if unused. */
	struct gpio_dt_spec de;
	/** RS-485 receiver-enable pin (active low), or {0} if unused. */
	struct gpio_dt_spec re;
#endif
};

/**
 * @brief Check if a valid DMX signal is being received.
 */
bool dmx_uart_is_receiving(const struct device *dev);

/**
 * @brief Handle a detected BREAK (framing error).
 *
 * Validates break-to-break timing per ANSI E1.11 Table 7 and manages the sync/receive state
 * machine. The first break after enable enters a sync state that discards one frame to ensure the
 * first delivered frame starts at a clean frame boundary.
 */
void dmx_uart_handle_break(struct dmx_uart_data *data);

/**
 * @brief Process a received byte through the RX state machine.
 *
 * Shared by the ISR loop, the per-byte drain path, and the DMA completion callback.
 */
void dmx_uart_process_byte(struct dmx_uart_data *data, uint8_t byte);

/**
 * @brief Common UART configuration for DMX512 (250000 baud, 8N2).
 * @return 0 on success, negative errno on failure.
 */
int dmx_uart_configure(const struct device *uart_dev);

/**
 * @brief Common enable logic: configure UART and set GPIO pins for RX.
 * @return 0 on success, negative errno on failure.
 */
int dmx_uart_enable_common(const struct device *dev);

/**
 * @brief Common disable logic: de-assert GPIO pins.
 */
void dmx_uart_disable_common(const struct device *dev);

/**
 * @brief Common set_mode implementation.
 */
int dmx_uart_set_mode(const struct device *dev, enum dmx_mode mode);

/**
 * @brief Initialize GPIO pins during device init.
 * @return 0 on success, negative errno on failure.
 */
int dmx_uart_init_gpio(const struct dmx_uart_config *cfg);

/** Helper: get the buffer size for a DMX UART instance. */
#define DMX_UART_BUF_SIZE(inst) DT_INST_PROP_OR(inst, rx_buf_size, CONFIG_DMX_RX_BUF_SIZE)

/**
 * Common fields for the device instantiation macro. Backend-specific macros extend this with
 * their own fields.
 */
#define DMX_UART_COMMON_CONFIG_INIT(inst)                                     \
	.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                            \
	IF_ENABLED(CONFIG_GPIO, (                                             \
		.de = GPIO_DT_SPEC_INST_GET_OR(inst, de_gpios, {0}),         \
		.re = GPIO_DT_SPEC_INST_GET_OR(inst, re_gpios, {0}),         \
	))

#endif /* ZEPHYR_SUBSYS_DMX_DMX_UART_COMMON_H_ */
