/*
 * Copyright (c) 2026 ELAN Microelectronics Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT elan_em32_uart

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

#define UART_STATE_TX_BUSY_MASK        BIT(0)
#define UART_STATE_RX_RDY_MASK         BIT(1)
#define UART_STATE_RX_BUF_OVERRUN_MASK BIT(3)

/* EM32 UART register offsets - corrected per spec */
#define UART_DATA_OFFSET      0x00
#define UART_STATE_OFFSET     0x04
#define UART_CTRL_OFFSET      0x08
#define UART_INTSTACLR_OFFSET 0x0C
#define UART_BAUDDIV_OFFSET   0x10

LOG_MODULE_REGISTER(uart_em32, CONFIG_UART_LOG_LEVEL);

struct uart_em32_config {
	uintptr_t base;                 /* base address from DTS `reg` */
	const struct device *clock_dev; /* clock device reference from DTS "clocks" property */
	uint32_t clock_gate_id;
	const struct pinctrl_dev_config *pcfg;
};

struct uart_em32_data {
	uint32_t baudrate;
};

static inline uint32_t uart_em32_read(const struct device *dev, uint32_t offset)
{
	const struct uart_em32_config *config = dev->config;

	return sys_read32(config->base + offset);
}

static inline void uart_em32_write(const struct device *dev, uint32_t offset, uint32_t value)
{
	const struct uart_em32_config *config = dev->config;

	sys_write32(value, config->base + offset);
}

static int uart_em32_uart_poll_in(const struct device *dev, unsigned char *p_char)
{
	/* Check if RX data is available */
	if (!(uart_em32_read(dev, UART_STATE_OFFSET) & UART_STATE_RX_RDY_MASK)) {
		return -1;
	}

	*p_char = uart_em32_read(dev, UART_DATA_OFFSET) & 0xFF;
	return 0;
}

static void uart_em32_uart_poll_out(const struct device *dev, unsigned char out_char)
{
	/* Wait until TX is not busy */
	while (uart_em32_read(dev, UART_STATE_OFFSET) & UART_STATE_TX_BUSY_MASK) {
		/* spin */
		continue;
	}

	uart_em32_write(dev, UART_DATA_OFFSET, out_char);
}

static int uart_em32_uart_err_check(const struct device *dev)
{
	uint32_t status = uart_em32_read(dev, UART_STATE_OFFSET);
	int err = 0;

	if (status & UART_STATE_RX_BUF_OVERRUN_MASK) {
		err |= UART_ERROR_OVERRUN;
	}

	return err;
}

static const struct uart_driver_api uart_em32_api = {
	.poll_in = uart_em32_uart_poll_in,
	.poll_out = uart_em32_uart_poll_out,
	.err_check = uart_em32_uart_err_check,
};

static int uart_em32_init(const struct device *dev)
{
	const struct uart_em32_config *cfg = dev->config;
	struct uart_em32_data *data = dev->data;
	uint32_t apb_clk_rate = 0;
	uint32_t bauddiv;
	uint32_t baudrate = data->baudrate;
	int ret = 0;

	/* Apply pinctrl configuration first so IOShare and IOMUX are set
	 * before configuring UART registers. Some hardware requires the
	 * pin routing to be in place before the peripheral is initialized.
	 */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Enable clock to specified peripheral */

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, UINT_TO_POINTER(cfg->clock_gate_id));
	if (ret) {
		return ret;
	}

	/* Get APB Clock Rate */
	ret = clock_control_get_rate(cfg->clock_dev, NULL, &apb_clk_rate);
	if (ret < 0) {
		return ret;
	}

	bauddiv = (apb_clk_rate + (baudrate / 2)) / baudrate;

	if (bauddiv < 16) {
		bauddiv = 16;
	}

	uart_em32_write(dev, UART_BAUDDIV_OFFSET, bauddiv);
	uart_em32_write(dev, UART_INTSTACLR_OFFSET, 0xF);
	uart_em32_write(dev, UART_CTRL_OFFSET, 0x3);

	return 0;
}

#define UART_EM32_INIT(index)                                                                      \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	static struct uart_em32_data uart_em32_data_##index = {                                    \
		.baudrate = DT_INST_PROP(index, current_speed),                                    \
	};                                                                                         \
                                                                                                   \
	static const struct uart_em32_config uart_em32_config_##index = {                          \
		.base = DT_INST_REG_ADDR(index),                                                   \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_gate_id = DT_INST_CLOCKS_CELL_BY_IDX(index, 0, gate_id),                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, uart_em32_init, NULL, /* PM control */                        \
			      &uart_em32_data_##index, &uart_em32_config_##index, PRE_KERNEL_1,    \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_em32_api);

DT_INST_FOREACH_STATUS_OKAY(UART_EM32_INIT)
