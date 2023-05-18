/*
 * Copyright (c) 2023 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT adi_max32_uart

LOG_MODULE_REGISTER(uart_max32, CONFIG_UART_LOG_LEVEL);

struct max32_uart_regs {
	uint32_t ctrl;
	uint32_t thresh_ctrl;
	uint32_t status;
	uint32_t int_en;
	uint32_t int_fl;
	uint32_t baud0;
	uint32_t baud1;
	uint32_t fifo;
	uint32_t dma;
	uint32_t tx_fifo;
};

struct uart_max32_config {
	volatile struct max32_uart_regs *uart;
	uint32_t baud_rate;
	uint8_t parity;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	uint32_t clock_bus;
	uint32_t clock_bit;
	uint8_t data_bits;
	bool use_obrc;
};

#define UART_CTRL_CLKSEL_OBRC   BIT(15)
#define UART_CTRL_BIT_SIZE_MASK GENMASK(9, 8)
#define UART_CTRL_PARITY_MASK   GENMASK(3, 2)
#define UART_CTRL_PARITY_EVEN   0x00
#define UART_CTRL_PARITY_ODD    0x01
#define UART_CTRL_PARITY_MARK   0x02
#define UART_CTRL_PARITY_SPACE  0x03
#define UART_CTRL_PARITY_ENABLE BIT(1)
#define UART_CTRL_ENABLE        BIT(0)
#define UART_STATUS_RX_EMPTY    BIT(4)
#define UART_STATUS_TX_FULL     BIT(7)
#define UART_INTFL_FRAME        BIT(0)
#define UART_INTFL_PARITY       BIT(1)
#define UART_INTFL_OVERRUN      BIT(3)
#define UART_INTFL_BREAK        BIT(7)

static void uart_max32_poll_out(const struct device *dev, unsigned char c)
{
	volatile struct max32_uart_regs *uart = dev->data;

	while (uart->status & UART_STATUS_TX_FULL)
		;

	uart->fifo = c;
}

static int uart_max32_poll_in(const struct device *dev, unsigned char *c)
{
	volatile struct max32_uart_regs *uart = dev->data;

	if (uart->status & UART_STATUS_RX_EMPTY) {
		return -ENODATA;
	}

	*c = (unsigned char)(uart->fifo & 0xFF);

	return 0;
}

static int uart_max32_err_check(const struct device *dev)
{
	volatile struct max32_uart_regs *uart = dev->data;
	uint32_t flags = uart->int_fl;
	int err = 0;

	if (flags & UART_INTFL_FRAME) {
		err |= UART_ERROR_FRAMING;
	}

	if (flags & UART_INTFL_PARITY) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & UART_INTFL_OVERRUN) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & UART_INTFL_BREAK) {
		err |= UART_BREAK;
	}

	return err;
}

static int uart_max32_set_baud_rate(const struct device *dev, uint32_t baud)
{
	const struct uart_max32_config *const cfg = dev->config;
	volatile struct max32_uart_regs *uart = cfg->uart;

	uint64_t clock_rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 2;

	if (cfg->use_obrc) {
		uart->ctrl |= UART_CTRL_CLKSEL_OBRC;
		clock_rate = DT_PROP(DT_NODELABEL(clk_obrc), clock_frequency);
	}

	uint32_t div;
	uint32_t factor;

	for (factor = 0; factor < 5; factor++) {
		div = (clock_rate << 7) / ((1 << (7 - factor)) * baud);
		if (div >= (1 << 7)) {
			/* found most accurate rate. */
			break;
		}
	}

	if (div < (1 << 7)) {
		/* cannot achieve desired baud rate. */
		return -EINVAL;
	}

	uint32_t ibaud = div >> 7;
	uint32_t dbaud = div & (0x7F);

	if (dbaud > 3) {
		dbaud -= 3;
	} else {
		dbaud += 3;
	}

	uart->baud0 = (factor << 16) | ibaud;
	uart->baud1 = dbaud;

	return 0;
}

static void uart_max32_set_data_bits(const struct device *dev)
{
	const struct uart_max32_config *const cfg = dev->config;
	volatile struct max32_uart_regs *uart = cfg->uart;

	uart->ctrl &= ~UART_CTRL_BIT_SIZE_MASK;
	uart->ctrl |= FIELD_PREP(UART_CTRL_BIT_SIZE_MASK, cfg->data_bits);
}

static int uart_max32_init(const struct device *dev)
{
	const struct uart_max32_config *const cfg = dev->config;
	volatile struct max32_uart_regs *uart = cfg->uart;
	uint32_t clkcfg;
	int ret;

	if (!device_is_ready(cfg->clock)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	clkcfg = (cfg->clock_bus << 16) | cfg->clock_bit;
	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)clkcfg);
	if (ret != 0) {
		LOG_ERR("cannot enable UART clock");
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	if (cfg->parity) {
		uart->ctrl &= ~UART_CTRL_PARITY_MASK;

		switch (cfg->parity) {
		case UART_CFG_PARITY_ODD:
			uart->ctrl |= FIELD_PREP(UART_CTRL_PARITY_MASK, UART_CTRL_PARITY_ODD);
			break;
		case UART_CFG_PARITY_EVEN:
			uart->ctrl |= FIELD_PREP(UART_CTRL_PARITY_MASK, UART_CTRL_PARITY_EVEN);
			break;
		case UART_CFG_PARITY_SPACE:
			uart->ctrl |= FIELD_PREP(UART_CTRL_PARITY_MASK, UART_CTRL_PARITY_SPACE);
		case UART_CFG_PARITY_MARK:
			uart->ctrl |= FIELD_PREP(UART_CTRL_PARITY_MASK, UART_CTRL_PARITY_MARK);
			break;
		}

		uart->ctrl |= UART_CTRL_PARITY_ENABLE;
	}

	uart_max32_set_data_bits(dev);

	ret = uart_max32_set_baud_rate(dev, cfg->baud_rate);
	if (ret < 0) {
		LOG_ERR("cannot set uart baudrate: %d", cfg->baud_rate);
		return ret;
	}

	uart->int_fl = uart->int_fl;

	uart->ctrl |= UART_CTRL_ENABLE;

	return 0;
}

static const struct uart_driver_api uart_max32_driver_api = {
	.poll_in = uart_max32_poll_in,
	.poll_out = uart_max32_poll_out,
	.err_check = uart_max32_err_check,
};

#define MAX32_UART_INIT(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct uart_max32_config uart_max32_config_##n = {                            \
		.uart = (struct max32_uart_regs *)DT_INST_REG_ADDR(n),                             \
		.baud_rate = DT_INST_PROP(n, current_speed),                                       \
		.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),                    \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                        \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                    \
		.clock_bus = DT_INST_CLOCKS_CELL(n, offset),                                       \
		.clock_bit = DT_INST_CLOCKS_CELL(n, bit),                                          \
		.use_obrc = DT_INST_ENUM_IDX(n, clock_source),                                     \
		.data_bits = DT_INST_ENUM_IDX(n, data_bits),                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_max32_init, NULL,                                            \
			      (struct max32_uart_regs *)DT_INST_REG_ADDR(n),                       \
			      &uart_max32_config_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,   \
			      (void *)&uart_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX32_UART_INIT)
