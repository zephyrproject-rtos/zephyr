/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_uart0

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/types.h>

#include <soc.h>

#define UART_EV_TX		BIT(0)
#define UART_EV_RX		BIT(1)

struct uart_litex_device_config {
	uint32_t rxtx_addr;
	uint32_t txfull_addr;
	uint32_t rxempty_addr;
	uint32_t ev_status_addr;
	uint32_t ev_pending_addr;
	uint32_t ev_enable_addr;
	uint32_t txempty_addr;
	uint32_t rxfull_addr;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*config_func)(const struct device *dev);
#endif
};

struct uart_litex_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct k_timer timer;
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

/**
 * @brief Output a character in polled mode.
 *
 * Writes data to tx register. Waits for space if transmitter is full.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_litex_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_litex_device_config *config = dev->config;
	/* wait for space */
	while (litex_read8(config->txfull_addr)) {
	}

	litex_write8(c, config->rxtx_addr);
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_litex_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_litex_device_config *config = dev->config;

	if (!litex_read8(config->rxempty_addr)) {
		*c = litex_read8(config->rxtx_addr);

		/* refresh UART_RXEMPTY by writing UART_EV_RX
		 * to UART_EV_PENDING
		 */
		litex_write8(UART_EV_RX, config->ev_pending_addr);
		return 0;
	} else {
		return -1;
	}
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/**
 * @brief Enable TX interrupt in event register
 *
 * @param dev UART device struct
 */
static void uart_litex_irq_tx_enable(const struct device *dev)
{
	const struct uart_litex_device_config *config = dev->config;
	struct uart_litex_data *data = dev->data;

	uint8_t enable = litex_read8(config->ev_enable_addr);

	litex_write8(enable | UART_EV_TX, config->ev_enable_addr);

	if (!litex_read8(config->txfull_addr)) {
		/*
		 * TX done event already generated an edge interrupt. Generate a
		 * soft interrupt and have it call the callback function in
		 * timer isr context.
		 */
		k_timer_start(&data->timer, K_NO_WAIT, K_NO_WAIT);
	}
}

/**
 * @brief Disable TX interrupt in event register
 *
 * @param dev UART device struct
 */
static void uart_litex_irq_tx_disable(const struct device *dev)
{
	const struct uart_litex_device_config *config = dev->config;

	uint8_t enable = litex_read8(config->ev_enable_addr);

	litex_write8(enable & ~(UART_EV_TX), config->ev_enable_addr);
}

/**
 * @brief Enable RX interrupt in event register
 *
 * @param dev UART device struct
 */
static void uart_litex_irq_rx_enable(const struct device *dev)
{
	const struct uart_litex_device_config *config = dev->config;

	uint8_t enable = litex_read8(config->ev_enable_addr);

	litex_write8(enable | UART_EV_RX, config->ev_enable_addr);
}

/**
 * @brief Disable RX interrupt in event register
 *
 * @param dev UART device struct
 */
static void uart_litex_irq_rx_disable(const struct device *dev)
{
	const struct uart_litex_device_config *config = dev->config;

	uint8_t enable = litex_read8(config->ev_enable_addr);

	litex_write8(enable & ~(UART_EV_RX), config->ev_enable_addr);
}

/**
 * @brief Check if Tx IRQ has been raised and UART is ready to accept new data
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ has been raised, 0 otherwise
 */
static int uart_litex_irq_tx_ready(const struct device *dev)
{
	const struct uart_litex_device_config *config = dev->config;

	uint8_t val = litex_read8(config->txfull_addr);

	return !val;
}

/**
 * @brief Check if Rx IRQ has been raised and there's data to be read from UART
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ has been raised, 0 otherwise
 */
static int uart_litex_irq_rx_ready(const struct device *dev)
{
	const struct uart_litex_device_config *config = dev->config;
	uint8_t pending;

	pending = litex_read8(config->ev_pending_addr);

	if (pending & UART_EV_RX) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int uart_litex_fifo_fill(const struct device *dev,
				const uint8_t *tx_data, int size)
{
	const struct uart_litex_device_config *config = dev->config;
	int i;

	for (i = 0; i < size && !litex_read8(config->txfull_addr); i++) {
		litex_write8(tx_data[i], config->rxtx_addr);
	}

	return i;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct
 * @param rxData Data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static int uart_litex_fifo_read(const struct device *dev,
				uint8_t *rx_data, const int size)
{
	const struct uart_litex_device_config *config = dev->config;
	int i;

	for (i = 0; i < size && !litex_read8(config->rxempty_addr); i++) {
		rx_data[i] = litex_read8(config->rxtx_addr);

		/* refresh UART_RXEMPTY by writing UART_EV_RX
		 * to UART_EV_PENDING
		 */
		litex_write8(UART_EV_RX, config->ev_pending_addr);
	}

	return i;
}

static void uart_litex_irq_err(const struct device *dev)
{
	ARG_UNUSED(dev);
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_litex_irq_is_pending(const struct device *dev)
{
	return (uart_litex_irq_tx_ready(dev) || uart_litex_irq_rx_ready(dev));
}

static int uart_litex_irq_update(const struct device *dev)
{
	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 */
static void uart_litex_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_litex_data *data;

	data = dev->data;
	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_litex_irq_handler(const struct device *dev)
{
	const struct uart_litex_device_config *config = dev->config;
	struct uart_litex_data *data = dev->data;
	unsigned int key = irq_lock();

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}

	/* Clear RX events, TX events still needed to enqueue the next transfer */
	litex_write8(UART_EV_RX, config->ev_pending_addr);

	irq_unlock(key);
}

static void uart_litex_tx_soft_isr(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);

	uart_litex_irq_handler(dev);
}
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_litex_driver_api = {
	.poll_in		= uart_litex_poll_in,
	.poll_out		= uart_litex_poll_out,
	.err_check		= NULL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill		= uart_litex_fifo_fill,
	.fifo_read		= uart_litex_fifo_read,
	.irq_tx_enable		= uart_litex_irq_tx_enable,
	.irq_tx_disable		= uart_litex_irq_tx_disable,
	.irq_tx_ready		= uart_litex_irq_tx_ready,
	.irq_rx_enable		= uart_litex_irq_rx_enable,
	.irq_rx_disable		= uart_litex_irq_rx_disable,
	.irq_rx_ready		= uart_litex_irq_rx_ready,
	.irq_err_enable		= uart_litex_irq_err,
	.irq_err_disable	= uart_litex_irq_err,
	.irq_is_pending		= uart_litex_irq_is_pending,
	.irq_update		= uart_litex_irq_update,
	.irq_callback_set	= uart_litex_irq_callback_set
#endif
};

static int uart_litex_init(const struct device *dev)
{
	const struct uart_litex_device_config *config = dev->config;

	litex_write8(UART_EV_TX | UART_EV_RX, config->ev_pending_addr);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct uart_litex_data *data = dev->data;

	k_timer_init(&data->timer, &uart_litex_tx_soft_isr, NULL);
	k_timer_user_data_set(&data->timer, (void *)dev);

	config->config_func(dev);
#endif

	return 0;
}

#define LITEX_UART_IRQ_INIT(n)                                                                     \
	static void uart_irq_config##n(const struct device *dev)                                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_litex_irq_handler,     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define LITEX_UART_INIT(n)                                                                         \
	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (LITEX_UART_IRQ_INIT(n)))                         \
                                                                                                   \
	static struct uart_litex_data uart_litex_data_##n;                                         \
                                                                                                   \
	static const struct uart_litex_device_config uart_litex_dev_cfg_##n = {                    \
		.rxtx_addr = DT_INST_REG_ADDR_BY_NAME(n, rxtx),                                    \
		.txfull_addr = DT_INST_REG_ADDR_BY_NAME(n, txfull),                                \
		.rxempty_addr = DT_INST_REG_ADDR_BY_NAME(n, rxempty),                              \
		.ev_status_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_status),                          \
		.ev_pending_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_pending),                        \
		.ev_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_enable),                          \
		.txempty_addr = DT_INST_REG_ADDR_BY_NAME(n, txempty),                              \
		.rxfull_addr = DT_INST_REG_ADDR_BY_NAME(n, rxfull),                                \
		.baud_rate = DT_INST_PROP(n, current_speed),                                       \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (.config_func = uart_irq_config##n,))};   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_litex_init, NULL, &uart_litex_data_##n,                      \
			      &uart_litex_dev_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,  \
			      (void *)&uart_litex_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LITEX_UART_INIT)
