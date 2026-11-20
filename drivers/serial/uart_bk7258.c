/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BK7258 UART driver, polled mode.
 *
 * On this board the port is configured by the firmware the device ships with,
 * so the driver has to leave the hardware alone; see
 * CONFIG_UART_BK7258_NO_HW_INIT.
 *
 * Only poll_in / poll_out / err_check and the optional runtime configure are
 * implemented. There is no pinctrl driver for this SoC yet, so pin muxing is
 * written to registers directly. Register programming follows the vendor
 * uart_driver.c uart_id_init_common() sequence.
 */

#define DT_DRV_COMPAT beken_bk7258_uart

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/init.h>

#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_bk7258, CONFIG_UART_LOG_LEVEL);

/*
 * Configuration and runtime data
 */

struct uart_bk7258_config {
	DEVICE_MMIO_ROM;
	uint32_t clock_freq;
	uint32_t baud;
	uint32_t tx_pin;
	uint32_t rx_pin;
	uint32_t tx_func;
	uint32_t rx_func;
	bool use_apll;
};

struct uart_bk7258_data {
	DEVICE_MMIO_RAM;
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uart_config current;
#endif
};

/*
 * Internal helpers
 */

static inline soc_uart_hw_t *uart_bk7258_hw(const struct device *dev)
{
	return (soc_uart_hw_t *)DEVICE_MMIO_GET(dev);
}

/**
 * @brief Put one pin into peripheral mux mode.
 *
 * Two-level mechanism:
 *   level 1: pin configuration word bit 6, gpio_2_func_en = 1
 *   level 2: seven words from 0x440100C0, 4-bit function number per pin
 *
 * The vendor uart_init_gpio() pulls up both TX and RX; same here.
 */
static void uart_bk7258_pin_setup(uint32_t pin, uint32_t func)
{
	uint32_t cfg_addr = SOC_GPIO_PIN_CFG_ADDR(pin);
	uint32_t cfg;

	/* Level 1: enable the secondary function, the pad output driver and the
	 * pull-up. Without the output driver the pad stays an input and TX goes
	 * nowhere, which looks exactly like the port never being initialised.
	 */
	cfg = sys_read32(cfg_addr);
	cfg |= SOC_GPIO_CFG_2_FUNC_EN_BIT;
	cfg |= SOC_GPIO_CFG_OUTPUT_EN_BIT;
	cfg |= SOC_GPIO_CFG_PULL_MODE_BIT; /* 1 = pull-up */
	cfg |= SOC_GPIO_CFG_PULL_MODE_EN_BIT;
	sys_write32(cfg, cfg_addr);

	/* Level 2: write the function number. UART0 is function 0 on GPIO10/11,
	 * but writing it explicitly does not depend on the reset value.
	 */
	{
		uint32_t reg = SOC_GPIO_FUNC_MODE_ADDR(pin);
		uint32_t shift = SOC_GPIO_FUNC_MODE_SHIFT(pin);
		uint32_t val = sys_read32(reg);

		val &= ~(0xFU << shift);
		val |= (func & 0xFU) << shift;
		sys_write32(val, reg);
	}
}

/**
 * @brief Select the UART clock source.
 *
 * The vendor sys_hal_uart_select_clock(): clksel_uart0 = 0 -> XTAL, 1 -> APLL.
 * The reset default is already XTAL, but it is written explicitly rather than
 * relying on the reset value.
 */
static void uart_bk7258_clock_select(bool use_apll)
{
	uint32_t reg = sys_read32(SOC_SYS_CPU_CLK_DIV_MODE1_ADDR);

	if (use_apll) {
		reg |= SOC_SYS_CLKSEL_UART0_BIT;
	} else {
		reg &= ~SOC_SYS_CLKSEL_UART0_BIT;
	}
	sys_write32(reg, SOC_SYS_CPU_CLK_DIV_MODE1_ADDR);
}

/**
 * @brief Set the baud rate (only the clk_div field; tx/rx enables untouched).
 *
 * The vendor uart_hal_set_baud_rate(): clk_div = UART_CLOCK / baud_rate - 1
 * 115200 at 26 MHz -> 224
 */
static void uart_bk7258_set_baud(const struct device *dev, uint32_t baud)
{
	const struct uart_bk7258_config *cfg = dev->config;
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);
	uint32_t clk_div;
	uint32_t val;

	if (baud == 0U) {
		LOG_ERR("baud must not be 0");
		return;
	}

	clk_div = (cfg->clock_freq / baud) - 1U;

	/* clk_div is [23:8]: clear it before writing */
	val = hw->config;
	val &= ~(0xFFFFU << SOC_UART_CFG_CLK_DIV_SHIFT);
	val |= (clk_div & 0xFFFFU) << SOC_UART_CFG_CLK_DIV_SHIFT;
	hw->config = val;

	LOG_DBG("baud=%u clk=%u clk_div=%u actual=%u", baud, cfg->clock_freq, clk_div,
		cfg->clock_freq / (clk_div + 1U));
}

/*
 * Driver API
 */

static int uart_bk7258_init(const struct device *dev)
{
	const struct uart_bk7258_config *cfg = dev->config;
	soc_uart_hw_t *hw;
	uint32_t val;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	hw = uart_bk7258_hw(dev);

	LOG_DBG("init base=%#lx baud=%u clk=%u", (unsigned long)DEVICE_MMIO_GET(dev), cfg->baud,
		cfg->clock_freq);

	/*
	 * When the port belongs to the firmware the device ships with, not a
	 * single register is written. A soft reset, a pin change or a new
	 * divider here would truncate that console mid-line and the port would
	 * stay silent afterwards. See UART_BK7258_NO_HW_INIT.
	 */
	if (IS_ENABLED(CONFIG_UART_BK7258_NO_HW_INIT)) {
		LOG_INF("UART%p left as configured by the firmware on the device: "
			"skipping hardware init (assumed %u 8N1)",
			(void *)DEVICE_MMIO_GET(dev), cfg->baud);
		goto done;
	}

	/* --- 1. clock source (before enabling tx/rx) --- */
	uart_bk7258_clock_select(cfg->use_apll);

	/* --- 2. soft reset ---
	 *
	 * The bit is a strobe: it stays readable as 1 and must be left alone, so
	 * this is the only write to global_ctrl. Clearing it and setting the
	 * clock gate bypass instead leaves the port unable to transmit, which is
	 * what the vendor uart_ll_init() avoids by not doing either.
	 */
	hw->global_ctrl = SOC_UART_GLOBAL_SOFT_RESET_BIT;

	/* --- 3. pin muxing (pull-up on both TX and RX) --- */
	uart_bk7258_pin_setup(cfg->tx_pin, cfg->tx_func);
	uart_bk7258_pin_setup(cfg->rx_pin, cfg->rx_func);

	/* --- 4. no interrupts (polled mode); clear anything left pending --- */
	hw->int_enable = 0U;
	hw->int_status = SOC_UART_INT_ALL_MASK; /* write 1 to clear */

	/* --- 5. FIFO thresholds, no flow control --- */
	hw->fifo_config = 1U | (1U << SOC_UART_FIFO_RX_THRESH_SHIFT);
	hw->flow_ctrl_config = 0U;

	/* --- 6. frame format: 8N1 --- */
	val = hw->config;
	val &= ~(0x3U << SOC_UART_CFG_DATA_BITS_SHIFT);
	val |= (SOC_UART_CFG_DATA_BITS_8 << SOC_UART_CFG_DATA_BITS_SHIFT);
	val &= ~(SOC_UART_CFG_PARITY_EN_BIT | SOC_UART_CFG_PARITY_ODD_BIT |
		 SOC_UART_CFG_STOP_BITS_2_BIT);
	hw->config = val;

	/* --- 7. baud rate --- */
	uart_bk7258_set_baud(dev, cfg->baud);

	/* --- 8. enable tx and rx --- */
	hw->config |= SOC_UART_CFG_TX_ENABLE_BIT | SOC_UART_CFG_RX_ENABLE_BIT;

done:
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	{
		struct uart_bk7258_data *data = dev->data;

		data->current.baudrate = cfg->baud;
		data->current.parity = UART_CFG_PARITY_NONE;
		data->current.stop_bits = UART_CFG_STOP_BITS_1;
		data->current.data_bits = UART_CFG_DATA_BITS_8;
		data->current.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	}
#endif

	return 0;
}

/**
 * @brief Send one byte (blocking until the FIFO accepts it).
 *
 * fifo_status bit 20 = fifo_wr_ready
 */
static void uart_bk7258_poll_out(const struct device *dev, unsigned char c)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	while ((hw->fifo_status & SOC_UART_FIFO_STATUS_WR_READY_BIT) == 0U) {
		/* Busy wait: fine during initialisation, a timeout can come later */
	}

	hw->fifo_port = (uint32_t)c & SOC_UART_FIFO_PORT_TX_DATA_MASK;
}

/**
 * @brief Receive one byte (non-blocking).
 *
 * fifo_status bit 21 = fifo_rd_ready, data in fifo_port [15:8]
 *
 * @return 0 a byte was read; -1 nothing available
 */
static int uart_bk7258_poll_in(const struct device *dev, unsigned char *c)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	if ((hw->fifo_status & SOC_UART_FIFO_STATUS_RD_READY_BIT) == 0U) {
		return -1;
	}

	*c = (unsigned char)((hw->fifo_port >> SOC_UART_FIFO_PORT_RX_DATA_SHIFT) & 0xFFU);
	return 0;
}

static int uart_bk7258_err_check(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);
	uint32_t st = hw->int_status;
	int err = 0;

	if (st & SOC_UART_INT_RX_OVERFLOW_BIT) {
		err |= UART_ERROR_OVERRUN;
	}
	if (st & SOC_UART_INT_RX_PARITY_ERR_BIT) {
		err |= UART_ERROR_PARITY;
	}
	if (st & SOC_UART_INT_RX_STOP_ERR_BIT) {
		err |= UART_ERROR_FRAMING;
	}

	if (err) {
		hw->int_status = st; /* write 1 to clear */
	}
	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int uart_bk7258_configure(const struct device *dev, const struct uart_config *cfg)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);
	uint32_t val;

	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		LOG_ERR("hardware flow control is not supported");
		return -ENOTSUP;
	}
	if (cfg->stop_bits == UART_CFG_STOP_BITS_1_5) {
		LOG_ERR("hardware supports 1 or 2 stop bits only");
		return -ENOTSUP;
	}
	if (cfg->baudrate == 0U) {
		return -EINVAL;
	}

	val = hw->config;

	/* data_bits: 0=5bit 1=6bit 2=7bit 3=8bit (vendor field semantics) */
	val &= ~(0x3U << SOC_UART_CFG_DATA_BITS_SHIFT);
	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		val |= 0U << SOC_UART_CFG_DATA_BITS_SHIFT;
		break;
	case UART_CFG_DATA_BITS_6:
		val |= 1U << SOC_UART_CFG_DATA_BITS_SHIFT;
		break;
	case UART_CFG_DATA_BITS_7:
		val |= 2U << SOC_UART_CFG_DATA_BITS_SHIFT;
		break;
	case UART_CFG_DATA_BITS_8:
	default:
		val |= SOC_UART_CFG_DATA_BITS_8 << SOC_UART_CFG_DATA_BITS_SHIFT;
		break;
	}

	/* parity */
	switch (cfg->parity) {
	case UART_CFG_PARITY_ODD:
		val |= SOC_UART_CFG_PARITY_EN_BIT | SOC_UART_CFG_PARITY_ODD_BIT;
		break;
	case UART_CFG_PARITY_EVEN:
		val |= SOC_UART_CFG_PARITY_EN_BIT;
		val &= ~SOC_UART_CFG_PARITY_ODD_BIT;
		break;
	case UART_CFG_PARITY_NONE:
	default:
		val &= ~(SOC_UART_CFG_PARITY_EN_BIT | SOC_UART_CFG_PARITY_ODD_BIT);
		break;
	}

	/* stop bits */
	if (cfg->stop_bits == UART_CFG_STOP_BITS_2) {
		val |= SOC_UART_CFG_STOP_BITS_2_BIT;
	} else {
		val &= ~SOC_UART_CFG_STOP_BITS_2_BIT;
	}

	hw->config = val;
	uart_bk7258_set_baud(dev, cfg->baudrate);

	((struct uart_bk7258_data *)dev->data)->current = *cfg;
	return 0;
}

static int uart_bk7258_config_get(const struct device *dev, struct uart_config *cfg)
{
	*cfg = ((struct uart_bk7258_data *)dev->data)->current;
	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static DEVICE_API(uart, uart_bk7258_driver_api) = {
	.poll_out = uart_bk7258_poll_out,
	.poll_in = uart_bk7258_poll_in,
	.err_check = uart_bk7258_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_bk7258_configure,
	.config_get = uart_bk7258_config_get,
#endif
};

/*
 * Instantiation
 */

#define UART_BK7258_INIT(n)                                                                        \
	static struct uart_bk7258_data uart_bk7258_data_##n;                                       \
                                                                                                   \
	static const struct uart_bk7258_config uart_bk7258_cfg_##n = {                             \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		.baud = DT_INST_PROP(n, current_speed),                                            \
		.tx_pin = DT_INST_PROP(n, tx_pin),                                                 \
		.rx_pin = DT_INST_PROP(n, rx_pin),                                                 \
		.tx_func = DT_INST_PROP(n, tx_func),                                               \
		.rx_func = DT_INST_PROP(n, rx_func),                                               \
		.use_apll = DT_INST_ENUM_IDX(n, clock_source) == 1,                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_bk7258_init, NULL, &uart_bk7258_data_##n,                    \
			      &uart_bk7258_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,     \
			      &uart_bk7258_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_BK7258_INIT)
