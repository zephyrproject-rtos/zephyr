/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for UART port on STM32F10x family processor.
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 27: Universal synchronous asynchronous receiver
 *             transmitter (USART)
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <board.h>
#include <init.h>
#include <uart.h>
#include <clock_control.h>

#include <linker/sections.h>
#include <clock_control/stm32_clock_control.h>
#include "uart_stm32.h"

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct uart_stm32_config * const)(dev)->config->config_info)
#define DEV_DATA(dev)							\
	((struct uart_stm32_data * const)(dev)->driver_data)
#define UART_STRUCT(dev)					\
	((USART_TypeDef *)(DEV_CFG(dev))->uconf.base)

#define TIMEOUT 1000

static int uart_stm32_poll_in(struct device *dev, unsigned char *c)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	if (!LL_USART_IsActiveFlag_RXNE(UartInstance)) {
		return -1;
	}

	*c = (unsigned char)LL_USART_ReceiveData8(UartInstance);

	return 0;
}

static unsigned char uart_stm32_poll_out(struct device *dev,
					unsigned char c)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Wait for TXE flag to be raised */
	while (!LL_USART_IsActiveFlag_TXE(UartInstance))
		;

	LL_USART_ClearFlag_TC(UartInstance);

	LL_USART_TransmitData8(UartInstance, (u8_t)c);

	return c;
}

static inline void __uart_stm32_get_clock(struct device *dev)
{
	struct uart_stm32_data *data = DEV_DATA(dev);
	struct device *clk =
		device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(clk);

	data->clock = clk;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_stm32_fifo_fill(struct device *dev, const u8_t *tx_data,
				  int size)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	u8_t num_tx = 0;

	while ((size - num_tx > 0) &&
	       LL_USART_IsActiveFlag_TXE(UartInstance)) {
		/* TXE flag will be cleared with byte write to DR register */

		/* Send a character (8bit , parity none) */
		LL_USART_TransmitData8(UartInstance, tx_data[num_tx++]);
	}

	return num_tx;
}

static int uart_stm32_fifo_read(struct device *dev, u8_t *rx_data,
				  const int size)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);
	u8_t num_rx = 0;

	while ((size - num_rx > 0) &&
	       LL_USART_IsActiveFlag_RXNE(UartInstance)) {
#if defined(CONFIG_SOC_SERIES_STM32F1X) || defined(CONFIG_SOC_SERIES_STM32F4X)
		/* Clear the interrupt */
		LL_USART_ClearFlag_RXNE(UartInstance);
#endif

		/* Receive a character (8bit , parity none) */
		rx_data[num_rx++] = LL_USART_ReceiveData8(UartInstance);
	}
	return num_rx;
}

static void uart_stm32_irq_tx_enable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_EnableIT_TC(UartInstance);
}

static void uart_stm32_irq_tx_disable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_DisableIT_TC(UartInstance);
}

static int uart_stm32_irq_tx_ready(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_TXE(UartInstance);
}

static int uart_stm32_irq_tx_complete(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_TXE(UartInstance);
}

static void uart_stm32_irq_rx_enable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_EnableIT_RXNE(UartInstance);
}

static void uart_stm32_irq_rx_disable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_DisableIT_RXNE(UartInstance);
}

static int uart_stm32_irq_rx_ready(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return LL_USART_IsActiveFlag_RXNE(UartInstance);
}

static void uart_stm32_irq_err_enable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Enable FE, ORE interruptions */
	LL_USART_EnableIT_ERROR(UartInstance);
	/* Enable Line break detection */
#ifndef CONFIG_SOC_SERIES_STM32F0X
	LL_USART_EnableIT_LBD(UartInstance);
#endif
	/* Enable parity error interruption */
	LL_USART_EnableIT_PE(UartInstance);
}

static void uart_stm32_irq_err_disable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Enable FE, ORE interruptions */
	LL_USART_DisableIT_ERROR(UartInstance);
	/* Enable Line break detection */
#ifndef CONFIG_SOC_SERIES_STM32F0X
	LL_USART_DisableIT_LBD(UartInstance);
#endif
	/* Enable parity error interruption */
	LL_USART_DisableIT_PE(UartInstance);
}

static int uart_stm32_irq_is_pending(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	return ((LL_USART_IsActiveFlag_RXNE(UartInstance) &&
		 LL_USART_IsEnabledIT_RXNE(UartInstance)) ||
		(LL_USART_IsActiveFlag_TXE(UartInstance) &&
		 LL_USART_IsEnabledIT_TXE(UartInstance)));
}

static int uart_stm32_irq_update(struct device *dev)
{
	return 1;
}

static void uart_stm32_irq_callback_set(struct device *dev,
					uart_irq_callback_t cb)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	data->user_cb = cb;
}

static void uart_stm32_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_stm32_data *data = DEV_DATA(dev);

	if (data->user_cb) {
		data->user_cb(dev);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_stm32_driver_api = {
	.poll_in = uart_stm32_poll_in,
	.poll_out = uart_stm32_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_stm32_fifo_fill,
	.fifo_read = uart_stm32_fifo_read,
	.irq_tx_enable = uart_stm32_irq_tx_enable,
	.irq_tx_disable = uart_stm32_irq_tx_disable,
	.irq_tx_ready = uart_stm32_irq_tx_ready,
	.irq_tx_complete = uart_stm32_irq_tx_complete,
	.irq_rx_enable = uart_stm32_irq_rx_enable,
	.irq_rx_disable = uart_stm32_irq_rx_disable,
	.irq_rx_ready = uart_stm32_irq_rx_ready,
	.irq_err_enable = uart_stm32_irq_err_enable,
	.irq_err_disable = uart_stm32_irq_err_disable,
	.irq_is_pending = uart_stm32_irq_is_pending,
	.irq_update = uart_stm32_irq_update,
	.irq_callback_set = uart_stm32_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */
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
static int uart_stm32_init(struct device *dev)
{
	const struct uart_stm32_config *config = DEV_CFG(dev);
	struct uart_stm32_data *data = DEV_DATA(dev);
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	u32_t clock_rate;

	__uart_stm32_get_clock(dev);
	/* enable clock */
	clock_control_on(data->clock,
			(clock_control_subsys_t *)&config->pclken);

	LL_USART_Disable(UartInstance);

	/* TX/RX direction */
	LL_USART_SetTransferDirection(UartInstance,
				      LL_USART_DIRECTION_TX_RX);

	/* 8 data bit, 1 start bit, 1 stop bit, no parity */
	LL_USART_ConfigCharacter(UartInstance,
				 LL_USART_DATAWIDTH_8B,
				 LL_USART_PARITY_NONE,
				 LL_USART_STOPBITS_1);

	/* Get clock rate */
	clock_control_get_rate(data->clock,
			       (clock_control_subsys_t *)&config->pclken,
			       &clock_rate);

	LL_USART_SetBaudRate(UartInstance,
			     clock_rate,
#ifdef USART_CR1_OVER8
			     LL_USART_OVERSAMPLING_16,
#endif
			     data->huart.Init.BaudRate);

	LL_USART_Enable(UartInstance);

#if !defined(CONFIG_SOC_SERIES_STM32F4X) && !defined(CONFIG_SOC_SERIES_STM32F1X)
	/* Polling USART initialisation */
	while ((!(LL_USART_IsActiveFlag_TEACK(UartInstance))) ||
	      (!(LL_USART_IsActiveFlag_REACK(UartInstance))))
		;
#endif /* !CONFIG_SOC_SERIES_STM32F4X */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->uconf.irq_config_func(dev);
#endif
	return 0;
}

/* Define clocks */
	#define STM32_CLOCK_UART(type, apb, n)				\
		.pclken = { .bus = STM32_CLOCK_BUS_ ## apb,		\
			    .enr = LL_##apb##_GRP1_PERIPH_##type##n }

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define STM32_UART_IRQ_HANDLER_DECL(n)					\
	static void uart_stm32_irq_config_func_##n(struct device *dev)
#define STM32_UART_IRQ_HANDLER_FUNC(n)					\
	.irq_config_func = uart_stm32_irq_config_func_##n,
#define STM32_UART_IRQ_HANDLER(n)					\
static void uart_stm32_irq_config_func_##n(struct device *dev)		\
{									\
	IRQ_CONNECT(PORT_ ## n ## _IRQ,					\
		CONFIG_UART_STM32_PORT_ ## n ## _IRQ_PRI,		\
		uart_stm32_isr, DEVICE_GET(uart_stm32_ ## n),		\
		0);							\
	irq_enable(PORT_ ## n ## _IRQ);					\
}
#else
#define STM32_UART_IRQ_HANDLER_DECL(n)
#define STM32_UART_IRQ_HANDLER_FUNC(n)
#define STM32_UART_IRQ_HANDLER(n)
#endif

#define UART_DEVICE_INIT_STM32(type, n, apb)				\
STM32_UART_IRQ_HANDLER_DECL(n);						\
									\
static const struct uart_stm32_config uart_stm32_dev_cfg_##n = {	\
	.uconf = {							\
		.base = (u8_t *)CONFIG_UART_STM32_PORT_ ## n ## _BASE_ADDRESS, \
		STM32_UART_IRQ_HANDLER_FUNC(n)				\
	},								\
	STM32_CLOCK_UART(type, apb, n),					\
};									\
									\
static struct uart_stm32_data uart_stm32_dev_data_##n = {		\
	.huart = {							\
		.Init = {						\
			.BaudRate = CONFIG_UART_STM32_PORT_##n##_BAUD_RATE     \
		}							\
	}								\
};									\
									\
DEVICE_AND_API_INIT(uart_stm32_##n, CONFIG_UART_STM32_PORT_##n##_NAME,	\
		    &uart_stm32_init,					\
		    &uart_stm32_dev_data_##n, &uart_stm32_dev_cfg_##n,	\
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &uart_stm32_driver_api);				\
									\
STM32_UART_IRQ_HANDLER(n)

#ifdef CONFIG_UART_STM32_PORT_1
UART_DEVICE_INIT_STM32(USART, 1, APB2)
#endif	/* CONFIG_UART_STM32_PORT_1 */

#ifdef CONFIG_UART_STM32_PORT_2
UART_DEVICE_INIT_STM32(USART, 2, APB1)
#endif	/* CONFIG_UART_STM32_PORT_2 */

#ifdef CONFIG_UART_STM32_PORT_3
UART_DEVICE_INIT_STM32(USART, 3, APB1)
#endif	/* CONFIG_UART_STM32_PORT_3 */

#ifdef CONFIG_UART_STM32_PORT_4
UART_DEVICE_INIT_STM32(UART, 4, APB1)
#endif /* CONFIG_UART_STM32_PORT_4 */

#ifdef CONFIG_UART_STM32_PORT_5
UART_DEVICE_INIT_STM32(UART, 5, APB1)
#endif /* CONFIG_UART_STM32_PORT_5 */

#ifdef CONFIG_UART_STM32_PORT_6
UART_DEVICE_INIT_STM32(USART, 6, APB2)
#endif /* CONFIG_UART_STM32_PORT_6 */

#ifdef CONFIG_UART_STM32_PORT_7
UART_DEVICE_INIT_STM32(UART, 7, APB1)
#endif /* CONFIG_UART_STM32_PORT_7 */

#ifdef CONFIG_UART_STM32_PORT_8
UART_DEVICE_INIT_STM32(UART, 8, APB1)
#endif /* CONFIG_UART_STM32_PORT_8 */

#ifdef CONFIG_UART_STM32_PORT_9
UART_DEVICE_INIT_STM32(UART, 9, APB2)
#endif /* CONFIG_UART_STM32_PORT_9 */

#ifdef CONFIG_UART_STM32_PORT_10
UART_DEVICE_INIT_STM32(UART, 10, APB2)
#endif /* CONFIG_UART_STM32_PORT_10 */
