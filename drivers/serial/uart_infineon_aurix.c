/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

#define DT_DRV_COMPAT infineon_asclin_uart

#define UART_ASCLIN_FIFO_SIZE 16

#if CONFIG_SOC_SERIES_TC3X
#define ASCLIN_TXFIFOCON   0x0C
#define ASCLIN_RXFIFOCON   0x10
#define ASCLIN_BITCON      0x14
#define ASCLIN_FRAMECON    0x18
#define ASCLIN_DATCON      0x1C
#define ASCLIN_BRG         0x20
#define ASCLIN_FLAGSCLEAR  0x3C
#define ASCLIN_FLAGSENABLE 0x40
#define ASCLIN_TXDATA      0x44
#define ASCLIN_RXDATA      0x48
#define ASCLIN_CSR         0x4C
#elif CONFIG_SOC_SERIES_TC4X
#define ASCLIN_TXFIFOCON   0x104
#define ASCLIN_RXFIFOCON   0x108
#define ASCLIN_BITCON      0x10C
#define ASCLIN_FRAMECON    0x110
#define ASCLIN_DATCON      0x114
#define ASCLIN_BRG         0x118
#define ASCLIN_FLAGSCLEAR  0x134
#define ASCLIN_FLAGSENABLE 0x138
#define ASCLIN_TXDATA      0x140
#define ASCLIN_RXDATA      0x160
#define ASCLIN_CSR         0x13C
#endif

#define BITCON_PRESCALER    GENMASK(11, 0)
#define BITCON_OVERSAMPLING GENMASK(19, 16)
#define BITCON_SAMPLEPOINT  GENMASK(27, 24)
#define BITCON_SM           BIT(31)
#define FRAMECON_STOP       GENMASK(11, 9)
#define FRAMECON_MODE       GENMASK(17, 16)
#define FRAMECON_PEN        BIT(30)
#define FRAMECON_ODD        BIT(31)
#define DATCON_DATLEN       GENMASK(3, 0)
#define BRG_DENOMINATOR     GENMASK(11, 0)
#define BRG_NUMERATOR       GENMASK(27, 16)
#define FIFOCON_FLUSH       BIT(0)
#define TXFIFOCON_ENO       BIT(1)
#define RXFIFOCON_ENI       BIT(1)
#define TXFIFOCON_INW       GENMASK(7, 6)
#define RXFIFOCON_OUTW      GENMASK(7, 6)
#define FIFOCON_FILL        GENMASK(20, 16)
#define CSR_CLKSEL          GENMASK(4, 0)
#define CSR_CON             BIT(31)

/* CSR.CLKSEL value selecting the fast ASCLIN clock (fASCLINF). */
#define UART_ASCLIN_CLKSEL_OFF 0
#define UART_ASCLIN_CLKSEL_ON  2

struct uart_asclin_data {
	struct uart_config *uart_cfg;
	struct k_spinlock lock;
};

struct uart_asclin_config {
	mem_addr_t base;
	uint32_t clock_freq;
	uint16_t prescaler;
	uint8_t oversampling;
	uint8_t samplepoint;
	bool median_filter;
};

static inline uint32_t uart_asclin_rx_fifo_fill_level(mem_addr_t base)
{
	return FIELD_GET(FIFOCON_FILL, sys_read32(base + ASCLIN_RXFIFOCON));
}

static inline uint32_t uart_asclin_tx_fifo_fill_level(mem_addr_t base)
{
	return FIELD_GET(FIFOCON_FILL, sys_read32(base + ASCLIN_TXFIFOCON));
}

static inline uint8_t uart_asclin_rx_fifo_read(mem_addr_t base)
{
	return (uint8_t)sys_read32(base + ASCLIN_RXDATA);
}

static inline void uart_asclin_tx_fifo_write(mem_addr_t base, uint8_t c)
{
	sys_write32(c, base + ASCLIN_TXDATA);
}

static inline void uart_asclin_set_bittime(mem_addr_t base, uint8_t oversampling,
					   uint16_t prescaler, uint8_t samplepoint,
					   bool median_filter)
{
	uint32_t bitcon = FIELD_PREP(BITCON_OVERSAMPLING, oversampling - 1) |
			  FIELD_PREP(BITCON_PRESCALER, prescaler - 1) |
			  FIELD_PREP(BITCON_SAMPLEPOINT, samplepoint) |
			  (median_filter ? BITCON_SM : 0);

	sys_write32(bitcon, base + ASCLIN_BITCON);
}

static inline int uart_asclin_set_clk(mem_addr_t base, uint8_t clksel)
{
	sys_write32(clksel, base + ASCLIN_CSR);
	if (clksel) {
		return !WAIT_FOR((sys_read32(base + ASCLIN_CSR) & CSR_CON) != 0, 1000,
				 k_busy_wait(1));
	} else {
		return !WAIT_FOR((sys_read32(base + ASCLIN_CSR) & CSR_CON) == 0, 1000,
				 k_busy_wait(1));
	}
}

static bool uart_asclin_set_baudrate(mem_addr_t base, uint32_t baudrate, uint32_t fpd,
				     uint8_t oversampling)
{
	uint32_t fovs = baudrate * oversampling;
	int32_t m[2][2];
	int32_t ai;
	float divider;

	if (fpd == 0) {
		return false;
	}

	divider = ((float)fovs / (float)fpd);

	/* initialize matrix */
	m[0][0] = 1;
	m[1][1] = 1;
	m[0][1] = 0;
	m[1][0] = 0;

	/* loop finding terms until denom gets too big */
	while (m[1][0] * (ai = (uint32_t)divider) + m[1][1] <= 4095) {
		int32_t t;

		t = m[0][0] * ai + m[0][1];
		m[0][1] = m[0][0];
		m[0][0] = t;
		t = m[1][0] * ai + m[1][1];
		m[1][1] = m[1][0];
		m[1][0] = t;
		divider = 1 / (divider - (float)ai);
	}

	sys_write32(FIELD_PREP(BRG_NUMERATOR, m[0][0]) | FIELD_PREP(BRG_DENOMINATOR, m[1][0]),
		    base + ASCLIN_BRG);

	return true;
}

static inline void uart_asclin_set_framecfg(mem_addr_t base, uint8_t data_bits, uint8_t stop_bits,
					    bool parity_enable, bool parity_odd)
{
	uint32_t framecon = FIELD_PREP(FRAMECON_STOP, stop_bits) | FIELD_PREP(FRAMECON_MODE, 1) |
			    (parity_enable ? FRAMECON_PEN : 0) | (parity_odd ? FRAMECON_ODD : 0);
	uint32_t datcon = FIELD_PREP(DATCON_DATLEN, data_bits);
	uint32_t txfifocon =
		FIFOCON_FLUSH | TXFIFOCON_ENO | FIELD_PREP(TXFIFOCON_INW, data_bits > 8 ? 2 : 1);
	uint32_t rxfifocon =
		FIFOCON_FLUSH | RXFIFOCON_ENI | FIELD_PREP(RXFIFOCON_OUTW, data_bits > 8 ? 2 : 1);

	sys_write32(framecon, base + ASCLIN_FRAMECON);
	sys_write32(datcon, base + ASCLIN_DATCON);
	sys_write32(txfifocon, base + ASCLIN_TXFIFOCON);
	sys_write32(rxfifocon, base + ASCLIN_RXFIFOCON);
}

static int uart_asclin_poll_in(const struct device *dev, unsigned char *p_char)
{
	const struct uart_asclin_config *config = dev->config;
	struct uart_asclin_data *data = dev->data;
	int ret_val = -1;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(p_char != NULL, "p_char is null pointer!");

	/* Stop, if p_char is null pointer */
	if (p_char == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* check if received character is ready.*/
	if (uart_asclin_rx_fifo_fill_level(config->base)) {
		/* got a character */
		*p_char = uart_asclin_rx_fifo_read(config->base);
		ret_val = 0;
	}

	k_spin_unlock(&data->lock, key);

	return ret_val;
}

static void uart_asclin_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_asclin_config *config = dev->config;
	struct uart_asclin_data *data = dev->data;

	k_spinlock_key_t key;

	/* wait until uart is free to transmit.*/
	while (true) {
		key = k_spin_lock(&data->lock);
		if (uart_asclin_tx_fifo_fill_level(config->base) < UART_ASCLIN_FIFO_SIZE) {
			break;
		}
		k_spin_unlock(&data->lock, key);
	}

	uart_asclin_tx_fifo_write(config->base, c);

	k_spin_unlock(&data->lock, key);
}

static int uart_asclin_init(const struct device *dev)
{
	const struct uart_asclin_config *cfg = dev->config;
	const struct uart_asclin_data *data = dev->data;

	if (uart_asclin_set_clk(cfg->base, UART_ASCLIN_CLKSEL_OFF)) {
		return -ETIMEDOUT;
	}
	sys_write32(0, cfg->base + ASCLIN_FRAMECON);

	uart_asclin_set_bittime(cfg->base, cfg->oversampling, cfg->prescaler, cfg->samplepoint,
				cfg->median_filter);
	uart_asclin_set_baudrate(cfg->base, data->uart_cfg->baudrate, cfg->clock_freq,
				 cfg->oversampling);
	uart_asclin_set_framecfg(cfg->base, data->uart_cfg->data_bits + 4,
				 data->uart_cfg->stop_bits,
				 data->uart_cfg->parity != UART_CFG_PARITY_NONE,
				 data->uart_cfg->parity == UART_CFG_PARITY_ODD);

	sys_write32(0, cfg->base + ASCLIN_FLAGSENABLE);
	sys_write32(0xFFFFFFFFu, cfg->base + ASCLIN_FLAGSCLEAR);

	if (uart_asclin_set_clk(cfg->base, UART_ASCLIN_CLKSEL_ON)) {
		return -ETIMEDOUT;
	}

	return 0;
}

static DEVICE_API(uart, uart_asclin_driver_api) = {
	.poll_in = uart_asclin_poll_in,
	.poll_out = uart_asclin_poll_out,
};

#define UART_ASCLIN_DEVICE_INIT(n)                                                                 \
	static struct uart_config uart_cfg_##n = {                                                 \
		.baudrate = DT_INST_PROP_OR(n, current_speed, 115200),                             \
		.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),                    \
		.stop_bits = DT_INST_ENUM_IDX_OR(n, stop_bits, UART_CFG_STOP_BITS_1),              \
		.data_bits = DT_INST_ENUM_IDX_OR(n, data_bits, UART_CFG_DATA_BITS_8),              \
		.flow_ctrl = DT_INST_PROP(n, hw_flow_control) ? UART_CFG_FLOW_CTRL_RTS_CTS         \
							      : UART_CFG_FLOW_CTRL_NONE,           \
	};                                                                                         \
	static struct uart_asclin_data uart_asclin_dev_data_##n = {.uart_cfg = &uart_cfg_##n};     \
	static const struct uart_asclin_config uart_asclin_dev_cfg_##n = {                         \
		.base = (mem_addr_t)DT_INST_REG_ADDR(n),                                           \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		.oversampling = DT_INST_PROP_OR(n, oversampling, 16),                              \
		.samplepoint = DT_INST_PROP_OR(n, samplepoint, 8),                                 \
		.median_filter = DT_INST_PROP(n, median_filter),                                   \
		.prescaler = DT_INST_PROP_OR(n, prescaler, 1)};                                    \
	DEVICE_DT_INST_DEFINE(n, uart_asclin_init, NULL, &uart_asclin_dev_data_##n,                \
			      &uart_asclin_dev_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &uart_asclin_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_ASCLIN_DEVICE_INIT)
