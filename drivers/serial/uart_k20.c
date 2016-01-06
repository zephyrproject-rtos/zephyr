/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief UART driver for the Freescale K20 Family of microprocessors.
 *
 * Before individual UART port can be used, uart_k20_port_init() has to be
 * called to setup the port.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <stdint.h>

#include <board.h>
#include <init.h>
#include <uart.h>
#include <toolchain.h>
#include <sections.h>

#include "uart_k20_priv.h"

/* convenience defines */

#define DEV_CFG(dev) \
	((struct uart_device_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_k20_dev_data_t * const)(dev)->driver_data)
#define UART_STRUCT(dev) \
	((volatile struct K20_UART *)(DEV_CFG(dev))->base)

/* Device data structure */
struct uart_k20_dev_data_t {
	uint32_t baud_rate;	/* Baud rate */
};

static struct uart_driver_api uart_k20_driver_api;

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return DEV_OK
 */
static int uart_k20_init(struct device *dev)
{
	int old_level; /* old interrupt lock level */
	union C1 c1;				   /* UART C1 register value */
	union C2 c2;				   /* UART C2 register value */

	volatile struct K20_UART *uart = UART_STRUCT(dev);
	struct uart_device_config * const dev_cfg = DEV_CFG(dev);
	struct uart_k20_dev_data_t * const dev_data = DEV_DATA(dev);

	/* disable interrupts */
	old_level = irq_lock();

	_uart_k20_baud_rate_set(uart, dev_cfg->sys_clk_freq,
				dev_data->baud_rate);

	/* 1 start bit, 8 data bits, no parity, 1 stop bit */
	c1.value = 0;

	uart->c1 = c1;

	/* enable Rx and Tx with interrupts disabled */
	c2.value = 0;
	c2.field.rx_enable = 1;
	c2.field.tx_enable = 1;

	uart->c2 = c2;

	/* restore interrupt state */
	irq_unlock(old_level);

	dev->driver_api = &uart_k20_driver_api;

	return DEV_OK;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_k20_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);

	if (uart->s1.field.rx_data_full == 0)
		return (-1);

	/* got a character */
	*c = uart->d;

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * If the hardware flow control is enabled then the handshake signal CTS has to
 * be asserted in order to send a character.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param c Character to send
 *
 * @return sent character
 */
static unsigned char uart_k20_poll_out(struct device *dev,
				       unsigned char c)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);

	/* wait for transmitter to ready to accept a character */
	while (uart->s1.field.tx_data_empty == 0)
		;

	uart->d = c;

	return c;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param tx_data Data to transmit
 * @param len Number of bytes to send
 *
 * @return number of bytes sent
 */
static int uart_k20_fifo_fill(struct device *dev, const uint8_t *tx_data,
			      int len)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);
	uint8_t num_tx = 0;

	while ((len - num_tx > 0) && (uart->s1.field.tx_data_empty == 1)) {
		uart->d = tx_data[num_tx++];
	}

	return num_tx;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param rx_data Pointer to data container
 * @param size Container size in bytes
 *
 * @return number of bytes read
 */
static int uart_k20_fifo_read(struct device *dev, uint8_t *rx_data,
			      const int size)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);
	uint8_t num_rx = 0;

	while ((size - num_rx > 0) && (uart->s1.field.rx_data_full != 0)) {
		rx_data[num_rx++] = uart->d;
	}

	return num_rx;
}

/**
 * @brief Enable TX interrupt
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void uart_k20_irq_tx_enable(struct device *dev)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);

	uart->c2.field.tx_int_dma_tx_en = 1;
}

/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void uart_k20_irq_tx_disable(struct device *dev)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);

	uart->c2.field.tx_int_dma_tx_en = 0;
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_k20_irq_tx_ready(struct device *dev)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);

	return (uart->c2.field.tx_int_dma_tx_en == 0) ?
			0 : uart->s1.field.tx_data_empty;
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void uart_k20_irq_rx_enable(struct device *dev)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);

	uart->c2.field.rx_full_int_dma_tx_en = 1;
}

/**
 * @brief Disable RX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void uart_k20_irq_rx_disable(struct device *dev)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);

	uart->c2.field.rx_full_int_dma_tx_en = 0;
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_k20_irq_rx_ready(struct device *dev)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);

	return (uart->c2.field.rx_full_int_dma_tx_en == 0) ?
			0 : uart->s1.field.rx_data_full;
}

/**
 * @brief Enable error interrupt
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void uart_k20_irq_err_enable(struct device *dev)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);
	union C3 c3 = uart->c3;

	c3.field.parity_err_int_en = 1;
	c3.field.frame_err_int_en = 1;
	c3.field.noise_err_int_en = 1;
	c3.field.overrun_err_int_en = 1;
	uart->c3 = c3;
}

/**
 * @brief Disable error interrupt
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void uart_k20_irq_err_disable(struct device *dev)
{
	volatile struct K20_UART *uart = UART_STRUCT(dev);
	union C3 c3 = uart->c3;

	c3.field.parity_err_int_en = 0;
	c3.field.frame_err_int_en = 0;
	c3.field.noise_err_int_en = 0;
	c3.field.overrun_err_int_en = 0;
	uart->c3 = c3;
}

/**
 * @brief Check if Tx or Rx IRQ is pending
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if a Tx or Rx IRQ is pending, 0 otherwise
 */
static int uart_k20_irq_is_pending(struct device *dev)
{

	return uart_k20_irq_tx_ready(dev) || uart_k20_irq_rx_ready(dev);
}

/**
 * @brief Update IRQ status
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return always 1
 */
static int uart_k20_irq_update(struct device *dev)
{
	return 1;
}

/**
 * @brief Returns UART interrupt number
 *
 * Returns the IRQ number used by the specified UART port
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return IRQ number
 */
static unsigned int uart_k20_irq_get(struct device *dev)
{
	return (unsigned int)DEV_CFG(dev)->irq;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


static struct uart_driver_api uart_k20_driver_api = {
	.poll_in = uart_k20_poll_in,
	.poll_out = uart_k20_poll_out,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	.fifo_fill = uart_k20_fifo_fill,
	.fifo_read = uart_k20_fifo_read,
	.irq_tx_enable = uart_k20_irq_tx_enable,
	.irq_tx_disable = uart_k20_irq_tx_disable,
	.irq_tx_ready = uart_k20_irq_tx_ready,
	.irq_rx_enable = uart_k20_irq_rx_enable,
	.irq_rx_disable = uart_k20_irq_rx_disable,
	.irq_rx_ready = uart_k20_irq_rx_ready,
	.irq_err_enable = uart_k20_irq_err_enable,
	.irq_err_disable = uart_k20_irq_err_disable,
	.irq_is_pending = uart_k20_irq_is_pending,
	.irq_update = uart_k20_irq_update,
	.irq_get = uart_k20_irq_get,

#endif
};


#ifdef CONFIG_UART_K20_PORT_0

static struct uart_device_config uart_k20_dev_cfg_0 = {
	.base = (uint8_t *)CONFIG_UART_K20_PORT_0_BASE_ADDR,
	.irq = CONFIG_UART_K20_PORT_0_IRQ,
	.irq_pri = CONFIG_UART_K20_PORT_0_IRQ_PRI,

	.sys_clk_freq = CONFIG_UART_K20_PORT_0_CLK_FREQ,
};

static struct uart_k20_dev_data_t uart_k20_dev_data_0 = {
	.baud_rate = CONFIG_UART_K20_PORT_0_BAUD_RATE,
};

DECLARE_DEVICE_INIT_CONFIG(uart_k20_0,
			   CONFIG_UART_K20_PORT_0_NAME,
			   &uart_k20_init,
			   &uart_k20_dev_cfg_0);

SYS_DEFINE_DEVICE(uart_k20_0, &uart_k20_dev_data_0, PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_UART_K20_PORT_0 */

#ifdef CONFIG_UART_K20_PORT_1

static struct uart_device_config uart_k20_dev_cfg_1 = {
	.base = (uint8_t *)CONFIG_UART_K20_PORT_1_BASE_ADDR,
	.irq = CONFIG_UART_K20_PORT_1_IRQ,
	.irq_pri = CONFIG_UART_K20_PORT_1_IRQ_PRI,

	.sys_clk_freq = CONFIG_UART_K20_PORT_1_CLK_FREQ,
};

static struct uart_k20_dev_data_t uart_k20_dev_data_1 = {
	.baud_rate = CONFIG_UART_K20_PORT_1_BAUD_RATE,
};

DECLARE_DEVICE_INIT_CONFIG(uart_k20_1,
			   CONFIG_UART_K20_PORT_1_NAME,
			   &uart_k20_init,
			   &uart_k20_dev_cfg_1);

SYS_DEFINE_DEVICE(uart_k20_1, &uart_k20_dev_data_1, PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_UART_K20_PORT_1 */

#ifdef CONFIG_UART_K20_PORT_2

static struct uart_device_config uart_k20_dev_cfg_2 = {
	.base = (uint8_t *)CONFIG_UART_K20_PORT_2_BASE_ADDR,
	.irq = CONFIG_UART_K20_PORT_2_IRQ,
	.irq_pri = CONFIG_UART_K20_PORT_2_IRQ_PRI,

	.sys_clk_freq = CONFIG_UART_K20_PORT_2_CLK_FREQ,
};

static struct uart_k20_dev_data_t uart_k20_dev_data_2 = {
	.baud_rate = CONFIG_UART_K20_PORT_2_BAUD_RATE,
};

DECLARE_DEVICE_INIT_CONFIG(uart_k20_2,
			   CONFIG_UART_K20_PORT_2_NAME,
			   &uart_k20_init,
			   &uart_k20_dev_cfg_2);

SYS_DEFINE_DEVICE(uart_k20_2, &uart_k20_dev_data_2, PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_UART_K20_PORT_2 */

#ifdef CONFIG_UART_K20_PORT_3

static struct uart_device_config uart_k20_dev_cfg_3 = {
	.base = (uint8_t *)CONFIG_UART_K20_PORT_3_BASE_ADDR,
	.irq = CONFIG_UART_K20_PORT_3_IRQ,
	.irq_pri = CONFIG_UART_K20_PORT_3_IRQ_PRI,

	.sys_clk_freq = CONFIG_UART_K20_PORT_3_CLK_FREQ,
};

static struct uart_k20_dev_data_t uart_k20_dev_data_3 = {
	.baud_rate = CONFIG_UART_K20_PORT_3_BAUD_RATE,
};

DECLARE_DEVICE_INIT_CONFIG(uart_k20_3,
			   CONFIG_UART_K20_PORT_3_NAME,
			   &uart_k20_init,
			   &uart_k20_dev_cfg_3);

SYS_DEFINE_DEVICE(uart_k20_3, &uart_k20_dev_data_3, PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_UART_K20_PORT_3 */

#ifdef CONFIG_UART_K20_PORT_4

static struct uart_device_config uart_k20_dev_cfg_4 = {
	.base = (uint8_t *)CONFIG_UART_K20_PORT_4_BASE_ADDR,
	.irq = CONFIG_UART_K20_PORT_4_IRQ,
	.irq_pri = CONFIG_UART_K20_PORT_4_IRQ_PRI,

	.sys_clk_freq = CONFIG_UART_K20_PORT_4_CLK_FREQ,
};

static struct uart_k20_dev_data_t uart_k20_dev_data_4 = {
	.baud_rate = CONFIG_UART_K20_PORT_4_BAUD_RATE,
};

DECLARE_DEVICE_INIT_CONFIG(uart_k20_4,
			   CONFIG_UART_K20_PORT_4_NAME,
			   &uart_k20_init,
			   &uart_k20_dev_cfg_4);

SYS_DEFINE_DEVICE(uart_k20_4, &uart_k20_dev_data_4, PRIMARY,
		  CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_UART_K20_PORT_4 */
