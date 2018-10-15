/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <uart.h>

#include "uwp_hal.h"
#include "uart_uwp.h"
#include "../interrupt_controller/intc_uwp.h"

/* convenience defines */
#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_uwp_dev_data_t * const)(dev)->driver_data)
#define UART_STRUCT(dev) \
	((volatile struct uwp_uart *)(DEV_CFG(dev))->base)

static struct device DEVICE_NAME_GET(uart_uwp_1);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_uwp_isr(void *arg);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/*private data structure*/
struct uart_uwp_dev_data_t {
	u32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /**< Callback function pointer */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static struct uart_uwp_dev_data_t uart_uwp_dev_data_1 = {
	.baud_rate = CONFIG_UART_UWP_SPEED,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cb = NULL,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static const struct uart_device_config uart_uwp_dev_cfg_1 = {
	.base = (void *)CONFIG_UART_UWP_BASE,
	.sys_clk_freq = CONFIG_UART_UWP_CLOCK,
};

#ifdef CONFIG_AON_UART_UWP
static struct device DEVICE_NAME_GET(uart_uwp_2);
static struct uart_uwp_dev_data_t uart_uwp_dev_data_2 = {
	.baud_rate = CONFIG_UART_UWP_SPEED,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cb = NULL,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static const struct uart_device_config uart_uwp_dev_cfg_2 = {
	.base = (void *)BASE_AON_UART,
	.sys_clk_freq = CONFIG_UART_UWP_CLOCK,
};
#endif

static int uart_uwp_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	while (!malin_uart_rx_ready(uart)) {
	};

	*c = uwp_uart_read(uart);

	return 0;
}

static unsigned char uart_uwp_poll_out(struct device *dev, unsigned char c)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	while (!malin_uart_tx_ready(uart)) {
	};

	uwp_uart_write(uart, c);

	while (!uwp_uart_trans_over(uart)) {
	};

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_uwp_fifo_fill(struct device *dev, const u8_t *tx_data,
				 int size)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);
	unsigned int num_tx = 0;

	while ((size - num_tx) > 0) {
		/* Send a character */
		if (malin_uart_tx_ready(uart)) {
			uwp_uart_write(uart, tx_data[num_tx]);
			num_tx++;
		} else {
			break;
		}
	}

	return (int)num_tx;
}

static int uart_uwp_fifo_read(struct device *dev, u8_t *rx_data,
				 const int size)
{
	unsigned int num_rx = 0;
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	while (((size - num_rx) > 0) &&
			malin_uart_rx_ready(uart)) {

		/* Receive a character */
		rx_data[num_rx++] = uwp_uart_read(uart);
	}

	return num_rx;
}

static void uart_uwp_irq_tx_enable(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	uwp_uart_int_enable(uart, UART_TXF_EMPTY);
}

static void uart_uwp_irq_tx_disable(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	uwp_uart_int_disable(uart, UART_TXF_EMPTY);
}

static int uart_uwp_irq_tx_ready(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);
	u32_t status;

	status = uwp_uart_status(uart);

	return (status & UART_TXF_EMPTY);
}

static void uart_uwp_irq_rx_enable(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	uwp_uart_int_enable(uart, BIT(UART_RXF_FULL));
}

static void uart_uwp_irq_rx_disable(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	uwp_uart_int_disable(uart, UART_RXD);
}

static int uart_uwp_irq_tx_complete(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);
	u32_t status;

	status = uwp_uart_status(uart);

	return (status & UART_TXF_EMPTY);
}

static int uart_uwp_irq_rx_ready(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	return malin_uart_rx_ready(uart);
}

static void uart_uwp_irq_err_enable(struct device *dev)
{
	/* Not yet used in zephyr */
}

static void uart_uwp_irq_err_disable(struct device *dev)
{
	/* Not yet used in zephyr */
}

static int uart_uwp_irq_is_pending(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);
	u32_t status;

	status = uwp_uart_status(uart);

	return (status & (UART_TXF_EMPTY | UART_RXF_FULL));
}

static int uart_uwp_irq_update(struct device *dev)
{
	return 1;
}

static void uart_uwp_irq_callback_set(struct device *dev,
					 uart_irq_callback_user_data_t cb,
					 void *user_data)
{
	struct uart_uwp_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
}

static void uart_uwp_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_uwp_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->cb) {
		dev_data->cb(dev);
	}

	/* Clear interrupts here */
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_uwp_init(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);
	const struct uart_device_config * const dev_cfg = DEV_CFG(dev);
	struct uart_uwp_dev_data_t * const dev_data = DEV_DATA(dev);

	uwp_sys_enable(BIT(APB_EB_UART0));
	uwp_sys_reset(BIT(APB_EB_UART0));

	uwp_uart_set_cdk(uart, DIV_ROUND(dev_cfg->sys_clk_freq,
			dev_data->baud_rate));
	uwp_uart_set_stop_bit_num(uart, 1);
	uwp_uart_set_byte_len(uart, 3);

	uwp_uart_init(uart);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	IRQ_CONNECT(CONFIG_UART_UWP_IRQ,
			CONFIG_UART_UWP_IRQ_PRIO,
			uart_uwp_isr,
			DEVICE_GET(uart_uwp_1), 0);
	irq_enable(CONFIG_UART_UWP_IRQ);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

static const struct uart_driver_api uart_uwp_driver_api = {
	.poll_in = uart_uwp_poll_in,
	.poll_out = uart_uwp_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill	  = uart_uwp_fifo_fill,
	.fifo_read	  = uart_uwp_fifo_read,
	.irq_tx_enable	  = uart_uwp_irq_tx_enable,
	.irq_tx_disable	  = uart_uwp_irq_tx_disable,
	.irq_tx_ready	  = uart_uwp_irq_tx_ready,
	.irq_rx_enable	  = uart_uwp_irq_rx_enable,
	.irq_rx_disable	  = uart_uwp_irq_rx_disable,
	.irq_tx_complete  = uart_uwp_irq_tx_complete,
	.irq_rx_ready	  = uart_uwp_irq_rx_ready,
	.irq_err_enable	  = uart_uwp_irq_err_enable,
	.irq_err_disable  = uart_uwp_irq_err_disable,
	.irq_is_pending	  = uart_uwp_irq_is_pending,
	.irq_update	  = uart_uwp_irq_update,
	.irq_callback_set = uart_uwp_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

DEVICE_AND_API_INIT(uart_uwp_1, CONFIG_UART_UWP_NAME,
		    uart_uwp_init, &uart_uwp_dev_data_1,
		    &uart_uwp_dev_cfg_1,
		    PRE_KERNEL_1, 10,
		    (void *)&uart_uwp_driver_api);

#ifdef CONFIG_AON_UART_UWP
static void aon_uart_uwp_isr(int ch, void *arg)
{
	struct device *dev = arg;
	struct uart_uwp_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->cb) {
		dev_data->cb(dev);
	}
}

static int aon_uart_uwp_init(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);
	const struct uart_device_config * const dev_cfg = DEV_CFG(dev);
	struct uart_uwp_dev_data_t * const dev_data = DEV_DATA(dev);

	uwp_aon_enable(BIT(AON_EB_UART));
	uwp_aon_reset(BIT(AON_RST_UART));

	uwp_uart_set_cdk(uart, DIV_ROUND(dev_cfg->sys_clk_freq,
			dev_data->baud_rate));
	uwp_uart_set_stop_bit_num(uart, 1);
	uwp_uart_set_byte_len(uart, 3);

	uwp_uart_init(uart);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uwp_aon_intc_set_irq_callback(AON_INT_UART, aon_uart_uwp_isr, dev);
	uwp_aon_irq_enable(AON_INT_UART);
	uart_uwp_irq_rx_enable(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

DEVICE_AND_API_INIT(uart_uwp_2, UNISOC_UART_40838000_LABEL,
		    aon_uart_uwp_init, &uart_uwp_dev_data_2,
		    &uart_uwp_dev_cfg_2,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&uart_uwp_driver_api);
#endif
