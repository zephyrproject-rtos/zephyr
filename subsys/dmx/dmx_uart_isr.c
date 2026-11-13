/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DMX512 UART backend using interrupt-driven (ISR) byte-by-byte reception.
 *
 * Break detection uses uart_err_check() in the UART IRQ callback. The drain strategy after a
 * break depends on the UART's framing-error-reporting DT property.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dmx, CONFIG_DMX_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/dmx/dmx.h>

#include "dmx_uart_common.h"

#define DT_DRV_COMPAT zephyr_dmx_uart_isr

/**
 * Read the framing-error-reporting property from the parent UART node. Resolves to a compile-time
 * enum uart_framing_error_reporting value. Use with DT_STRING_TOKEN (see uart-controller.yaml).
 */
#define DMX_UART_ISR_FE_REPORT(inst) DT_STRING_TOKEN(DT_INST_BUS(inst), framing_error_reporting)

#define DMX_UART_ISR_ASSERT_FE_PROP(inst)                                     \
	BUILD_ASSERT(DT_NODE_HAS_PROP(DT_INST_BUS(inst),                     \
				      framing_error_reporting),                \
		     "Parent UART of dmx-uart-isr instance " #inst " must "   \
		     "set the framing-error-reporting property. "              \
		     "See uart-controller.yaml.");

/** ISR-specific config extending the common config. */
struct dmx_uart_isr_config {
	struct dmx_uart_config common;
	/** How the parent UART reports framing errors. */
	enum uart_framing_error_reporting fe_report;
};

/* ---------- Break drain -------------------------------------------------- */

/**
 * @brief Drain break bytes after a framing error is detected.
 *
 * The drain strategy depends on the UART's framing-error-reporting behavior (from the parent
 * UART's DT property):
 *
 * FIFO_HEAD (STM32, NS16550):
 *   err_check reflects the FIFO head (next byte to be read). Drain while the head has errors.
 *   When the loop exits, the start code is still in the FIFO for normal processing.
 *
 * LAST_READ (PL011):
 *   err_check reflects the byte just read. Drain while errors persist. When the loop exits,
 *   the last byte read was clean (the start code). Process it immediately.
 *
 * STICKY_MULTIPLE (LPUART, SAM0):
 *   err_check clears after the first read. Drain one byte (the error byte). The start code
 *   arrives later.
 *
 * STICKY_SINGLE (nRF UARTE):
 *   Single-byte buffer. Don't drain; each byte arrives in its own ISR. The BREAK state handler
 *   discards additional break bytes.
 *
 * ISR_SAVED (ESP32):
 *   Same behavior as STICKY_MULTIPLE for drain purposes.
 */
static void dmx_uart_isr_drain_break(const struct device *uart_dev,
				     struct dmx_uart_data *data,
				     enum uart_framing_error_reporting fe_report)
{
	uint8_t byte;

	switch (fe_report) {
	case FIFO_HEAD:
		/*
		 * STM32/NS16550: err_check reflects the FIFO head. Drain while the head has errors.
		 * The start code remains in the FIFO for normal processing.
		 */
		do {
			if (uart_fifo_read(uart_dev, &byte, 1) != 1) {
				return;
			}
		} while (uart_err_check(uart_dev) & (UART_BREAK | UART_ERROR_FRAMING));
		return;

	case LAST_READ:
		/*
		 * PL011: err_check reflects the byte just read. Drain while the byte we read has
		 * errors. When the loop exits, 'byte' IS the first clean byte (the start code).
		 * Process it now.
		 */
		do {
			if (uart_fifo_read(uart_dev, &byte, 1) != 1) {
				return;
			}
		} while (uart_err_check(uart_dev) & (UART_BREAK | UART_ERROR_FRAMING));

		dmx_uart_process_byte(data, byte);
		return;

	case STICKY_MULTIPLE:
	case ISR_SAVED:
		/*
		 * Sticky flag with multiple bytes buffered: drain one byte (the error byte that
		 * triggered detection). The start code arrives in a subsequent ISR.
		 */
		uart_fifo_read(uart_dev, &byte, 1);
		return;

	case STICKY_SINGLE:
		/*
		 * Single-byte buffer (nRF UARTE): don't drain. Each byte arrives in its own ISR
		 * invocation. The BREAK state handler already discards additional break bytes.
		 */
		return;

	default:
		CODE_UNREACHABLE;
	}
}

/* ---------- UART IRQ callback -------------------------------------------- */

static void dmx_uart_isr_cb(const struct device *uart_dev, void *user_data)
{
	const struct device *dev = user_data;
	const struct dmx_uart_isr_config *cfg = dev->config;
	struct dmx_uart_data *data = dev->data;
	int err;
	uint8_t byte;

	uart_irq_update(uart_dev);

	if (!uart_irq_is_pending(uart_dev)) {
		return;
	}

	/*
	 * Check for framing errors before rx_ready so that error-only interrupts (no data ready)
	 * are handled on platforms with separate error interrupt lines.
	 */
	err = uart_err_check(uart_dev);
	if (err & (UART_BREAK | UART_ERROR_FRAMING)) {
		dmx_uart_handle_break(data);
		dmx_uart_isr_drain_break(uart_dev, data, cfg->fe_report);
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	/*
	 * DMX break detection via framing errors.
	 *
	 * We also check errors AFTER each byte read for UARTs where err_check reflects the most
	 * recently read byte (see uart-controller.yaml framing-error-reporting).
	 */
	while (true) {
		err = uart_err_check(uart_dev);
		if (err & (UART_BREAK | UART_ERROR_FRAMING)) {
			dmx_uart_handle_break(data);
			dmx_uart_isr_drain_break(uart_dev, data, cfg->fe_report);
			continue;
		}

		if (uart_fifo_read(uart_dev, &byte, 1) != 1) {
			break;
		}

		err = uart_err_check(uart_dev);
		if (err & (UART_BREAK | UART_ERROR_FRAMING)) {
			dmx_uart_handle_break(data);
			dmx_uart_isr_drain_break(uart_dev, data, cfg->fe_report);
			continue;
		}

		dmx_uart_process_byte(data, byte);
	}
}

/* ---------- Driver API --------------------------------------------------- */

static int dmx_uart_isr_enable(const struct device *dev)
{
	struct dmx_uart_data *data = dev->data;
	const struct dmx_uart_isr_config *cfg = dev->config;
	int ret;

	ret = dmx_uart_enable_common(dev);
	if (ret != 0) {
		return ret;
	}

	uart_irq_callback_user_data_set(cfg->common.uart, dmx_uart_isr_cb, (void *)dev);
	uart_irq_rx_enable(cfg->common.uart);
	uart_irq_err_enable(cfg->common.uart);

	data->common.enabled = true;
	return 0;
}

static int dmx_uart_isr_disable(const struct device *dev)
{
	struct dmx_uart_data *data = dev->data;
	const struct dmx_uart_isr_config *cfg = dev->config;

	if (!data->common.enabled) {
		return 0;
	}

	uart_irq_rx_disable(cfg->common.uart);
	uart_irq_err_disable(cfg->common.uart);

	dmx_uart_disable_common(dev);
	return 0;
}

static DEVICE_API(dmx, dmx_uart_isr_api) = {
	.set_mode = dmx_uart_set_mode,
	.enable = dmx_uart_isr_enable,
	.disable = dmx_uart_isr_disable,
	.is_receiving = dmx_uart_is_receiving,
};

/* ---------- Device instantiation ----------------------------------------- */

#define DMX_UART_ISR_DEFINE(inst)                                             \
	static uint8_t dmx_uart_isr_rxbuf_##inst[DMX_UART_BUF_SIZE(inst)]    \
		__aligned(4);                                                 \
                                                                              \
	static struct dmx_uart_data dmx_uart_isr_data_##inst;                 \
                                                                              \
	DMX_UART_ISR_ASSERT_FE_PROP(inst)                                     \
                                                                              \
	static const struct dmx_uart_isr_config dmx_uart_isr_cfg_##inst = {   \
		.common = {                                                   \
			DMX_UART_COMMON_CONFIG_INIT(inst)                     \
		},                                                            \
		.fe_report = DMX_UART_ISR_FE_REPORT(inst),                    \
	};                                                                    \
                                                                              \
	static int dmx_uart_isr_init_##inst(const struct device *dev)         \
	{                                                                     \
		struct dmx_uart_data *data = dev->data;                       \
		const struct dmx_uart_isr_config *cfg = dev->config;          \
                                                                              \
		if (!device_is_ready(cfg->common.uart)) {                     \
			LOG_ERR("UART device not ready");                     \
			return -ENODEV;                                       \
		}                                                             \
                                                                              \
		dmx_data_init(&data->common, dmx_uart_isr_rxbuf_##inst,       \
			      sizeof(dmx_uart_isr_rxbuf_##inst));             \
                                                                              \
		return dmx_uart_init_gpio(&cfg->common);                      \
	}                                                                     \
                                                                              \
	DEVICE_DT_INST_DEFINE(inst, dmx_uart_isr_init_##inst, NULL,           \
			      &dmx_uart_isr_data_##inst,                      \
			      &dmx_uart_isr_cfg_##inst,                       \
			      POST_KERNEL, CONFIG_DMX_INIT_PRIORITY,          \
			      &dmx_uart_isr_api);

DT_INST_FOREACH_STATUS_OKAY(DMX_UART_ISR_DEFINE)
