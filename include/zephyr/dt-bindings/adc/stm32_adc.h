/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2022 STMicroelectronics
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_STM32_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_STM32_H_

/**
 * @name custom ADC clock scheme
 * This value is to check <st,prescaler> (1st element)
 * and decide if the ADC clock is either SYNC_PCLK or ASYNC specific
 * Only Some stm32 have both possible ADC clocks : refer to the RefMan.
 * @{
 */
#define SYNC  1
#define ASYNC 2
/** @} */

/**
 * @cond INTERNAL_HIDDEN
 *
 * Internal documentation. Skip in public documentation
 */

/**
 * @brief Macro from the LL private for stm32 families
 * to identify if the CLOCK is common to several ADC instances or not.
 *
 * @param clock_pre Clock prescaler value to check.
 */
#define IS_LL_ADC_COMMON_CLOCK(clock_pre)		\
	(((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV1)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV2)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV4)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV6)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV8)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV10)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV12)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV16)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV32)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV64)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV128)	\
	|| ((clock_pre) == LL_ADC_CLOCK_ASYNC_DIV256)	\
	)

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_STM32_H_ */
