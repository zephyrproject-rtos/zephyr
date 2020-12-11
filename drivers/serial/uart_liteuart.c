/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_uart0

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <irq.h>
#include <device.h>
#include <drivers/uart.h>
#include <zephyr/types.h>

#define UART_EV_TX	(1 << 0)
#define UART_EV_RX	(1 << 1)
#define UART_BASE_ADDR	DT_INST_REG_ADDR(0)
#define UART_RXTX	((UART_BASE_ADDR) + 0x00)
#define UART_TXFULL	((UART_BASE_ADDR) + 0x04)
#define UART_RXEMPTY	((UART_BASE_ADDR) + 0x08)
#define UART_EV_STATUS	((UART_BASE_ADDR) + 0x0c)
#define UART_EV_PENDING	((UART_BASE_ADDR) + 0x10)
#define UART_EV_ENABLE	((UART_BASE_ADDR) + 0x14)
#define UART_IRQ	DT_INST_IRQN(0)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
typedef void (*irq_cfg_func_t)(void);
#endif

struct uart_liteuart_device_config {
	uint32_t port;
	uint32_t sys_clk_freq;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	irq_cfg_func_t cfg_func;
#endif
};

struct uart_liteuart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
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
static void uart_liteuart_poll_out(const struct device *dev, unsigned char c)
{
	/* wait for space */
	while (sys_read8(UART_TXFULL)) {
	}

	sys_write8(c, UART_RXTX);
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_liteuart_poll_in(const struct device *dev, unsigned char *c)
{
	if (!sys_read8(UART_RXEMPTY)) {
		*c = sys_read8(UART_RXTX);

		/* refresh UART_RXEMPTY by writing UART_EV_RX
		 * to UART_EV_PENDING
		 */
		sys_write8(UART_EV_RX, UART_EV_PENDING);
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
 *
 * @return N/A
 */
static void uart_liteuart_irq_tx_enable(const struct device *dev)
{
	uint8_t enable = sys_read8(UART_EV_ENABLE);

	sys_write8(enable | UART_EV_TX, UART_EV_ENABLE);
}

/**
 * @brief Disable TX interrupt in event register
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_liteuart_irq_tx_disable(const struct device *dev)
{
	uint8_t enable = sys_read8(UART_EV_ENABLE);

	sys_write8(enable & ~(UART_EV_TX), UART_EV_ENABLE);
}

/**
 * @brief Enable RX interrupt in event register
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_liteuart_irq_rx_enable(const struct device *dev)
{
	uint8_t enable = sys_read8(UART_EV_ENABLE);

	sys_write8(enable | UART_EV_RX, UART_EV_ENABLE);
}

/**
 * @brief Disable RX interrupt in event register
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_liteuart_irq_rx_disable(const struct device *dev)
{
	uint8_t enable = sys_read8(UART_EV_ENABLE);

	sys_write8(enable & ~(UART_EV_RX), UART_EV_ENABLE);
}

/**
 * @brief Check if Tx IRQ has been raised and UART is ready to accept new data
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ has been raised, 0 otherwise
 */
static int uart_liteuart_irq_tx_ready(const struct device *dev)
{
	uint8_t val = sys_read8(UART_TXFULL);

	return !val;
}

/**
 * @brief Check if Rx IRQ has been raised and there's data to be read from UART
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ has been raised, 0 otherwise
 */
static int uart_liteuart_irq_rx_ready(const struct device *dev)
{
	uint8_t pending;

	pending = sys_read8(UART_EV_PENDING);

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
static int uart_liteuart_fifo_fill(const struct device *dev,
				   const uint8_t *tx_data, int size)
{
	int i;

	for (i = 0; i < size && !sys_read8(UART_TXFULL); i++) {
		sys_write8(tx_data[i], UART_RXTX);
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
static int uart_liteuart_fifo_read(const struct device *dev,
				   uint8_t *rx_data, const int size)
{
	int i;

	for (i = 0; i < size && !sys_read8(UART_RXEMPTY); i++) {
		rx_data[i] = sys_read8(UART_RXTX);

		/* refresh UART_RXEMPTY by writing UART_EV_RX
		 * to UART_EV_PENDING
		 */
		sys_write8(UART_EV_RX, UART_EV_PENDING);
	}

	return i;
}

static void uart_liteuart_irq_err(const struct device *dev)
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
static int uart_liteuart_irq_is_pending(const struct device *dev)
{
	uint8_t pending;

	pending = sys_read8(UART_EV_PENDING);

	if (pending & (UART_EV_TX | UART_EV_RX)) {
		return 1;
	} else {
		return 0;
	}
}

static int uart_liteuart_irq_update(const struct device *dev)
{
	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 *
 * @return N/A
 */
static void uart_liteuart_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb,
					   void *cb_data)
{
	struct uart_liteuart_data *data;

	data = (struct uart_liteuart_data *)dev->data;
	data->callback = cb;
	data->cb_data = cb_data;
}

static void liteuart_uart_irq_handler(const struct device *dev)
{
	struct uart_liteuart_data *data = DEV_DATA(dev);
	int key = irq_lock();

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}

	/* clear events */
	sys_write8(UART_EV_TX | UART_EV_RX, UART_EV_PENDING);

	irq_unlock(key);
}
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_liteuart_driver_api = {
	.poll_in		= uart_liteuart_poll_in,
	.poll_out		= uart_liteuart_poll_out,
	.err_check		= NULL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill		= uart_liteuart_fifo_fill,
	.fifo_read		= uart_liteuart_fifo_read,
	.irq_tx_enable		= uart_liteuart_irq_tx_enable,
	.irq_tx_disable		= uart_liteuart_irq_tx_disable,
	.irq_tx_ready		= uart_liteuart_irq_tx_ready,
	.irq_rx_enable		= uart_liteuart_irq_rx_enable,
	.irq_rx_disable		= uart_liteuart_irq_rx_disable,
	.irq_rx_ready		= uart_liteuart_irq_rx_ready,
	.irq_err_enable		= uart_liteuart_irq_err,
	.irq_err_disable	= uart_liteuart_irq_err,
	.irq_is_pending		= uart_liteuart_irq_is_pending,
	.irq_update		= uart_liteuart_irq_update,
	.irq_callback_set	= uart_liteuart_irq_callback_set
#endif
};

static struct uart_liteuart_data uart_liteuart_data_0;
static int uart_liteuart_init(const struct device *dev);

static const struct uart_liteuart_device_config uart_liteuart_dev_cfg_0 = {
	.port		= UART_BASE_ADDR,
	.baud_rate	= DT_INST_PROP(0, current_speed)
};

DEVICE_DT_INST_DEFINE(0,
		uart_liteuart_init,
		device_pm_control_nop,
		&uart_liteuart_data_0, &uart_liteuart_dev_cfg_0,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		(void *)&uart_liteuart_driver_api);

static int uart_liteuart_init(const struct device *dev)
{
	sys_write8(UART_EV_TX | UART_EV_RX, UART_EV_PENDING);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	IRQ_CONNECT(UART_IRQ, DT_INST_IRQ(0, priority),
			liteuart_uart_irq_handler, DEVICE_DT_INST_GET(0),
			0);
	irq_enable(UART_IRQ);
#endif

	return 0;
}
