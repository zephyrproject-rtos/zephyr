/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT beken_bk7258_uart

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <soc.h>

/* Register offsets from the block base address */
#define BK_UART_GLOBAL_CTRL      0x08
#define BK_UART_CONFIG           0x10
#define BK_UART_FIFO_CONFIG      0x14
#define BK_UART_FIFO_STATUS      0x18
#define BK_UART_FIFO_PORT        0x1c
#define BK_UART_INT_ENABLE       0x20
#define BK_UART_INT_STATUS       0x24
#define BK_UART_FLOW_CTRL_CONFIG 0x28
#define BK_UART_WAKE_CONFIG      0x2c

/* BK_UART_GLOBAL_CTRL: the block is held in reset while this bit is clear */
#define BK_UART_GLOBAL_CTRL_SOFT_RESET_N BIT(0)

/* BK_UART_CONFIG */
#define BK_UART_CONFIG_TX_ENABLE  BIT(0)
#define BK_UART_CONFIG_RX_ENABLE  BIT(1)
#define BK_UART_CONFIG_DATA_BITS  GENMASK(4, 3)
#define BK_UART_CONFIG_PARITY_EN  BIT(5)
#define BK_UART_CONFIG_PARITY     BIT(6)
#define BK_UART_CONFIG_STOP_BITS  BIT(7)
#define BK_UART_CONFIG_CLK_DIV    GENMASK(23, 8)

/* BK_UART_CONFIG_DATA_BITS: number of data bits per character */
#define BK_UART_DATA_BITS_5 0U
#define BK_UART_DATA_BITS_6 1U
#define BK_UART_DATA_BITS_7 2U
#define BK_UART_DATA_BITS_8 3U

/*
 * BK_UART_CONFIG_PARITY, as the vendor SDK's uart_reg.h has it; the comment in
 * its uart_struct.h has the two reversed.
 */
#define BK_UART_PARITY_ODD  0U
#define BK_UART_PARITY_EVEN 1U

/*
 * BK_UART_FIFO_CONFIG, set to the vendor SDK's values: 32 bytes for TX, 64 bytes for RX
 * IDLE timeout: 32 bit.
 */
#define BK_UART_FIFO_CONFIG_TX_THRESHOLD GENMASK(7, 0)
#define BK_UART_FIFO_CONFIG_RX_THRESHOLD GENMASK(15, 8)
#define BK_UART_FIFO_CONFIG_RX_IDLE      GENMASK(17, 16)
#define BK_UART_TX_THRESHOLD             32U
#define BK_UART_RX_THRESHOLD             64U

/* BK_UART_FIFO_CONFIG_RX_IDLE: how long the line stays idle before RX_FINISH */
#define BK_UART_RX_IDLE_32_BITS  0U
#define BK_UART_RX_IDLE_64_BITS  1U
#define BK_UART_RX_IDLE_128_BITS 2U
#define BK_UART_RX_IDLE_256_BITS 3U

/* BK_UART_FIFO_STATUS */
#define BK_UART_FIFO_STATUS_TX_EMPTY BIT(17)
#define BK_UART_FIFO_STATUS_WR_READY BIT(20)
#define BK_UART_FIFO_STATUS_RD_READY BIT(21)

/* BK_UART_FIFO_PORT */
#define BK_UART_FIFO_PORT_TX_DATA GENMASK(7, 0)
#define BK_UART_FIFO_PORT_RX_DATA GENMASK(15, 8)

/*
 * BK_UART_INT_ENABLE and BK_UART_INT_STATUS.
 */
#define BK_UART_INT_TX_NEED_WRITE BIT(0)
#define BK_UART_INT_RX_NEED_READ  BIT(1)
#define BK_UART_INT_RX_FINISH     BIT(6)
#define BK_UART_INT_ALL           GENMASK(7, 0)

/*
 * The divider counts one baud period as (clk_div + 1) source clocks. The
 * vendor SDK limits it to this range, rather than to the field's 16 bits.
 */
#define BK_UART_CLK_DIV_MIN 4U
#define BK_UART_CLK_DIV_MAX 0x1fffU

struct uart_bk7258_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct uart_bk7258_data {
	/* Line settings in effect, initially those of the devicetree */
	struct uart_config uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct k_spinlock lock;
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

/*
 * Translate line settings into BK_UART_CONFIG bits. The block
 * supports 5 to 8 data bits, no, odd or even parity, and one or two stop bits.
 */
static int uart_bk7258_line_bits(const struct uart_config *cfg, uint32_t *bits)
{
	uint32_t val = 0U;

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		val |= FIELD_PREP(BK_UART_CONFIG_DATA_BITS, BK_UART_DATA_BITS_5);
		break;
	case UART_CFG_DATA_BITS_6:
		val |= FIELD_PREP(BK_UART_CONFIG_DATA_BITS, BK_UART_DATA_BITS_6);
		break;
	case UART_CFG_DATA_BITS_7:
		val |= FIELD_PREP(BK_UART_CONFIG_DATA_BITS, BK_UART_DATA_BITS_7);
		break;
	case UART_CFG_DATA_BITS_8:
		val |= FIELD_PREP(BK_UART_CONFIG_DATA_BITS, BK_UART_DATA_BITS_8);
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_ODD:
		val |= BK_UART_CONFIG_PARITY_EN |
		       FIELD_PREP(BK_UART_CONFIG_PARITY, BK_UART_PARITY_ODD);
		break;
	case UART_CFG_PARITY_EVEN:
		val |= BK_UART_CONFIG_PARITY_EN |
		       FIELD_PREP(BK_UART_CONFIG_PARITY, BK_UART_PARITY_EVEN);
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		break;
	case UART_CFG_STOP_BITS_2:
		val |= BK_UART_CONFIG_STOP_BITS;
		break;
	default:
		return -ENOTSUP;
	}

	*bits = val;

	return 0;
}

/* Program the line settings and the baud rate */
static int uart_bk7258_set_line(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_bk7258_config *config = dev->config;
	uint32_t clock_frequency;
	uint32_t line_bits;
	uint32_t clk_div;
	int err;

	if ((cfg->baudrate == 0U) || (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE)) {
		return -ENOTSUP;
	}

	err = uart_bk7258_line_bits(cfg, &line_bits);
	if (err < 0) {
		return err;
	}

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_frequency);
	if (err < 0) {
		return err;
	}

	clk_div = DIV_ROUND_CLOSEST(clock_frequency, cfg->baudrate) - 1U;
	if ((clk_div < BK_UART_CLK_DIV_MIN) || (clk_div > BK_UART_CLK_DIV_MAX)) {
		return -ENOTSUP;
	}

	sys_write32(line_bits | FIELD_PREP(BK_UART_CONFIG_CLK_DIV, clk_div) |
		    BK_UART_CONFIG_TX_ENABLE | BK_UART_CONFIG_RX_ENABLE,
		    config->base + BK_UART_CONFIG);

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_bk7258_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct uart_bk7258_data *data = dev->data;
	int err;

	err = uart_bk7258_set_line(dev, cfg);
	if (err < 0) {
		return err;
	}

	data->uart_cfg = *cfg;

	return 0;
}

static int uart_bk7258_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_bk7258_data *data = dev->data;

	*cfg = data->uart_cfg;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_bk7258_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_bk7258_config *config = dev->config;

	if ((sys_read32(config->base + BK_UART_FIFO_STATUS) &
	     BK_UART_FIFO_STATUS_RD_READY) == 0U) {
		return -1;
	}

	*c = (unsigned char)FIELD_GET(BK_UART_FIFO_PORT_RX_DATA,
				      sys_read32(config->base + BK_UART_FIFO_PORT));

	return 0;
}

static void uart_bk7258_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_bk7258_config *config = dev->config;

	while ((sys_read32(config->base + BK_UART_FIFO_STATUS) &
		BK_UART_FIFO_STATUS_WR_READY) == 0U) {
	}

	sys_write32(FIELD_PREP(BK_UART_FIFO_PORT_TX_DATA, c),
		    config->base + BK_UART_FIFO_PORT);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_bk7258_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_bk7258_config *config = dev->config;
	int i;

	for (i = 0; i < len; i++) {
		if ((sys_read32(config->base + BK_UART_FIFO_STATUS) &
		     BK_UART_FIFO_STATUS_WR_READY) == 0U) {
			break;
		}

		sys_write32(FIELD_PREP(BK_UART_FIFO_PORT_TX_DATA, tx_data[i]),
			    config->base + BK_UART_FIFO_PORT);
	}

	return i;
}

static int uart_bk7258_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_bk7258_config *config = dev->config;
	int i;

	for (i = 0; i < size; i++) {
		if ((sys_read32(config->base + BK_UART_FIFO_STATUS) &
		     BK_UART_FIFO_STATUS_RD_READY) == 0U) {
			break;
		}

		rx_data[i] = (uint8_t)FIELD_GET(BK_UART_FIFO_PORT_RX_DATA,
						 sys_read32(config->base + BK_UART_FIFO_PORT));
	}

	return i;
}

static void uart_bk7258_int_update(const struct device *dev, uint32_t mask, bool enable)
{
	const struct uart_bk7258_config *config = dev->config;
	struct uart_bk7258_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (enable) {
		sys_set_bits(config->base + BK_UART_INT_ENABLE, mask);
	} else {
		sys_clear_bits(config->base + BK_UART_INT_ENABLE, mask);
	}

	k_spin_unlock(&data->lock, key);
}

static bool uart_bk7258_int_is_enabled(const struct device *dev, uint32_t mask)
{
	const struct uart_bk7258_config *config = dev->config;

	return (sys_read32(config->base + BK_UART_INT_ENABLE) & mask) != 0U;
}

static void uart_bk7258_irq_tx_enable(const struct device *dev)
{
	uart_bk7258_int_update(dev, BK_UART_INT_TX_NEED_WRITE, true);
}

static void uart_bk7258_irq_tx_disable(const struct device *dev)
{
	uart_bk7258_int_update(dev, BK_UART_INT_TX_NEED_WRITE, false);
}

static int uart_bk7258_irq_is_tx_ready(const struct device *dev)
{
	const struct uart_bk7258_config *config = dev->config;

	return uart_bk7258_int_is_enabled(dev, BK_UART_INT_TX_NEED_WRITE) &&
	       ((sys_read32(config->base + BK_UART_FIFO_STATUS) &
		 BK_UART_FIFO_STATUS_WR_READY) != 0U);
}

/*
 * The transmitter's own completion flag can only be read while it raises an
 * interrupt, so this reports the FIFO empty: the last character may still be
 * shifting out.
 */
static int uart_bk7258_irq_is_tx_complete(const struct device *dev)
{
	const struct uart_bk7258_config *config = dev->config;

	return (sys_read32(config->base + BK_UART_FIFO_STATUS) &
		BK_UART_FIFO_STATUS_TX_EMPTY) != 0U;
}

static void uart_bk7258_rx_finish_ack(const struct device *dev)
{
	const struct uart_bk7258_config *config = dev->config;

	if ((sys_read32(config->base + BK_UART_FIFO_STATUS) & BK_UART_FIFO_STATUS_RD_READY) == 0U) {
		sys_write32(BK_UART_INT_RX_FINISH, config->base + BK_UART_INT_STATUS);
	}
}

static void uart_bk7258_irq_rx_enable(const struct device *dev)
{
	uart_bk7258_rx_finish_ack(dev);
	uart_bk7258_int_update(dev, BK_UART_INT_RX_NEED_READ | BK_UART_INT_RX_FINISH, true);
}

static void uart_bk7258_irq_rx_disable(const struct device *dev)
{
	uart_bk7258_int_update(dev, BK_UART_INT_RX_NEED_READ | BK_UART_INT_RX_FINISH, false);
}

static int uart_bk7258_irq_is_rx_ready(const struct device *dev)
{
	const struct uart_bk7258_config *config = dev->config;

	return (sys_read32(config->base + BK_UART_FIFO_STATUS) &
		BK_UART_FIFO_STATUS_RD_READY) != 0U;
}

static int uart_bk7258_irq_is_pending(const struct device *dev)
{
	return uart_bk7258_irq_is_tx_ready(dev) ||
	       (uart_bk7258_int_is_enabled(dev, BK_UART_INT_RX_NEED_READ) &&
		uart_bk7258_irq_is_rx_ready(dev));
}

static void uart_bk7258_irq_callback_set(const struct device *dev,
					 uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct uart_bk7258_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_bk7258_isr(const struct device *dev)
{
	struct uart_bk7258_data *data = dev->data;

	if (data->callback != NULL) {
		data->callback(dev, data->cb_data);
	}

	uart_bk7258_rx_finish_ack(dev);
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_bk7258_init(const struct device *dev)
{
	const struct uart_bk7258_config *config = dev->config;
	struct uart_bk7258_data *data = dev->data;
	int err;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err < 0) {
		return err;
	}

	/*
	 * Release the block from soft reset, then program it as the vendor
	 * SDK does: interrupts off, FIFO thresholds, hardware flow control and
	 * wakeup off, and the line settings last.
	 */
	sys_write32(BK_UART_GLOBAL_CTRL_SOFT_RESET_N, config->base + BK_UART_GLOBAL_CTRL);
	sys_write32(0, config->base + BK_UART_INT_ENABLE);
	sys_write32(BK_UART_INT_ALL, config->base + BK_UART_INT_STATUS);
	sys_write32(FIELD_PREP(BK_UART_FIFO_CONFIG_TX_THRESHOLD, BK_UART_TX_THRESHOLD) |
		    FIELD_PREP(BK_UART_FIFO_CONFIG_RX_THRESHOLD, BK_UART_RX_THRESHOLD) |
		    FIELD_PREP(BK_UART_FIFO_CONFIG_RX_IDLE, BK_UART_RX_IDLE_32_BITS),
		    config->base + BK_UART_FIFO_CONFIG);
	sys_write32(0, config->base + BK_UART_FLOW_CTRL_CONFIG);
	sys_write32(0, config->base + BK_UART_WAKE_CONFIG);

	err = uart_bk7258_set_line(dev, &data->uart_cfg);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static DEVICE_API(uart, uart_bk7258_driver_api) = {
	.poll_in = uart_bk7258_poll_in,
	.poll_out = uart_bk7258_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_bk7258_configure,
	.config_get = uart_bk7258_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_bk7258_fifo_fill,
	.fifo_read = uart_bk7258_fifo_read,
	.irq_tx_enable = uart_bk7258_irq_tx_enable,
	.irq_tx_disable = uart_bk7258_irq_tx_disable,
	.irq_tx_ready = uart_bk7258_irq_is_tx_ready,
	.irq_tx_complete = uart_bk7258_irq_is_tx_complete,
	.irq_rx_enable = uart_bk7258_irq_rx_enable,
	.irq_rx_disable = uart_bk7258_irq_rx_disable,
	.irq_rx_ready = uart_bk7258_irq_is_rx_ready,
	.irq_is_pending = uart_bk7258_irq_is_pending,
	.irq_callback_set = uart_bk7258_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

#define UART_BK7258_IRQ_CONFIG(n)							\
	static void uart_bk7258_irq_config_##n(const struct device *dev)		\
	{										\
		ARG_UNUSED(dev);							\
											\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    uart_bk7258_isr, DEVICE_DT_INST_GET(n), 0);			\
		bk7258_irq_gate_enable(DT_INST_IRQN(n));				\
		irq_enable(DT_INST_IRQN(n));						\
	}
#define UART_BK7258_IRQ_CONFIG_INIT(n) .irq_config_func = uart_bk7258_irq_config_##n,
#else
#define UART_BK7258_IRQ_CONFIG(n)
#define UART_BK7258_IRQ_CONFIG_INIT(n)
#endif

/*
 * The devicetree's line settings, looked up by name: data-bits <7> selects
 * UART_CFG_DATA_BITS_7, parity "even" UART_CFG_PARITY_EVEN and stop-bits "2"
 * UART_CFG_STOP_BITS_2. Absent, they are 8 data bits, no parity, 1 stop bit.
 */
#define UART_BK7258_DT_DATA_BITS(n)							\
	UTIL_CAT(UART_CFG_DATA_BITS_, DT_INST_PROP_OR(n, data_bits, 8))
#define UART_BK7258_DT_PARITY(n)							\
	UTIL_CAT(UART_CFG_PARITY_, DT_INST_STRING_UPPER_TOKEN(n, parity))
#define UART_BK7258_DT_STOP_BITS(n)							\
	UTIL_CAT(UART_CFG_STOP_BITS_, DT_INST_STRING_UPPER_TOKEN_OR(n, stop_bits, 1))

/* Line settings the block cannot do are rejected when the devicetree is built */
#define UART_BK7258_CHECK_DT(n)								\
	BUILD_ASSERT((UART_BK7258_DT_DATA_BITS(n) == UART_CFG_DATA_BITS_5) ||		\
		     (UART_BK7258_DT_DATA_BITS(n) == UART_CFG_DATA_BITS_6) ||		\
		     (UART_BK7258_DT_DATA_BITS(n) == UART_CFG_DATA_BITS_7) ||		\
		     (UART_BK7258_DT_DATA_BITS(n) == UART_CFG_DATA_BITS_8),		\
		     "beken,bk7258-uart supports 5 to 8 data bits");			\
	BUILD_ASSERT((UART_BK7258_DT_PARITY(n) == UART_CFG_PARITY_NONE) ||		\
		     (UART_BK7258_DT_PARITY(n) == UART_CFG_PARITY_ODD) ||		\
		     (UART_BK7258_DT_PARITY(n) == UART_CFG_PARITY_EVEN),		\
		     "beken,bk7258-uart supports none, odd or even parity");		\
	BUILD_ASSERT((UART_BK7258_DT_STOP_BITS(n) == UART_CFG_STOP_BITS_1) ||		\
		     (UART_BK7258_DT_STOP_BITS(n) == UART_CFG_STOP_BITS_2),		\
		     "beken,bk7258-uart supports 1 or 2 stop bits");			\
	BUILD_ASSERT(!DT_INST_PROP(n, hw_flow_control),					\
		     "beken,bk7258-uart hardware flow control is not supported for now");

#define UART_BK7258_INIT(n)								\
	UART_BK7258_CHECK_DT(n)								\
	PINCTRL_DT_INST_DEFINE(n);							\
	UART_BK7258_IRQ_CONFIG(n)							\
											\
	static struct uart_bk7258_data uart_bk7258_data_##n = {				\
		.uart_cfg = {								\
			.baudrate = DT_INST_PROP(n, current_speed),			\
			.data_bits = UART_BK7258_DT_DATA_BITS(n),			\
			.parity = UART_BK7258_DT_PARITY(n),				\
			.stop_bits = UART_BK7258_DT_STOP_BITS(n),			\
			.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,				\
		},									\
	};										\
											\
	static const struct uart_bk7258_config uart_bk7258_config_##n = {		\
		.base = DT_INST_REG_ADDR(n),						\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, id),	\
		UART_BK7258_IRQ_CONFIG_INIT(n)						\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, uart_bk7258_init, NULL, &uart_bk7258_data_##n,	\
			      &uart_bk7258_config_##n, PRE_KERNEL_1,			\
			      CONFIG_SERIAL_INIT_PRIORITY,				\
			      &uart_bk7258_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_BK7258_INIT)
