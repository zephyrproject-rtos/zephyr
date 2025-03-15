/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * Based on uart_mcux_lpuart.c, which is:
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_xps_uartlite_1_00_a

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

/* AXI UART Lite v2 registers offsets (See Xilinx PG142 for details) */
#define RX_FIFO_OFFSET  0x00
#define TX_FIFO_OFFSET  0x04
#define STAT_REG_OFFSET 0x08
#define CTRL_REG_OFFSET 0x0c

/* STAT_REG bit definitions */
#define STAT_REG_RX_FIFO_VALID_DATA BIT(0)
#define STAT_REG_RX_FIFO_FULL       BIT(1)
#define STAT_REG_TX_FIFO_EMPTY      BIT(2)
#define STAT_REG_TX_FIFO_FULL       BIT(3)
#define STAT_REG_INTR_ENABLED       BIT(4)
#define STAT_REG_OVERRUN_ERROR      BIT(5)
#define STAT_REG_FRAME_ERROR        BIT(6)
#define STAT_REG_PARITY_ERROR       BIT(7)

/* STAT_REG bit masks */
#define STAT_REG_ERROR_MASK GENMASK(7, 5)

/* CTRL_REG bit definitions */
#define CTRL_REG_RST_TX_FIFO BIT(0)
#define CTRL_REG_RST_RX_FIFO BIT(1)
#define CTRL_REG_ENABLE_INTR BIT(4)

struct xlnx_uartlite_config {
	mm_reg_t base;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct xlnx_uartlite_data {
	uint32_t errors;

	/* spinlocks for RX and TX FIFO preventing a bus error */
	struct k_spinlock rx_lock;
	struct k_spinlock tx_lock;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct device *dev;
	struct k_timer timer;
	uart_irq_callback_user_data_t callback;
	void *callback_data;

	volatile uint8_t tx_irq_enabled : 1;
	volatile uint8_t rx_irq_enabled : 1;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static inline uint32_t xlnx_uartlite_read_status(const struct device *dev)
{
	const struct xlnx_uartlite_config *config = dev->config;
	struct xlnx_uartlite_data *data = dev->data;
	uint32_t status;

	/* Cache errors as they are cleared by reading the STAT_REG */
	status = sys_read32(config->base + STAT_REG_OFFSET);
	data->errors &= (status & STAT_REG_ERROR_MASK);

	/* Return current status and previously cached errors */
	return status | data->errors;
}

static inline void xlnx_uartlite_clear_status(const struct device *dev)
{
	struct xlnx_uartlite_data *data = dev->data;

	/* Clear cached errors */
	data->errors = 0;
}

static inline unsigned char xlnx_uartlite_read_rx_fifo(const struct device *dev)
{
	const struct xlnx_uartlite_config *config = dev->config;

	return (sys_read32(config->base + RX_FIFO_OFFSET) & BIT_MASK(8));
}

static inline void xlnx_uartlite_write_tx_fifo(const struct device *dev,
					       unsigned char c)
{
	const struct xlnx_uartlite_config *config = dev->config;

	sys_write32((uint32_t)c, config->base + TX_FIFO_OFFSET);
}

static int xlnx_uartlite_poll_in(const struct device *dev, unsigned char *c)
{
	uint32_t status;
	k_spinlock_key_t key;
	struct xlnx_uartlite_data *data = dev->data;
	int ret = -1;

	key = k_spin_lock(&data->rx_lock);
	status = xlnx_uartlite_read_status(dev);
	if ((status & STAT_REG_RX_FIFO_VALID_DATA) != 0) {
		*c = xlnx_uartlite_read_rx_fifo(dev);
		ret = 0;
	}
	k_spin_unlock(&data->rx_lock, key);

	return ret;
}

static void xlnx_uartlite_poll_out(const struct device *dev, unsigned char c)
{
	uint32_t status;
	k_spinlock_key_t key;
	struct xlnx_uartlite_data *data = dev->data;
	bool done = false;

	while (!done) {
		key = k_spin_lock(&data->tx_lock);
		status = xlnx_uartlite_read_status(dev);
		if ((status & STAT_REG_TX_FIFO_FULL) == 0) {
			xlnx_uartlite_write_tx_fifo(dev, c);
			done = true;
		}
		k_spin_unlock(&data->tx_lock, key);
	}
}

static int xlnx_uartlite_err_check(const struct device *dev)
{
	uint32_t status = xlnx_uartlite_read_status(dev);
	int err = 0;

	if (status & STAT_REG_OVERRUN_ERROR) {
		err |= UART_ERROR_OVERRUN;
	}

	if (status & STAT_REG_PARITY_ERROR) {
		err |= UART_ERROR_PARITY;
	}

	if (status & STAT_REG_FRAME_ERROR) {
		err |= UART_ERROR_FRAMING;
	}

	xlnx_uartlite_clear_status(dev);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static inline void xlnx_uartlite_irq_enable(const struct device *dev)
{
	const struct xlnx_uartlite_config *config = dev->config;

	sys_write32(CTRL_REG_ENABLE_INTR, config->base + CTRL_REG_OFFSET);
}

static inline void xlnx_uartlite_irq_cond_disable(const struct device *dev)
{
	const struct xlnx_uartlite_config *config = dev->config;
	struct xlnx_uartlite_data *data = dev->data;

	/* TX and RX IRQs are shared. Only disable if both are disabled. */
	if (!data->tx_irq_enabled && !data->rx_irq_enabled) {
		sys_write32(0, config->base + CTRL_REG_OFFSET);
	}
}

static int xlnx_uartlite_fifo_fill(const struct device *dev,
				   const uint8_t *tx_data,
				   int len)
{
	uint32_t status;
	k_spinlock_key_t key;
	struct xlnx_uartlite_data *data = dev->data;
	int count = 0U;

	while (len - count > 0) {
		key = k_spin_lock(&data->tx_lock);
		status = xlnx_uartlite_read_status(dev);
		if ((status & STAT_REG_TX_FIFO_FULL) == 0U) {
			xlnx_uartlite_write_tx_fifo(dev, tx_data[count++]);
		}
		k_spin_unlock(&data->tx_lock, key);
	}

	return count;
}

static int xlnx_uartlite_fifo_read(const struct device *dev, uint8_t *rx_data,
				   const int len)
{
	uint32_t status;
	k_spinlock_key_t key;
	struct xlnx_uartlite_data *data = dev->data;
	int count = 0U;

	while ((len - count) > 0) {
		key = k_spin_lock(&data->rx_lock);
		status = xlnx_uartlite_read_status(dev);
		if ((status & STAT_REG_RX_FIFO_VALID_DATA) != 0) {
			rx_data[count++] = xlnx_uartlite_read_rx_fifo(dev);
		}
		k_spin_unlock(&data->rx_lock, key);
		if (!(status & STAT_REG_RX_FIFO_VALID_DATA)) {
			break;
		}
	}

	return count;
}

static void xlnx_uartlite_tx_soft_isr(struct k_timer *timer)
{
	struct xlnx_uartlite_data *data =
		CONTAINER_OF(timer, struct xlnx_uartlite_data, timer);

	if (data->callback) {
		data->callback(data->dev, data->callback_data);
	}
}

static void xlnx_uartlite_irq_tx_enable(const struct device *dev)
{
	struct xlnx_uartlite_data *data = dev->data;
	uint32_t status;

	data->tx_irq_enabled = true;
	status = xlnx_uartlite_read_status(dev);
	xlnx_uartlite_irq_enable(dev);

	if ((status & STAT_REG_TX_FIFO_EMPTY) && data->callback) {
		/*
		 * TX_FIFO_EMPTY event already generated an edge
		 * interrupt. Generate a soft interrupt and have it call the
		 * callback function in timer isr context.
		 */
		k_timer_start(&data->timer, K_NO_WAIT, K_NO_WAIT);
	}
}

static void xlnx_uartlite_irq_tx_disable(const struct device *dev)
{
	struct xlnx_uartlite_data *data = dev->data;

	data->tx_irq_enabled = false;
	xlnx_uartlite_irq_cond_disable(dev);
}

static int xlnx_uartlite_irq_tx_ready(const struct device *dev)
{
	struct xlnx_uartlite_data *data = dev->data;
	uint32_t status = xlnx_uartlite_read_status(dev);

	return (((status & STAT_REG_TX_FIFO_FULL) == 0U) &&
		data->tx_irq_enabled);
}

static int xlnx_uartlite_irq_tx_complete(const struct device *dev)
{
	uint32_t status = xlnx_uartlite_read_status(dev);

	return (status & STAT_REG_TX_FIFO_EMPTY);
}

static void xlnx_uartlite_irq_rx_enable(const struct device *dev)
{
	struct xlnx_uartlite_data *data = dev->data;

	data->rx_irq_enabled = true;
	/* RX_FIFO_VALID_DATA generates a level interrupt */
	xlnx_uartlite_irq_enable(dev);
}

static void xlnx_uartlite_irq_rx_disable(const struct device *dev)
{
	struct xlnx_uartlite_data *data = dev->data;

	data->rx_irq_enabled = false;
	xlnx_uartlite_irq_cond_disable(dev);
}

static int xlnx_uartlite_irq_rx_ready(const struct device *dev)
{
	struct xlnx_uartlite_data *data = dev->data;
	uint32_t status = xlnx_uartlite_read_status(dev);

	return ((status & STAT_REG_RX_FIFO_VALID_DATA) &&
		data->rx_irq_enabled);
}

static int xlnx_uartlite_irq_is_pending(const struct device *dev)
{
	return (xlnx_uartlite_irq_tx_ready(dev) ||
		xlnx_uartlite_irq_rx_ready(dev));
}

static int xlnx_uartlite_irq_update(const struct device *dev)
{
	return 1;
}

static void xlnx_uartlite_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb,
					   void *user_data)
{
	struct xlnx_uartlite_data *data = dev->data;

	data->callback = cb;
	data->callback_data = user_data;
}

static __unused void xlnx_uartlite_isr(const struct device *dev)
{
	struct xlnx_uartlite_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->callback_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int xlnx_uartlite_init(const struct device *dev)
{
	const struct xlnx_uartlite_config *config = dev->config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct xlnx_uartlite_data *data = dev->data;

	data->dev = dev;
	k_timer_init(&data->timer, &xlnx_uartlite_tx_soft_isr, NULL);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	/* Reset FIFOs and disable interrupts */
	sys_write32(CTRL_REG_RST_RX_FIFO | CTRL_REG_RST_TX_FIFO,
		    config->base + CTRL_REG_OFFSET);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

static DEVICE_API(uart, xlnx_uartlite_driver_api) = {
	.poll_in = xlnx_uartlite_poll_in,
	.poll_out = xlnx_uartlite_poll_out,
	.err_check = xlnx_uartlite_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = xlnx_uartlite_fifo_fill,
	.fifo_read = xlnx_uartlite_fifo_read,
	.irq_tx_enable = xlnx_uartlite_irq_tx_enable,
	.irq_tx_disable = xlnx_uartlite_irq_tx_disable,
	.irq_tx_ready = xlnx_uartlite_irq_tx_ready,
	.irq_tx_complete = xlnx_uartlite_irq_tx_complete,
	.irq_rx_enable = xlnx_uartlite_irq_rx_enable,
	.irq_rx_disable = xlnx_uartlite_irq_rx_disable,
	.irq_rx_ready = xlnx_uartlite_irq_rx_ready,
	.irq_is_pending = xlnx_uartlite_irq_is_pending,
	.irq_update = xlnx_uartlite_irq_update,
	.irq_callback_set = xlnx_uartlite_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define XLNX_UARTLITE_IRQ_INIT(n, i)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, i),			\
			    DT_INST_IRQ_BY_IDX(n, i, priority),		\
			    xlnx_uartlite_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN_BY_IDX(n, i));			\
	} while (false)
#define XLNX_UARTLITE_CONFIG_FUNC(n)					\
	static void xlnx_uartlite_config_func_##n(const struct device *dev) \
	{								\
		/* IRQ line not always present on all instances */	\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0),			\
			   (XLNX_UARTLITE_IRQ_INIT(n, 0);))		\
	}
#define XLNX_UARTLITE_IRQ_CFG_FUNC_INIT(n)				\
	.irq_config_func = xlnx_uartlite_config_func_##n
#define XLNX_UARTLITE_INIT_CFG(n)					\
	XLNX_UARTLITE_DECLARE_CFG(n, XLNX_UARTLITE_IRQ_CFG_FUNC_INIT(n))
#else
#define XLNX_UARTLITE_CONFIG_FUNC(n)
#define XLNX_UARTLITE_IRQ_CFG_FUNC_INIT
#define XLNX_UARTLITE_INIT_CFG(n)					\
	XLNX_UARTLITE_DECLARE_CFG(n, XLNX_UARTLITE_IRQ_CFG_FUNC_INIT)
#endif

#define XLNX_UARTLITE_DECLARE_CFG(n, IRQ_FUNC_INIT)			\
static const struct xlnx_uartlite_config xlnx_uartlite_##n##_config = {	\
	.base = DT_INST_REG_ADDR(n),					\
	IRQ_FUNC_INIT							\
}

#define XLNX_UARTLITE_INIT(n)						\
	static struct xlnx_uartlite_data xlnx_uartlite_##n##_data;	\
									\
	static const struct xlnx_uartlite_config xlnx_uartlite_##n##_config;\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &xlnx_uartlite_init,			\
			    NULL,					\
			    &xlnx_uartlite_##n##_data,			\
			    &xlnx_uartlite_##n##_config,		\
			    PRE_KERNEL_1,				\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &xlnx_uartlite_driver_api);			\
									\
	XLNX_UARTLITE_CONFIG_FUNC(n)					\
									\
	XLNX_UARTLITE_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(XLNX_UARTLITE_INIT)
