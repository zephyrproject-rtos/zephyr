/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BK7258 UART driver: polled and interrupt-driven.
 *
 * Design notes:
 * - poll_in / poll_out / err_check are always available; the interrupt API is
 * compiled in with CONFIG_UART_INTERRUPT_DRIVEN and is a strict superset --
 * enabling it never changes what the polled path does.
 * - No pinctrl framework: pin muxing is written straight to registers (two
 * levels, two pins).
 * - The register semantics come one by one from the vendor SoC headers.
 *
 * Mapping against the vendor uart_driver.c::uart_id_init_common():
 * vendor this driver
 * power_uart_pwr_up(id) nothing: no power-up needed
 * clk_set_uart_clk_26m(id) uart_bk7258_clock_select()
 * uart_init_gpio(id) uart_bk7258_pin_setup()
 *
 * Interrupt model
 * ---------------
 * One UART-level interrupt per port, and the status/enable register pair lives
 * in the UART itself (not in an interrupt controller), so it is the UART's only
 * interrupt line. The ISR therefore cannot tell RX from TX out of the vector;
 * it reads int_status, masks it with int_enable, clears it (write 1 to clear,
 * see uart_ll_clear_interrupt_status() in the vendor HAL) and then hands over to
 * the callback, exactly like the vendor uart_isr_common() does. The callback is
 * expected to call uart_irq_update() and uart_fifo_read() itself; the ISR does
 * not drain the FIFO, otherwise data could not be accounted for.
 *
 * The system controller firmware may own the interrupt of a UART it is using, and the
 * interrupt enable bits are shared state (one register file, both cores see it).
 * uart_bk7258_irq_* only ever touches the bits it was asked for, and the port
 * leaves the whole question to the application: nothing enables an interrupt at
 * init time. See UART_BK7258_NO_HW_INIT.
 */

#define DT_DRV_COMPAT beken_bk7258_uart

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/init.h>

#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_bk7258, CONFIG_SERIAL_LOG_LEVEL);

/*
 * Configuration and runtime data
 */

struct uart_bk7258_config {
	uintptr_t base;
	uint32_t clock_freq;
	uint32_t baud;
	uint32_t tx_pin;
	uint32_t rx_pin;
	uint32_t tx_func;
	uint32_t rx_func;
	bool use_apll;
};

struct uart_bk7258_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;
	void *cb_data;
#endif
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uart_config current;
#endif
};

/*
 * Internal helpers
 */

static inline soc_uart_hw_t *uart_bk7258_hw(const struct device *dev)
{
	return (soc_uart_hw_t *)((const struct uart_bk7258_config *)dev->config)->base;
}

/**
 * @brief Put one pin into peripheral mux mode.
 *
 * Two-level mechanism:
 * level 1: pin configuration word bit 6, gpio_2_func_en = 1
 * level 2: seven words from 0x440100C0, 4-bit function number per pin
 *
 * The vendor uart_init_gpio() pulls up both TX and RX; same here.
 */
static void uart_bk7258_pin_setup(uint32_t pin, uint32_t func)
{
	uint32_t cfg_addr = SOC_GPIO_PIN_CFG_ADDR(pin);
	uint32_t cfg;

	/* Level 1: enable the secondary function and the pull-up, so a floating
	 * pin cannot fake a start bit.
	 */
	cfg = sys_read32(cfg_addr);
	cfg |= SOC_GPIO_CFG_2_FUNC_EN_BIT;
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
 * 115200 at 26 MHz -> 224 (actually 115555.6, an error of +0.31%)
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
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);
	uint32_t val;
	int i;

	LOG_DBG("init base=%#lx baud=%u clk=%u", (unsigned long)cfg->base, cfg->baud,
		cfg->clock_freq);

	/*
	 * When the UART is shared with the system controller, not a single register is written.
	 *
	 * Measured: UART0 belongs to the system controller (that core's console, and also where
	 * this core's log ends up once it is forwarded over the mailbox). A soft
	 * reset, a pin change or a new clk_div here truncates the system controller's log on
	 * the spot and the console then goes completely quiet: it looks like the
	 * board died, when in fact this core killed the UART.
	 * See UART_BK7258_NO_HW_INIT in Kconfig.
	 */
	if (IS_ENABLED(CONFIG_UART_BK7258_NO_HW_INIT)) {
		LOG_INF("sharing UART%p with the system controller: skipping hardware "
			"init, keeping its configuration (assumed %u 8N1)",
			(void *)cfg->base, cfg->baud);
		goto done;
	}

	/* --- 1. clock source (before enabling tx/rx) --- */
	uart_bk7258_clock_select(cfg->use_apll);

	/* --- 2. soft reset --- */
	hw->global_ctrl = SOC_UART_GLOBAL_SOFT_RESET_BIT;
	/* Wait for the reset bit to clear itself; force it after 1000 iterations
	 * if the hardware does not.
	 */
	for (i = 0; i < 1000; i++) {
		if ((hw->global_ctrl & SOC_UART_GLOBAL_SOFT_RESET_BIT) == 0U) {
			break;
		}
	}
	hw->global_ctrl &= ~SOC_UART_GLOBAL_SOFT_RESET_BIT;
	/* bypass the clock gate */
	hw->global_ctrl |= SOC_UART_GLOBAL_CLK_GATE_BYPASS_BIT;

	/* --- 3. pin muxing (pull-up on both TX and RX) --- */
	uart_bk7258_pin_setup(cfg->tx_pin, cfg->tx_func);
	uart_bk7258_pin_setup(cfg->rx_pin, cfg->rx_func);

	/* --- 4. interrupts: latch nothing, so polled mode stays polled. The
	 * interrupt API arms individual bits later; anything BL2 left pending is
	 * dropped here (write 1 to clear).
	 */
	hw->int_enable = 0U;
	hw->int_status = SOC_UART_INT_ALL_MASK;

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

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/*
 * Interrupt-driven API
 *
 * There is one interrupt line per UART and the status/enable register pair
 * lives inside the UART, so the vector says nothing about which direction
 * fired. Direction is recovered by masking int_status with int_enable -- the
 * same thing the vendor uart_isr_common() does. Both registers are shared with
 * the system controller firmware on a shared port, so every enable bit is read-modify-written
 * and only the bits this driver owns are touched; nothing here ever writes a
 * whole register.
 */

/* Conditions this driver subscribes to, and the ones it only reports. */
#define UART_BK7258_INT_RX_BITS   (SOC_UART_INT_RX_NEED_READ_BIT | SOC_UART_INT_RX_FINISH_BIT)
#define UART_BK7258_INT_DATA_BITS (SOC_UART_INT_TX_NEED_WRITE_BIT | UART_BK7258_INT_RX_BITS)
#define UART_BK7258_INT_ERR_BITS                                                                   \
	(SOC_UART_INT_RX_OVERFLOW_BIT | SOC_UART_INT_RX_PARITY_ERR_BIT |                           \
	 SOC_UART_INT_RX_STOP_ERR_BIT)

static int uart_bk7258_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);
	int n = 0;

	/* fifo_status bit 20 = fifo_wr_ready: room for at least one more byte */
	while (n < len && (hw->fifo_status & SOC_UART_FIFO_STATUS_WR_READY_BIT) != 0U) {
		hw->fifo_port = (uint32_t)tx_data[n] & SOC_UART_FIFO_PORT_TX_DATA_MASK;
		n++;
	}

	return n;
}

static int uart_bk7258_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);
	int n = 0;

	/* fifo_status bit 21 = fifo_rd_ready, the byte sits in fifo_port [15:8] */
	while (n < size && (hw->fifo_status & SOC_UART_FIFO_STATUS_RD_READY_BIT) != 0U) {
		rx_data[n] = (uint8_t)((hw->fifo_port >> SOC_UART_FIFO_PORT_RX_DATA_SHIFT) & 0xFFU);
		n++;
	}

	return n;
}

static void uart_bk7258_irq_tx_enable(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	hw->int_enable |= SOC_UART_INT_TX_NEED_WRITE_BIT;
}

static void uart_bk7258_irq_tx_disable(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	hw->int_enable &= ~SOC_UART_INT_TX_NEED_WRITE_BIT;

	/* int_status is latched, not level: drop what is still pending, or the
	 * next irq_tx_enable() fires immediately for a transfer already done.
	 */
	hw->int_status = SOC_UART_INT_TX_NEED_WRITE_BIT;
}

/**
 * @brief The FIFO has room for more, so the callback can push bytes.
 */
static int uart_bk7258_irq_tx_ready(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	return ((hw->int_enable & SOC_UART_INT_TX_NEED_WRITE_BIT) != 0U &&
		(hw->fifo_status & SOC_UART_FIFO_STATUS_WR_READY_BIT) != 0U)
		       ? 1
		       : 0;
}

/**
 * @brief Everything written to the FIFO has left the shift register.
 */
static int uart_bk7258_irq_tx_complete(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	return (hw->fifo_status & SOC_UART_FIFO_STATUS_TX_EMPTY_BIT) != 0U ? 1 : 0;
}

static void uart_bk7258_irq_rx_enable(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	/*
	 * rx_fifo_need_read: the RX FIFO reached its threshold.
	 * rx_finish: no byte arrived for rx_stop_detect_time (fifo_config
	 * [17:16]). Without the second one a short trailing packet would sit in
	 * the FIFO until the next burst pushes it over the threshold.
	 */
	hw->int_enable |= UART_BK7258_INT_RX_BITS;
}

static void uart_bk7258_irq_rx_disable(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	hw->int_enable &= ~UART_BK7258_INT_RX_BITS;
}

static int uart_bk7258_irq_rx_ready(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	return (hw->fifo_status & SOC_UART_FIFO_STATUS_RD_READY_BIT) != 0U ? 1 : 0;
}

static void uart_bk7258_irq_err_enable(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	hw->int_enable |= UART_BK7258_INT_ERR_BITS;
}

static void uart_bk7258_irq_err_disable(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	hw->int_enable &= ~UART_BK7258_INT_ERR_BITS;
}

static int uart_bk7258_irq_is_pending(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);

	return (hw->int_status & hw->int_enable) != 0U ? 1 : 0;
}

static void uart_bk7258_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Nothing to latch. The error bits live in int_status and stay readable
	 * until uart_bk7258_err_check() clears them, which can happen at any
	 * time, not only inside the ISR.
	 */
}

static void uart_bk7258_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *user_data)
{
	struct uart_bk7258_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;
}

static void uart_bk7258_isr(const struct device *dev)
{
	soc_uart_hw_t *hw = uart_bk7258_hw(dev);
	struct uart_bk7258_data *data = dev->data;
	uint32_t pending;

	/* Mask with int_enable first: on a shared port the system controller firmware may have
	 * subscribed to bits this driver did not ask for, and this ISR must not
	 * consume its interrupt.
	 */
	pending = hw->int_status & hw->int_enable &
		  (UART_BK7258_INT_DATA_BITS | UART_BK7258_INT_ERR_BITS);
	if (pending == 0U) {
		return;
	}

	/* Data conditions are consumed here (write 1 to clear). Error conditions
	 * are deliberately left latched: uart_err_check() is what reports them,
	 * and it clears them on the way out.
	 */
	hw->int_status = pending & UART_BK7258_INT_DATA_BITS;

	if (data->cb != NULL) {
		data->cb(dev, data->cb_data);
		return;
	}

	if ((pending & UART_BK7258_INT_ERR_BITS) != 0U) {
		/* No callback to report through, and an enabled error interrupt
		 * would otherwise keep the line asserted and spin the ISR.
		 */
		hw->int_status = pending & UART_BK7258_INT_ERR_BITS;
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_bk7258_driver_api) = {
	.poll_out = uart_bk7258_poll_out,
	.poll_in = uart_bk7258_poll_in,
	.err_check = uart_bk7258_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_bk7258_configure,
	.config_get = uart_bk7258_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_bk7258_fifo_fill,
	.fifo_read = uart_bk7258_fifo_read,
	.irq_tx_enable = uart_bk7258_irq_tx_enable,
	.irq_tx_disable = uart_bk7258_irq_tx_disable,
	.irq_tx_ready = uart_bk7258_irq_tx_ready,
	.irq_tx_complete = uart_bk7258_irq_tx_complete,
	.irq_rx_enable = uart_bk7258_irq_rx_enable,
	.irq_rx_disable = uart_bk7258_irq_rx_disable,
	.irq_rx_ready = uart_bk7258_irq_rx_ready,
	.irq_err_enable = uart_bk7258_irq_err_enable,
	.irq_err_disable = uart_bk7258_irq_err_disable,
	.irq_is_pending = uart_bk7258_irq_is_pending,
	.irq_update = uart_bk7258_irq_update,
	.irq_callback_set = uart_bk7258_irq_callback_set,
#endif
};

/*
 * Instantiation
 */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/*
 * The interrupt is two-level on this SoC: the NVIC line plus a second-level
 * enable in the SYSTEM domain. IRQ_CONNECT alone is not enough -- without the
 * system bit the NVIC looks clean and the callback simply never arrives
 * (measured on the GPIO driver first, then again here).
 *
 * Every UART IRQ on this SoC is below 32 (UART0 = 4, UART1 = 15, UART2 = 16),
 * so the lower half of the split enable register is the one to use.
 */
#define UART_BK7258_IRQ_CONNECT(n)                                                                 \
	BUILD_ASSERT(DT_INST_IRQN(n) < 32, "UART IRQ must be in the SYSTEM low half");             \
                                                                                                   \
	static void uart_bk7258_irq_link_##n(void)                                                 \
	{                                                                                          \
		sys_write32(sys_read32(SOC_SYS_CPU1_INT_0_31_EN_ADDR) | BIT(DT_INST_IRQN(n)),      \
			    SOC_SYS_CPU1_INT_0_31_EN_ADDR);                                        \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_bk7258_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define UART_BK7258_IRQ_LINK(n) uart_bk7258_irq_link_##n();

#else

#define UART_BK7258_IRQ_CONNECT(n)
#define UART_BK7258_IRQ_LINK(n)

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_BK7258_INIT(n)                                                                        \
	UART_BK7258_IRQ_CONNECT(n)                                                                 \
                                                                                                   \
	static int uart_bk7258_init_##n(const struct device *dev)                                  \
	{                                                                                          \
		UART_BK7258_IRQ_LINK(n)                                                            \
		return uart_bk7258_init(dev);                                                      \
	}                                                                                          \
                                                                                                   \
	static struct uart_bk7258_data uart_bk7258_data_##n;                                       \
                                                                                                   \
	static const struct uart_bk7258_config uart_bk7258_cfg_##n = {                             \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clock_freq = DT_INST_PROP(n, clock_frequency),                                    \
		.baud = DT_INST_PROP(n, current_speed),                                            \
		.tx_pin = DT_INST_PROP(n, tx_pin),                                                 \
		.rx_pin = DT_INST_PROP(n, rx_pin),                                                 \
		.tx_func = DT_INST_PROP(n, tx_func),                                               \
		.rx_func = DT_INST_PROP(n, rx_func),                                               \
		.use_apll = DT_INST_ENUM_IDX(n, clock_source) == 1,                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_bk7258_init_##n, NULL, &uart_bk7258_data_##n,                \
			      &uart_bk7258_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,     \
			      &uart_bk7258_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_BK7258_INIT)
