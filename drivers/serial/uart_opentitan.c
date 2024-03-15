/*
 * Copyright (c) 2023 by Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

/* Register offsets within the UART device register space. */
#define UART_INTR_STATE_REG_OFFSET 0x0
#define UART_INTR_ENABLE_REG_OFFSET 0x4
#define UART_CTRL_REG_OFFSET 0x10
#define UART_STATUS_REG_OFFSET 0x14
#define UART_RDATA_REG_OFFSET 0x18
#define UART_WDATA_REG_OFFSET 0x1c
#define UART_FIFO_CTRL_REG_OFFSET 0x20
#define UART_OVRD_REG_OFFSET 0x28
#define UART_TIMEOUT_CTRL_REG_OFFSET 0x30

/* Control register bits. */
#define UART_CTRL_TX_BIT BIT(0)
#define UART_CTRL_RX_BIT BIT(1)
#define UART_CTRL_NCO_OFFSET 16

/* FIFO control register bits. */
#define UART_FIFO_CTRL_RXRST_BIT BIT(0)
#define UART_FIFO_CTRL_TXRST_BIT BIT(1)

/* Status register bits. */
#define UART_STATUS_TXFULL_BIT BIT(0)
#define UART_STATUS_RXEMPTY_BIT BIT(5)

#define DT_DRV_COMPAT lowrisc_opentitan_uart

struct uart_opentitan_config {
	mem_addr_t base;
	uint32_t nco_reg;
};

static int uart_opentitan_init(const struct device *dev)
{
	const struct uart_opentitan_config *cfg = dev->config;

	/* Reset settings. */
	sys_write32(0u, cfg->base + UART_CTRL_REG_OFFSET);

	/* Clear FIFOs. */
	sys_write32(UART_FIFO_CTRL_RXRST_BIT | UART_FIFO_CTRL_TXRST_BIT,
		    cfg->base + UART_FIFO_CTRL_REG_OFFSET);

	/* Clear other states. */
	sys_write32(0u, cfg->base + UART_OVRD_REG_OFFSET);
	sys_write32(0u, cfg->base + UART_TIMEOUT_CTRL_REG_OFFSET);

	/* Disable interrupts. */
	sys_write32(0u, cfg->base + UART_INTR_ENABLE_REG_OFFSET);

	/* Clear interrupts. */
	sys_write32(0xffffffffu, cfg->base + UART_INTR_STATE_REG_OFFSET);

	/* Set baud and enable TX and RX. */
	sys_write32(UART_CTRL_TX_BIT | UART_CTRL_RX_BIT |
		(cfg->nco_reg << UART_CTRL_NCO_OFFSET),
		cfg->base + UART_CTRL_REG_OFFSET);
	return 0;
}

static int uart_opentitan_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_opentitan_config *cfg = dev->config;

	if (sys_read32(cfg->base + UART_STATUS_REG_OFFSET) & UART_STATUS_RXEMPTY_BIT) {
	    /* Empty RX FIFO */
		return -1;
	}
	*c = sys_read32(cfg->base + UART_RDATA_REG_OFFSET);
	return 0;
}

static void uart_opentitan_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_opentitan_config *cfg = dev->config;

	/* Wait for space in the TX FIFO */
	while (sys_read32(cfg->base + UART_STATUS_REG_OFFSET) &
		UART_STATUS_TXFULL_BIT) {
		;
	}

	sys_write32(c, cfg->base + UART_WDATA_REG_OFFSET);
}

static const struct uart_driver_api uart_opentitan_driver_api = {
	.poll_in = uart_opentitan_poll_in,
	.poll_out = uart_opentitan_poll_out,
};

/* The baud rate is set by writing to the CTRL.NCO register, which is
 * calculated based on baud ticks per system clock tick multiplied by a
 * predefined scaler value.
 */
#define NCO_REG(baud, clk) (BIT64(20) * (baud) / (clk))

#define UART_OPENTITAN_INIT(n) \
	static struct uart_opentitan_config uart_opentitan_config_##n = \
	{ \
		.base = DT_INST_REG_ADDR(n), \
		.nco_reg = NCO_REG(DT_INST_PROP(n, current_speed), \
			DT_INST_PROP(n, clock_frequency)), \
	}; \
	\
	DEVICE_DT_INST_DEFINE(n, uart_opentitan_init, NULL, NULL, \
				&uart_opentitan_config_##n, \
				PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
				&uart_opentitan_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_OPENTITAN_INIT)
