/*
 * Copyright (c) 2018 Diego Sueiro <diego.sueiro@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_uart

/**
 * @brief Driver for UART on NXP IMX family processor.
 *
 * For full serial function, use the USART controller.
 *
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <sys/__assert.h>
#include <soc.h>
#include <init.h>
#include <drivers/uart.h>
#include <uart_imx.h>

#define DEV_CFG(dev) \
	((const struct imx_uart_config *const)(dev)->config)
#define UART_STRUCT(dev) \
	((UART_Type *)(DEV_CFG(dev))->base)

struct imx_uart_config {
	UART_Type *base;
	uint32_t baud_rate;
	uint8_t modem_mode;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct imx_uart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_imx_init(const struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);
	const struct imx_uart_config *config = dev->config;
	unsigned int old_level;

	/* disable interrupts */
	old_level = irq_lock();

	/* Setup UART init structure */
	uart_init_config_t initConfig = {
		.baudRate	= config->baud_rate,
		.wordLength = uartWordLength8Bits,
		.stopBitNum = uartStopBitNumOne,
		.parity		= uartParityDisable,
		.direction	= uartDirectionTxRx
	};

	/* Get current module clock frequency */
	initConfig.clockRate  = get_uart_clock_freq(uart);

	UART_Init(uart, &initConfig);

	/* Set UART build-in hardware FIFO Watermark. */
	UART_SetTxFifoWatermark(uart, 2);
	UART_SetRxFifoWatermark(uart, 1);

	/* restore interrupt state */
	irq_unlock(old_level);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	/* Set UART modem mode */
	UART_SetModemMode(uart, config->modem_mode);

	/* Finally, enable the UART module */
	UART_Enable(uart);

	return 0;
}

static void uart_imx_poll_out(const struct device *dev, unsigned char c)
{
	UART_Type *uart = UART_STRUCT(dev);

	while (!UART_GetStatusFlag(uart, uartStatusTxReady)) {
	}
	UART_Putchar(uart, c);
}

static int uart_imx_poll_in(const struct device *dev, unsigned char *c)
{
	UART_Type *uart = UART_STRUCT(dev);

	while (!UART_GetStatusFlag(uart, uartStatusRxDataReady)) {
	}
	*c = UART_Getchar(uart);

	if (UART_GetStatusFlag(uart, uartStatusRxOverrun)) {
		UART_ClearStatusFlag(uart, uartStatusRxOverrun);
	}

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_imx_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int size)
{
	UART_Type *uart = UART_STRUCT(dev);
	unsigned int num_tx = 0U;

	while (((size - num_tx) > 0) &&
		   UART_GetStatusFlag(uart, uartStatusTxReady)) {
		/* Send a character */
		UART_Putchar(uart, tx_data[num_tx]);
		num_tx++;
	}

	return (int)num_tx;
}

static int uart_imx_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	UART_Type *uart = UART_STRUCT(dev);
	unsigned int num_rx = 0U;

	while (((size - num_rx) > 0) &&
		   UART_GetStatusFlag(uart, uartStatusRxReady)) {
		/* Receive a character */
		rx_data[num_rx++] = UART_Getchar(uart);
	}

	if (UART_GetStatusFlag(uart, uartStatusRxOverrun)) {
		UART_ClearStatusFlag(uart, uartStatusRxOverrun);
	}

	return num_rx;
}

static void uart_imx_irq_tx_enable(const struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntTxReady, true);
}

static void uart_imx_irq_tx_disable(const struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntTxReady, false);
}

static int uart_imx_irq_tx_ready(const struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	return UART_GetStatusFlag(uart, uartStatusTxReady);
}

static void uart_imx_irq_rx_enable(const struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntRxReady, true);
}

static void uart_imx_irq_rx_disable(const struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntRxReady, false);
}

static int uart_imx_irq_rx_ready(const struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	return UART_GetStatusFlag(uart, uartStatusRxReady);
}

static void uart_imx_irq_err_enable(const struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntParityError, true);
	UART_SetIntCmd(uart, uartIntFrameError, true);
}

static void uart_imx_irq_err_disable(const struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntParityError, false);
	UART_SetIntCmd(uart, uartIntFrameError, false);
}

static int uart_imx_irq_is_pending(const struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	return UART_GetStatusFlag(uart, uartStatusRxReady) ||
		UART_GetStatusFlag(uart, uartStatusTxReady);
}

static int uart_imx_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_imx_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct imx_uart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * Note: imx UART Tx interrupts when ready to send; Rx interrupts when char
 * received.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
void uart_imx_isr(const struct device *dev)
{
	struct imx_uart_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_imx_driver_api = {
	.poll_in  = uart_imx_poll_in,
	.poll_out = uart_imx_poll_out,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill		  = uart_imx_fifo_fill,
	.fifo_read		  = uart_imx_fifo_read,
	.irq_tx_enable	  = uart_imx_irq_tx_enable,
	.irq_tx_disable   = uart_imx_irq_tx_disable,
	.irq_tx_ready	  = uart_imx_irq_tx_ready,
	.irq_rx_enable	  = uart_imx_irq_rx_enable,
	.irq_rx_disable   = uart_imx_irq_rx_disable,
	.irq_rx_ready	  = uart_imx_irq_rx_ready,
	.irq_err_enable   = uart_imx_irq_err_enable,
	.irq_err_disable  = uart_imx_irq_err_disable,
	.irq_is_pending   = uart_imx_irq_is_pending,
	.irq_update		  = uart_imx_irq_update,
	.irq_callback_set = uart_imx_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */

};

#define UART_IMX_DECLARE_CFG(n, IRQ_FUNC_INIT)				\
	static const struct imx_uart_config imx_uart_##n##_config = {	\
		.base = (UART_Type *) DT_INST_REG_ADDR(n),		\
		.baud_rate = DT_INST_PROP(n, current_speed),		\
		.modem_mode = DT_INST_PROP(n, modem_mode),		\
		IRQ_FUNC_INIT						\
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_IMX_CONFIG_FUNC(n)						\
	static void irq_config_func_##n(const struct device *dev)		\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
				DT_INST_IRQ(n, priority),		\
				uart_imx_isr,				\
				DEVICE_GET(uart_##n), 0);		\
		irq_enable(DT_INST_IRQN(n));				\
	}
#define UART_IMX_IRQ_CFG_FUNC_INIT(n)					\
	.irq_config_func = irq_config_func_##n
#define UART_IMX_INIT_CFG(n)						\
	UART_IMX_DECLARE_CFG(n, UART_IMX_IRQ_CFG_FUNC_INIT(n))
#else
#define UART_IMX_CONFIG_FUNC(n)
#define UART_IMX_IRQ_CFG_FUNC_INIT
#define UART_IMX_INIT_CFG(n)						\
	UART_IMX_DECLARE_CFG(n, UART_IMX_IRQ_CFG_FUNC_INIT)
#endif

#define UART_IMX_INIT(n)						\
	static struct imx_uart_data imx_uart_##n##_data;		\
									\
	static const struct imx_uart_config imx_uart_##n##_config;	\
									\
	DEVICE_AND_API_INIT(uart_##n, DT_INST_LABEL(n), &uart_imx_init,	\
			&imx_uart_##n##_data, &imx_uart_##n##_config,	\
			PRE_KERNEL_1,					\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			&uart_imx_driver_api);				\
									\
	UART_IMX_CONFIG_FUNC(n)						\
									\
	UART_IMX_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(UART_IMX_INIT)
