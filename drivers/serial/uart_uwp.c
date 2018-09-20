/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT unisoc_uwp_uart

#include <kernel.h>
#include <arch/cpu.h>
#include <drivers/uart.h>
#include <irq_nextlevel.h>

#include "uwp_hal.h"

#define DIV_ROUND(n, d) (((n) + ((d)/2)) / (d))

/* convenience defines */
#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config_info)
#define DEV_DATA(dev) \
	((struct uart_uwp_dev_data_t * const)(dev)->driver_data)
#define UART_STRUCT(dev) \
	((volatile struct uwp_uart *)(DEV_CFG(dev))->base)

#define UART(idx) DT_NODELABEL(uart##idx)

static struct device DEVICE_NAME_GET(uart_uwp_0);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_uwp_isr(void *arg);
static void uwp_config_0_irq(struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/*private data structure*/
struct uart_uwp_dev_data_t {
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /**< Callback function pointer */
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static struct uart_uwp_dev_data_t uart_uwp_dev_data_0 = {
	.baud_rate = DT_INST_PROP(0, current_speed),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cb = NULL,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static const struct uart_device_config uart_uwp_dev_cfg_0 = {
	.base = (void *)DT_INST_REG_ADDR(0),
	.sys_clk_freq = DT_INST_PROP(0, clock_frequency),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uwp_config_0_irq,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART1_UWP
static struct device DEVICE_NAME_GET(uart_uwp_1);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uwp_config_1_irq(struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static struct uart_uwp_dev_data_t uart_uwp_dev_data_1 = {
	.baud_rate = DT_INST_PROP(1, current_speed),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cb = NULL,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static const struct uart_device_config uart_uwp_dev_cfg_1 = {
	.base = (void *)DT_INST_REG_ADDR(1),
	.sys_clk_freq = DT_INST_PROP(1, clock_frequency),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uwp_config_1_irq,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};
#endif

#ifdef CONFIG_AON_UART_UWP
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uwp_config_2_irq(struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
static struct device DEVICE_NAME_GET(uart_uwp_2);
static struct uart_uwp_dev_data_t uart_uwp_dev_data_2 = {
	.baud_rate = DT_INST_PROP(2, current_speed),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cb = NULL,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static const struct uart_device_config uart_uwp_dev_cfg_2 = {
	.base = (void *)DT_INST_REG_ADDR(2),
	.sys_clk_freq = DT_INST_PROP(2, clock_frequency),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uwp_config_2_irq,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};
#endif /* CONFIG_AON_UART_UWP */

static int uart_uwp_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	if (uwp_uart_rx_ready(uart)) {
		*c = uwp_uart_read(uart);
		return 0;
	}

	return -1;
}

static void uart_uwp_poll_out(struct device *dev, unsigned char c)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	if (uwp_uart_tx_ready(uart)) {
		uwp_uart_write(uart, c);
		while (!uwp_uart_trans_over(uart))
			;
	};
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_uwp_fifo_fill(struct device *dev, const uint8_t *tx_data,
				 int size)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);
	unsigned int num_tx = 0;

	while ((size - num_tx) > 0) {
		/* Send a character */
		if (uwp_uart_tx_ready(uart)) {
			uwp_uart_write(uart, tx_data[num_tx]);
			num_tx++;
		} else {
			break;
		}
	}

	return (int)num_tx;
}

static int uart_uwp_fifo_read(struct device *dev, uint8_t *rx_data,
				 const int size)
{
	unsigned int num_rx = 0;
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	while (((size - num_rx) > 0) &&
			uwp_uart_rx_ready(uart)) {

		/* Receive a character */
		rx_data[num_rx++] = uwp_uart_read(uart);
	}

	return num_rx;
}

static void uart_uwp_irq_tx_enable(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	uwp_uart_int_enable(uart, BIT(UART_TXF_EMPTY));
}

static void uart_uwp_irq_tx_disable(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	uwp_uart_int_disable(uart, BIT(UART_TXF_EMPTY));
}

static int uart_uwp_irq_tx_ready(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);
	uint32_t status;

	status = uwp_uart_status(uart);

	return (status & BIT(UART_TXF_EMPTY));
}

static void uart_uwp_irq_rx_enable(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	uwp_uart_int_enable(uart, BIT(UART_RXF_FULL));
}

static void uart_uwp_irq_rx_disable(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	uwp_uart_int_disable(uart, BIT(UART_RXD));
}

static int uart_uwp_irq_tx_complete(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);
	uint32_t status;

	status = uwp_uart_status(uart);

	return (status & BIT(UART_TXF_EMPTY));
}

static int uart_uwp_irq_rx_ready(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);

	return uwp_uart_rx_ready(uart);
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
	uint32_t status;

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
	dev_data->user_data = user_data;
}

static void uart_uwp_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_uwp_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->cb) {
		dev_data->cb(dev_data->user_data);
	}
}

static void uwp_config_0_irq(struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    uart_uwp_isr,
		    DEVICE_GET(uart_uwp_0), 0);
	irq_enable(DT_INST_IRQN(0));
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
	dev_cfg->irq_config_func(dev);
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

DEVICE_AND_API_INIT(uart_uwp_0, DT_INST_LABEL(0),
		    uart_uwp_init, &uart_uwp_dev_data_0,
		    &uart_uwp_dev_cfg_0,
		    PRE_KERNEL_1, 10,
		    (void *)&uart_uwp_driver_api);

#ifdef CONFIG_UART1_UWP
static void uwp_config_1_irq(struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(1),
		    DT_INST_IRQ(1, priority),
		    uart_uwp_isr,
		    DEVICE_GET(uart_uwp_1), 0);
	irq_enable(DT_INST_IRQN(1));
}

static int uart1_uwp_init(struct device *dev)
{
	volatile struct uwp_uart *uart = UART_STRUCT(dev);
	const struct uart_device_config * const dev_cfg = DEV_CFG(dev);
	struct uart_uwp_dev_data_t * const dev_data = DEV_DATA(dev);

	uwp_sys_enable(BIT(APB_EB_UART1));
	uwp_sys_reset(BIT(APB_EB_UART1));

	uwp_uart_set_cdk(uart, DIV_ROUND(dev_cfg->sys_clk_freq,
			dev_data->baud_rate));
	uwp_uart_set_stop_bit_num(uart, 1);
	uwp_uart_set_byte_len(uart, 3);

	uwp_uart_init(uart);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

DEVICE_AND_API_INIT(uart_uwp_1, DT_INST_LABEL(1),
		    uart1_uwp_init, &uart_uwp_dev_data_1,
		    &uart_uwp_dev_cfg_1,
		    PRE_KERNEL_1, 10,
		    (void *)&uart_uwp_driver_api);
#endif

#ifdef CONFIG_AON_UART_UWP
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uwp_config_2_irq(struct device *dev)
{
	struct device *aon_int_dev;

	aon_int_dev = device_get_binding(DT_LABEL(DT_NODELABEL(aon_intc)));
	if (aon_int_dev == NULL) {
		printk("Can not find device: %s.\n",
				DT_LABEL(DT_NODELABEL(aon_intc)));
		return;
	}

	IRQ_CONNECT(DT_INST_IRQN(2),
		    DT_INST_IRQ(2, priority),
		    uart_uwp_isr,
		    DEVICE_GET(uart_uwp_2), 0);
	irq_enable_next_level(aon_int_dev, AON_INT_UART);
	uart_uwp_irq_rx_enable(dev);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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
	dev_cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

DEVICE_AND_API_INIT(uart_uwp_2, DT_INST_LABEL(2),
		    aon_uart_uwp_init, &uart_uwp_dev_data_2,
		    &uart_uwp_dev_cfg_2,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&uart_uwp_driver_api);
#endif /* CONFIG_AON_UART_UWP */
