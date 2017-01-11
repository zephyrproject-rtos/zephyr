/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for UART port on STM32 family processor.
 *
 */

#ifndef _STM32_UART_H_
#define _STM32_UART_H_

/* device config */
struct uart_stm32_config {
	struct uart_device_config uconf;
	/* clock subsystem driving this peripheral */
#if defined(CONFIG_SOC_SERIES_STM32F1X) || \
    defined(CONFIG_SOC_SERIES_STM32F3X) || \
    defined(CONFIG_SOC_SERIES_STM32L4X)
	clock_control_subsys_t clock_subsys;
#elif defined(CONFIG_SOC_SERIES_STM32F4X)
	struct stm32f4x_pclken pclken;
#endif
};

/* driver data */
struct uart_stm32_data {
	/* Uart peripheral handler */
	UART_HandleTypeDef huart;
	/* clock device */
	struct device *clock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_t user_cb;
#endif
};

#endif	/* _STM32_UART_H_ */
