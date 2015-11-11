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
 * Before individual UART port can be used, k20_uart_port_init() has to be
 * called to setup the port.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <stdint.h>

#include <board.h>
#include <uart.h>
#include <drivers/k20_uart.h>
#include <drivers/k20_sim.h>
#include <toolchain.h>
#include <sections.h>

/* convenience defines */

#define DEV_CFG(dev) \
	((struct uart_device_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_k20_dev_data *)(dev)->driver_data)
#define UART_STRUCT(dev) \
	((volatile struct K20_UART *)(DEV_CFG(dev))->base)

static struct uart_driver_api k20_uart_driver_api;

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param init_info Initial configuration for UART
 *
 * @return N/A
 */
void k20_uart_port_init(struct device *dev,
			const struct uart_init_info * const init_info)
{
	struct uart_k20_dev_data *dev_data = DEV_DATA(dev);

	int old_level; /* old interrupt lock level */
	volatile struct K20_SIM *sim = /* sys integ. ctl */
		(volatile struct K20_SIM *)PERIPH_ADDR_BASE_SIM;
	union C1 c1;				   /* UART C1 register value */
	union C2 c2;				   /* UART C2 register value */

	DEV_CFG(dev)->irq_pri = init_info->irq_pri;

	volatile struct K20_UART *uart = UART_STRUCT(dev);

	/* disable interrupts */
	old_level = irq_lock();

	/* enable clock to Uart - must be done prior to device access */
	_k20_sim_uart_clk_enable(sim, dev_data->seq_port_num);

	_k20_uart_baud_rate_set(uart, init_info->sys_clk_freq,
				init_info->baud_rate);

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

	dev->driver_api = &k20_uart_driver_api;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int k20_uart_poll_in(struct device *dev, unsigned char *c)
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
static unsigned char k20_uart_poll_out(struct device *dev,
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
static int k20_uart_fifo_fill(struct device *dev, const uint8_t *tx_data,
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
static int k20_uart_fifo_read(struct device *dev, uint8_t *rx_data,
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
static void k20_uart_irq_tx_enable(struct device *dev)
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
static void k20_uart_irq_tx_disable(struct device *dev)
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
static int k20_uart_irq_tx_ready(struct device *dev)
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
static void k20_uart_irq_rx_enable(struct device *dev)
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
static void k20_uart_irq_rx_disable(struct device *dev)
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
static int k20_uart_irq_rx_ready(struct device *dev)
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
static void k20_uart_irq_err_enable(struct device *dev)
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
static void k20_uart_irq_err_disable(struct device *dev)
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
static int k20_uart_irq_is_pending(struct device *dev)
{

	return k20_uart_irq_tx_ready(dev) || k20_uart_irq_rx_ready(dev);
}

/**
 * @brief Update IRQ status
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return always 1
 */
static int k20_uart_irq_update(struct device *dev)
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
 * @return N/A
 */
static unsigned int k20_uart_irq_get(struct device *dev)
{
	return (unsigned int)DEV_CFG(dev)->irq;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


static struct uart_driver_api k20_uart_driver_api = {
	.poll_in = k20_uart_poll_in,
	.poll_out = k20_uart_poll_out,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	.fifo_fill = k20_uart_fifo_fill,
	.fifo_read = k20_uart_fifo_read,
	.irq_tx_enable = k20_uart_irq_tx_enable,
	.irq_tx_disable = k20_uart_irq_tx_disable,
	.irq_tx_ready = k20_uart_irq_tx_ready,
	.irq_rx_enable = k20_uart_irq_rx_enable,
	.irq_rx_disable = k20_uart_irq_rx_disable,
	.irq_rx_ready = k20_uart_irq_rx_ready,
	.irq_err_enable = k20_uart_irq_err_enable,
	.irq_err_disable = k20_uart_irq_err_disable,
	.irq_is_pending = k20_uart_irq_is_pending,
	.irq_update = k20_uart_irq_update,
	.irq_get = k20_uart_irq_get,

#endif
};
