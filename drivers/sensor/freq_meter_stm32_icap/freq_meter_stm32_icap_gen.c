/*
 * Copyright (c) 2024 Freedom Veiculos Eletricos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>
#include <stm32_ll_tim.h>

#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(freq_meter_stm32_icap_gen, CONFIG_SENSOR_LOG_LEVEL);

static inline int enable_frequency_generator(void)
{
	/*************************/
	/* GPIO AF configuration */
	/*************************/
	/* Enable the peripheral clock of GPIOs */
#if defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
#else
	LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
#endif

	/* GPIO TIM2_CH1 configuration */
	LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_ALTERNATE);
	LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_0, LL_GPIO_PULL_DOWN);
	LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_HIGH);
	LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_0, LL_GPIO_AF_1);

	/******************************/
	/* Peripheral clocks enabling */
	/******************************/
	/* Enable the timer peripheral clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

	/***************************/
	/* Time base configuration */
	/***************************/
	/* Set counter mode */
	/* Reset value is LL_TIM_COUNTERMODE_UP */
	LL_TIM_SetCounterMode(TIM2, LL_TIM_COUNTERMODE_UP);

	/* Enable TIM2_ARR register preload. Writing to or reading from the         */
	/* auto-reload register accesses the preload register. The content of the   */
	/* preload register are transferred into the shadow register at each update */
	/* event (UEV).                                                             */
	LL_TIM_EnableARRPreload(TIM2);

	/* Set the auto-reload value to have a counter frequency of 2 kHz           */
	/* TIM2CLK = SystemCoreClock / (APB prescaler & multiplier)                 */
	/* TIM2 counter frequency = TimOutClock / (ARR + 1)                   */
#if defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_TIM_SetAutoReload(TIM2, __LL_TIM_CALC_ARR(SystemCoreClock / 2, LL_TIM_GetPrescaler(TIM2),
						     CONFIG_FREQ_METER_STM32_ICAP_GEN_FREQ));
#else
	LL_TIM_SetAutoReload(TIM2, __LL_TIM_CALC_ARR(SystemCoreClock, LL_TIM_GetPrescaler(TIM2),
						     CONFIG_FREQ_METER_STM32_ICAP_GEN_FREQ));
#endif

	/*********************************/
	/* Output waveform configuration */
	/*********************************/
	/* Set output mode: PWM mode 1 */
	LL_TIM_OC_SetMode(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);

	/* Set compare value to half of the counter period (50% duty cycle )*/
	LL_TIM_OC_SetCompareCH1(TIM2, (LL_TIM_GetAutoReload(TIM2) / 2));

	/* Enable TIM2_CCR1 register preload. Read/Write operations access the      */
	/* preload register. TIM2_CCR1 preload value is loaded in the active        */
	/* at each update event.                                                    */
	LL_TIM_OC_EnablePreload(TIM2, LL_TIM_CHANNEL_CH1);

	/**********************************/
	/* Start output signal generation */
	/**********************************/
	/* Enable output channel 1 */
	LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1);

	/* Enable counter */
	LL_TIM_EnableCounter(TIM2);

	/* Force update generation */
	LL_TIM_GenerateEvent_UPDATE(TIM2);

	LOG_INF("TIM2 Frequency Generator initialized. Output Frequency at PA0");

	return 0;
}

SYS_INIT(enable_frequency_generator, POST_KERNEL, 99);
