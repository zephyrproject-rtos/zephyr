/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for UART port on STM32 family processor.
 *
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
#if defined(CONFIG_SOC_SERIES_STM32F1X) || defined(CONFIG_SOC_SERIES_STM32F4X) \
	|| defined(CONFIG_SOC_SERIES_STM32F2X)
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
#if !defined(CONFIG_SOC_SERIES_STM32F0X) || defined(USART_LIN_SUPPORT)
	/* Enable Line break detection */
	if (IS_UART_LIN_INSTANCE(UartInstance)) {
		LL_USART_EnableIT_LBD(UartInstance);
	}
#endif
	/* Enable parity error interruption */
	LL_USART_EnableIT_PE(UartInstance);
}

static void uart_stm32_irq_err_disable(struct device *dev)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	/* Disable FE, ORE interruptions */
	LL_USART_DisableIT_ERROR(UartInstance);
#if !defined(CONFIG_SOC_SERIES_STM32F0X) || defined(USART_LIN_SUPPORT)
	/* Disable Line break detection */
	if (IS_UART_LIN_INSTANCE(UartInstance)) {
		LL_USART_DisableIT_LBD(UartInstance);
	}
#endif
	/* Disable parity error interruption */
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
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_stm32_data *data = DEV_DATA(dev);

	data->user_cb = cb;
	data->user_data = cb_data;
}

static void uart_stm32_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_stm32_data *data = DEV_DATA(dev);

	if (data->user_cb) {
		data->user_cb(data->user_data);
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

static void uart_stm32_usart_set_baud_rate(struct device *dev,
					   u32_t clock_rate, u32_t baud_rate)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_USART_SetBaudRate(UartInstance,
			     clock_rate,
#ifdef USART_PRESC_PRESCALER
			     LL_USART_PRESCALER_DIV1,
#endif
#ifdef USART_CR1_OVER8
			     LL_USART_OVERSAMPLING_16,
#endif
			     baud_rate);
}

#ifdef CONFIG_UART_STM32_LPUART_1
static void uart_stm32_lpuart_set_baud_rate(struct device *dev,
					    u32_t clock_rate, u32_t baud_rate)
{
	USART_TypeDef *UartInstance = UART_STRUCT(dev);

	LL_LPUART_SetBaudRate(UartInstance,
			      clock_rate,
#ifdef USART_PRESC_PRESCALER
			      LL_USART_PRESCALER_DIV1,
#endif
			      baud_rate);
}
#endif

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

	u32_t baud_rate = config->baud_rate;
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

#ifdef CONFIG_UART_STM32_LPUART_1
	if (IS_LPUART_INSTANCE(UartInstance)) {
		uart_stm32_lpuart_set_baud_rate(dev, clock_rate, baud_rate);
	} else {
		uart_stm32_usart_set_baud_rate(dev, clock_rate, baud_rate);
	}
#else
	uart_stm32_usart_set_baud_rate(dev, clock_rate, baud_rate);
#endif

	LL_USART_Enable(UartInstance);

#ifdef USART_ISR_TEACK
	/* Wait until TEACK flag is set */
	while (!(LL_USART_IsActiveFlag_TEACK(UartInstance)))
		;
#endif /* !USART_ISR_TEACK */

#ifdef USART_ISR_REACK
	/* Wait until REACK flag is set */
	while (!(LL_USART_IsActiveFlag_REACK(UartInstance)))
		;
#endif /* !USART_ISR_REACK */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->uconf.irq_config_func(dev);
#endif
	return 0;
}

/* Define clocks */
#define STM32_CLOCK_UART(clock_bus, clock_enr)	\
	.pclken = { .bus = clock_bus,		\
		    .enr = clock_enr }

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define STM32_UART_IRQ_HANDLER_DECL(name)				\
	static void uart_stm32_irq_config_func_##name(struct device *dev)
#define STM32_UART_IRQ_HANDLER_FUNC(name)				\
	.irq_config_func = uart_stm32_irq_config_func_##name,
#define STM32_UART_IRQ_HANDLER(name)					\
static void uart_stm32_irq_config_func_##name(struct device *dev)	\
{									\
	IRQ_CONNECT(name##_IRQ,					\
		CONFIG_UART_STM32_##name##_IRQ_PRI,			\
		uart_stm32_isr, DEVICE_GET(uart_stm32_##name),	\
		0);							\
	irq_enable(name##_IRQ);					\
}
#else
#define STM32_UART_IRQ_HANDLER_DECL(name)
#define STM32_UART_IRQ_HANDLER_FUNC(name)
#define STM32_UART_IRQ_HANDLER(name)
#endif

#define STM32_UART_INIT(name, clock_bus, clock_enr)			\
STM32_UART_IRQ_HANDLER_DECL(name);					\
									\
static const struct uart_stm32_config uart_stm32_cfg_##name = {		\
	.uconf = {							\
		.base = (u8_t *)CONFIG_UART_STM32_##name##_BASE_ADDRESS,\
		STM32_UART_IRQ_HANDLER_FUNC(name)			\
	},								\
	STM32_CLOCK_UART(clock_bus, clock_enr),				\
	.baud_rate = CONFIG_UART_STM32_##name##_BAUD_RATE		\
};									\
									\
static struct uart_stm32_data uart_stm32_data_##name = {		\
};									\
									\
DEVICE_AND_API_INIT(uart_stm32_##name, CONFIG_UART_STM32_##name##_NAME,	\
		    &uart_stm32_init,					\
		    &uart_stm32_data_##name, &uart_stm32_cfg_##name,	\
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &uart_stm32_driver_api);				\
									\
STM32_UART_IRQ_HANDLER(name)

/*
 * STM32F0 and STM32L0 series differ from other STM32 series by some
 * peripheral names (UART vs USART). Besides, STM32F0 doesn't have APB2 bus,
 * so APB1 GRP2 should be accessed instead.
 */
#if defined(CONFIG_SOC_SERIES_STM32F0X)

#ifdef CONFIG_UART_STM32_PORT_1
STM32_UART_INIT(USART_1, STM32_CLOCK_BUS_APB1_2, LL_APB1_GRP2_PERIPH_USART1)
#endif	/* CONFIG_UART_STM32_PORT_1 */

#ifdef CONFIG_UART_STM32_PORT_2
STM32_UART_INIT(USART_2, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_USART2)
#endif	/* CONFIG_UART_STM32_PORT_2 */

#ifdef CONFIG_UART_STM32_PORT_3
STM32_UART_INIT(USART_3, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_USART3)
#endif	/* CONFIG_UART_STM32_PORT_3 */

#ifdef CONFIG_UART_STM32_PORT_4
STM32_UART_INIT(USART_4, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_USART4)
#endif /* CONFIG_UART_STM32_PORT_4 */

#ifdef CONFIG_UART_STM32_PORT_5
STM32_UART_INIT(USART_5, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_USART5)
#endif /* CONFIG_UART_STM32_PORT_5 */

#ifdef CONFIG_UART_STM32_PORT_6
STM32_UART_INIT(USART_6, STM32_CLOCK_BUS_APB1_2, LL_APB1_GRP2_PERIPH_USART6)
#endif /* CONFIG_UART_STM32_PORT_6 */

#ifdef CONFIG_UART_STM32_PORT_7
STM32_UART_INIT(USART_7, STM32_CLOCK_BUS_APB1_2, LL_APB1_GRP2_PERIPH_USART7)
#endif /* CONFIG_UART_STM32_PORT_7 */

#ifdef CONFIG_UART_STM32_PORT_8
STM32_UART_INIT(USART_8, STM32_CLOCK_BUS_APB1_2, LL_APB1_GRP2_PERIPH_USART8)
#endif /* CONFIG_UART_STM32_PORT_8 */

#elif defined(CONFIG_SOC_SERIES_STM32L0X)

#ifdef CONFIG_UART_STM32_PORT_1
STM32_UART_INIT(USART_1, STM32_CLOCK_BUS_APB2, LL_APB2_GRP1_PERIPH_USART1)
#endif	/* CONFIG_UART_STM32_PORT_1 */

#ifdef CONFIG_UART_STM32_PORT_2
STM32_UART_INIT(USART_2, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_USART2)
#endif	/* CONFIG_UART_STM32_PORT_2 */

#ifdef CONFIG_UART_STM32_PORT_4
STM32_UART_INIT(USART_4, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_USART4)
#endif /* CONFIG_UART_STM32_PORT_4 */

#ifdef CONFIG_UART_STM32_PORT_5
STM32_UART_INIT(USART_5, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_USART5)
#endif /* CONFIG_UART_STM32_PORT_5 */

#ifdef CONFIG_UART_STM32_LPUART_1
STM32_UART_INIT(LPUART_1, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_LPUART1)
#endif /* CONFIG_UART_STM32_LPUART_1 */

#else

#ifdef CONFIG_UART_STM32_PORT_1
STM32_UART_INIT(USART_1, STM32_CLOCK_BUS_APB2, LL_APB2_GRP1_PERIPH_USART1)
#endif	/* CONFIG_UART_STM32_PORT_1 */

#ifdef CONFIG_UART_STM32_PORT_2
STM32_UART_INIT(USART_2, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_USART2)
#endif	/* CONFIG_UART_STM32_PORT_2 */

#ifdef CONFIG_UART_STM32_PORT_3
STM32_UART_INIT(USART_3, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_USART3)
#endif	/* CONFIG_UART_STM32_PORT_3 */

#ifdef CONFIG_UART_STM32_PORT_4
STM32_UART_INIT(UART_4, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_UART4)
#endif /* CONFIG_UART_STM32_PORT_4 */

#ifdef CONFIG_UART_STM32_PORT_5
STM32_UART_INIT(UART_5, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_UART5)
#endif /* CONFIG_UART_STM32_PORT_5 */

#ifdef CONFIG_UART_STM32_PORT_6
STM32_UART_INIT(USART_6, STM32_CLOCK_BUS_APB2, LL_APB2_GRP1_PERIPH_USART6)
#endif /* CONFIG_UART_STM32_PORT_6 */

#ifdef CONFIG_UART_STM32_PORT_7
STM32_UART_INIT(UART_7, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_UART7)
#endif /* CONFIG_UART_STM32_PORT_7 */

#ifdef CONFIG_UART_STM32_PORT_8
STM32_UART_INIT(UART_8, STM32_CLOCK_BUS_APB1, LL_APB1_GRP1_PERIPH_UART8)
#endif /* CONFIG_UART_STM32_PORT_8 */

#ifdef CONFIG_UART_STM32_PORT_9
STM32_UART_INIT(UART_9, STM32_CLOCK_BUS_APB2, LL_APB2_GRP1_PERIPH_UART9)
#endif /* CONFIG_UART_STM32_PORT_9 */

#ifdef CONFIG_UART_STM32_PORT_10
STM32_UART_INIT(UART_10, STM32_CLOCK_BUS_APB2, LL_APB2_GRP1_PERIPH_UART10)
#endif /* CONFIG_UART_STM32_PORT_10 */

#ifdef CONFIG_SOC_SERIES_STM32L4X
#ifdef CONFIG_UART_STM32_LPUART_1
STM32_UART_INIT(LPUART_1, STM32_CLOCK_BUS_APB1_2, LL_APB1_GRP2_PERIPH_LPUART1)
#endif /* CONFIG_UART_STM32_LPUART_1 */
#endif /* CONFIG_SOC_SERIES_STM32L4X */

#endif
