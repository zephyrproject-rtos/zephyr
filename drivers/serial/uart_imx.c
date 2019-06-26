/*
 * Copyright (c) 2018 Diego Sueiro <diego.sueiro@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


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
	((const struct imx_uart_config *const)(dev)->config->config_info)
#define UART_STRUCT(dev) \
	((UART_Type *)(DEV_CFG(dev))->base)

struct imx_uart_config {
	UART_Type *base;
	u32_t baud_rate;
	u8_t modem_mode;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(struct device *dev);
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
static int uart_imx_init(struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);
	const struct imx_uart_config *config = dev->config->config_info;
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

static void uart_imx_poll_out(struct device *dev, unsigned char c)
{
	UART_Type *uart = UART_STRUCT(dev);

	while (!UART_GetStatusFlag(uart, uartStatusTxReady)) {
	}
	UART_Putchar(uart, c);
}

static int uart_imx_poll_in(struct device *dev, unsigned char *c)
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

static int uart_imx_fifo_fill(struct device *dev, const u8_t *tx_data,
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

static int uart_imx_fifo_read(struct device *dev, u8_t *rx_data,
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

static void uart_imx_irq_tx_enable(struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntTxReady, true);
}

static void uart_imx_irq_tx_disable(struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntTxReady, false);
}

static int uart_imx_irq_tx_ready(struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	return UART_GetStatusFlag(uart, uartStatusTxReady);
}

static void uart_imx_irq_rx_enable(struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntRxReady, true);
}

static void uart_imx_irq_rx_disable(struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntRxReady, false);
}

static int uart_imx_irq_rx_ready(struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	return UART_GetStatusFlag(uart, uartStatusRxReady);
}

static void uart_imx_irq_err_enable(struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntParityError, true);
	UART_SetIntCmd(uart, uartIntFrameError, true);
}

static void uart_imx_irq_err_disable(struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	UART_SetIntCmd(uart, uartIntParityError, false);
	UART_SetIntCmd(uart, uartIntFrameError, false);
}

static int uart_imx_irq_is_pending(struct device *dev)
{
	UART_Type *uart = UART_STRUCT(dev);

	return UART_GetStatusFlag(uart, uartStatusRxReady) ||
		UART_GetStatusFlag(uart, uartStatusTxReady);
}

static int uart_imx_irq_update(struct device *dev)
{
	return 1;
}

static void uart_imx_irq_callback_set(struct device *dev,
		uart_irq_callback_user_data_t cb,
		void *cb_data)
{
	struct imx_uart_data *data = dev->driver_data;

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
void uart_imx_isr(void *arg)
{
	struct device *dev = arg;
	struct imx_uart_data *data = dev->driver_data;

	if (data->callback) {
		data->callback(data->cb_data);
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


#ifdef CONFIG_UART_IMX_UART_1

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_1(struct device *port);
#endif

static const struct imx_uart_config imx_uart_1_config = {
	.base = (UART_Type *) DT_UART_IMX_UART_1_BASE_ADDRESS,
	.baud_rate = DT_UART_IMX_UART_1_BAUD_RATE,
	.modem_mode = DT_UART_IMX_UART_1_MODEM_MODE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_1,
#endif
};

static struct imx_uart_data imx_uart_1_data;

DEVICE_AND_API_INIT(uart_1, DT_UART_IMX_UART_1_NAME, &uart_imx_init,
		&imx_uart_1_data, &imx_uart_1_config,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&uart_imx_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_1(struct device *dev)
{
	IRQ_CONNECT(DT_UART_IMX_UART_1_IRQ_NUM,
			DT_UART_IMX_UART_1_IRQ_PRI,
			uart_imx_isr, DEVICE_GET(uart_1),
			0);
	irq_enable(DT_UART_IMX_UART_1_IRQ_NUM);
}
#endif

#endif /* CONFIG_UART_IMX_UART_1 */


#ifdef CONFIG_UART_IMX_UART_2

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_2(struct device *port);
#endif

static const struct imx_uart_config imx_uart_2_config = {
	.base = (UART_Type *) DT_UART_IMX_UART_2_BASE_ADDRESS,
	.baud_rate = DT_UART_IMX_UART_2_BAUD_RATE,
	.modem_mode = DT_UART_IMX_UART_2_MODEM_MODE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_2,
#endif
};

static struct imx_uart_data imx_uart_2_data;

DEVICE_AND_API_INIT(uart_2, DT_UART_IMX_UART_2_NAME, &uart_imx_init,
		&imx_uart_2_data, &imx_uart_2_config,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&uart_imx_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_2(struct device *dev)
{
	IRQ_CONNECT(DT_UART_IMX_UART_2_IRQ_NUM,
			DT_UART_IMX_UART_2_IRQ_PRI,
			uart_imx_isr, DEVICE_GET(uart_2),
			0);
	irq_enable(DT_UART_IMX_UART_2_IRQ_NUM);
}
#endif

#endif /* CONFIG_UART_IMX_UART_2 */

#ifdef CONFIG_UART_IMX_UART_3

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_3(struct device *port);
#endif

static const struct imx_uart_config imx_uart_3_config = {
	.base = (UART_Type *) DT_UART_IMX_UART_3_BASE_ADDRESS,
	.baud_rate = DT_UART_IMX_UART_3_BAUD_RATE,
	.modem_mode = DT_UART_IMX_UART_3_MODEM_MODE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_3,
#endif
};

static struct imx_uart_data imx_uart_3_data;

DEVICE_AND_API_INIT(uart_3, DT_UART_IMX_UART_3_NAME, &uart_imx_init,
		&imx_uart_3_data, &imx_uart_3_config,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&uart_imx_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_3(struct device *dev)
{
	IRQ_CONNECT(DT_UART_IMX_UART_3_IRQ_NUM,
			DT_UART_IMX_UART_3_IRQ_PRI,
			uart_imx_isr, DEVICE_GET(uart_3),
			0);
	irq_enable(DT_UART_IMX_UART_3_IRQ_NUM);
}
#endif

#endif /* CONFIG_UART_IMX_UART_3 */

#ifdef CONFIG_UART_IMX_UART_4

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_4(struct device *port);
#endif

static const struct imx_uart_config imx_uart_4_config = {
	.base = (UART_Type *) DT_UART_IMX_UART_4_BASE_ADDRESS,
	.baud_rate = DT_UART_IMX_UART_4_BAUD_RATE,
	.modem_mode = DT_UART_IMX_UART_4_MODEM_MODE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_4,
#endif
};

static struct imx_uart_data imx_uart_4_data;

DEVICE_AND_API_INIT(uart_4, DT_UART_IMX_UART_4_NAME, &uart_imx_init,
		&imx_uart_4_data, &imx_uart_4_config,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&uart_imx_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_4(struct device *dev)
{
	IRQ_CONNECT(DT_UART_IMX_UART_4_IRQ_NUM,
			DT_UART_IMX_UART_4_IRQ_PRI,
			uart_imx_isr, DEVICE_GET(uart_4),
			0);
	irq_enable(DT_UART_IMX_UART_4_IRQ_NUM);
}
#endif

#endif /* CONFIG_UART_IMX_UART_4 */

#ifdef CONFIG_UART_IMX_UART_5

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_5(struct device *port);
#endif

static const struct imx_uart_config imx_uart_5_config = {
	.base = (UART_Type *) DT_UART_IMX_UART_5_BASE_ADDRESS,
	.baud_rate = DT_UART_IMX_UART_5_BAUD_RATE,
	.modem_mode = DT_UART_IMX_UART_5_MODEM_MODE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_5,
#endif
};

static struct imx_uart_data imx_uart_5_data;

DEVICE_AND_API_INIT(uart_5, DT_UART_IMX_UART_5_NAME, &uart_imx_init,
		&imx_uart_5_data, &imx_uart_5_config,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&uart_imx_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_5(struct device *dev)
{
	IRQ_CONNECT(DT_UART_IMX_UART_5_IRQ_NUM,
			DT_UART_IMX_UART_5_IRQ_PRI,
			uart_imx_isr, DEVICE_GET(uart_5),
			0);
	irq_enable(DT_UART_IMX_UART_5_IRQ_NUM);
}
#endif

#endif /* CONFIG_UART_IMX_UART_5 */

#ifdef CONFIG_UART_IMX_UART_6

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_6(struct device *port);
#endif

static const struct imx_uart_config imx_uart_6_config = {
	.base = (UART_Type *) DT_UART_IMX_UART_6_BASE_ADDRESS,
	.baud_rate = DT_UART_IMX_UART_6_BAUD_RATE,
	.modem_mode = DT_UART_IMX_UART_6_MODEM_MODE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_6,
#endif
};

static struct imx_uart_data imx_uart_6_data;

DEVICE_AND_API_INIT(uart_6, DT_UART_IMX_UART_6_NAME, &uart_imx_init,
		&imx_uart_6_data, &imx_uart_6_config,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&uart_imx_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_6(struct device *dev)
{
	IRQ_CONNECT(DT_UART_IMX_UART_6_IRQ_NUM,
			DT_UART_IMX_UART_6_IRQ_PRI,
			uart_imx_isr, DEVICE_GET(uart_6),
			0);
	irq_enable(DT_UART_IMX_UART_6_IRQ_NUM);
}
#endif

#endif /* CONFIG_UART_IMX_UART_6 */

#ifdef CONFIG_UART_IMX_UART_7

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_7(struct device *port);
#endif

static const struct imx_uart_config imx_uart_7_config = {
	.base = (UART_Type *) DT_UART_IMX_UART_7_BASE_ADDRESS,
	.baud_rate = DT_UART_IMX_UART_7_BAUD_RATE,
	.modem_mode = DT_UART_IMX_UART_7_MODEM_MODE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_7,
#endif
};

static struct imx_uart_data imx_uart_7_data;

DEVICE_AND_API_INIT(uart_7, DT_UART_IMX_UART_7_NAME, &uart_imx_init,
		&imx_uart_7_data, &imx_uart_7_config,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&uart_imx_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_7(struct device *dev)
{
	IRQ_CONNECT(DT_UART_IMX_UART_7_IRQ_NUM,
			DT_UART_IMX_UART_7_IRQ_PRI,
			uart_imx_isr, DEVICE_GET(uart_7),
			0);
	irq_enable(DT_UART_IMX_UART_7_IRQ_NUM);
}
#endif

#endif /* CONFIG_UART_IMX_UART_7 */
