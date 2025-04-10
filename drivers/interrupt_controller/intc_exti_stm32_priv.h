/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_PRIV_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_PRIV_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_exti.h>
#include <stm32_ll_system.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXTI_NODE DT_INST(0, st_stm32_exti)

/* TODO: replace num_lines by toal_num_lines as soon as decision is made in PR */
#define STM32_EXTI_TOTAL_LINES_NUM	DT_PROP(EXTI_NODE, num_lines)

#define STM32_EXTI_TOTAL_LINES_NUM_64	(STM32_EXTI_TOTAL_LINES_NUM > 32) &&  \
										(STM32_EXTI_TOTAL_LINES_NUM <= 64)
#define STM32_EXTI_TOTAL_LINES_NUM_96	(STM32_EXTI_TOTAL_LINES_NUM > 64) &&  \
										(STM32_EXTI_TOTAL_LINES_NUM <= 96)

#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
#define CPU_NR _C2
#else /* CONFIG_SOC_SERIES_STM32H7X && CONFIG_CPU_CORTEX_M4 */
#define CPU_NR
#endif /* CONFIG_SOC_SERIES_STM32H7X && CONFIG_CPU_CORTEX_M4 */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)

/* Defeine general macros line-range independent */
#define EXTI_IS_ACTIVE_RISING_FLAG(line_num, line)	CONCAT(LL, CPU_NR, _EXTI_IsActiveRisingFlag_##line_num)(line)
#define EXTI_IS_ACTIVE_FALLING_FLAG(line_num, line)	CONCAT(LL, CPU_NR, _EXTI_IsActiveFallingFlag_##line_num)(line)
#define LL_EXTI_IsActiveFlag(line_num, line) \
	(EXTI_IS_ACTIVE_RISING_FLAG(line_num, line) || EXTI_IS_ACTIVE_FALLING_FLAG(line_num, line))

#define EXTI_CLEAR_RISING_FLAG(line_num, line)	CONCAT(LL, CPU_NR, _EXTI_ClearRisingFlag_##line_num)(line)
#define EXTI_CLEAR_FALLING_FLAG(line_num, line)	CONCAT(LL, CPU_NR, _EXTI_ClearFallingFlag_##line_num)(line)
#define LL_EXTI_ClearFlag(line_num, line)		\
	{										\
		EXTI_CLEAR_RISING_FLAG(line_num, line);	\
		EXTI_CLEAR_FALLING_FLAG(line_num, line);	\
	}

#define LL_EXTI_IsActiveFlag_0_31(line)	LL_EXTI_IsActiveFlag(0_31, line)
#define LL_EXTI_ClearFlag_0_31(line)	LL_EXTI_ClearFlag(0_31, line)

#if STM32_EXTI_TOTAL_LINES_NUM_64

#define LL_EXTI_IsActiveFlag_32_63(line)	LL_EXTI_IsActiveFlag(32_63, line)
#define LL_EXTI_ClearFlag_32_63(line)		LL_EXTI_ClearFlag(32_63, line)

#endif /* STM32_EXTI_TOTAL_LINES_NUM < 64 */

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti) */

/* Defeine general macros line-range independent */
#define EXTI_ENABLE_IT(line_num, line)				CONCAT(LL, CPU_NR, _EXTI_EnableIT_##line_num)(line)
#define EXTI_DISABLE_IT(line_num, line)				CONCAT(LL, CPU_NR, _EXTI_DisableIT_##line_num)(line)
#define EXTI_ENABLE_EVENT(line_num, line)			CONCAT(LL, CPU_NR, _EXTI_EnableEvent_##line_num)(line)
#define EXTI_DISABLE_EVENT(line_num, line)			CONCAT(LL, CPU_NR, _EXTI_DisableEvent_##line_num)(line)
#define EXTI_CLEAR_FLAG(line_num, line)				CONCAT(LL, CPU_NR, _EXTI_ClearFlag_##line_num)(line)
#define EXTI_IS_ACTIVE_FLAG(line_num, line)			CONCAT(LL, CPU_NR, _EXTI_IsActiveFlag_##line_num)(line)
#define EXTI_ENABLE_RISING_TRIG(line_num, line)		CONCAT(LL, _EXTI_EnableRisingTrig_##line_num)(line)
#define EXTI_ENABLE_FALLING_TRIG(line_num, line)	CONCAT(LL, _EXTI_EnableFallingTrig_##line_num)(line)
#define EXTI_DISABLE_FALLING_TRIG(line_num, line)	CONCAT(LL, _EXTI_DisableFallingTrig_##line_num)(line)
#define EXTI_DISABLE_RISING_TRIG(line_num, line)	CONCAT(LL, _EXTI_DisableRisingTrig_##line_num)(line)
#define EXTI_GENERATE_SWI(line_num, line)			CONCAT(LL, _EXTI_GenerateSWI_##line_num)(line)

/**
 * @returns LL_EXTI_LINE_n define corresponding to EXTI line number
 */
static inline uint32_t exti_linenum_to_ll_exti_line(uint32_t line_num)
{
	return BIT(line_num % 32);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_PRIV_H_ */
