/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
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
 * @brief Driver for UART port on STM32F10x, STM32F40x family processor.
 *
 */

#ifndef _STM32_UART_H_
#define _STM32_UART_H_

/* device config */
struct uart_stm32_config {
	struct uart_device_config uconf;
	/* clock subsystem driving this peripheral */
#ifdef CONFIG_SOC_SERIES_STM32F1X
	clock_control_subsys_t clock_subsys;
#elif CONFIG_SOC_SERIES_STM32F4X
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
