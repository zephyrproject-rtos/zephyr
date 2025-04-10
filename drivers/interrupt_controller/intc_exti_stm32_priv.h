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

#define STM32_EXTI_TOTAL_LINES_NUM	DT_PROP(EXTI_NODE, num_lines)

#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
	defined(CONFIG_SOC_SERIES_STM32MP13X)

#define EXTI_LINE_NOT_SUPP_ASSERT(line)	\
{										\
	LOG_ERR("Unsupported line number %u", line);	\
	__ASSERT_NO_MSG(0);								\
}

#define LL_EXTI_IsActiveRisingFlag_32_63(line)		0
#define LL_EXTI_IsActiveFallingFlag_32_63(line)		0

#define LL_EXTI_ClearRisingFlag_32_63(line)		EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_ClearFallingFlag_32_63(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_GenerateSWI_32_63(line)			EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_EnableRisingTrig_32_63(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_EnableFallingTrig_32_63(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_DisableRisingTrig_32_63(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_DisableFallingTrig_32_63(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_EnableEvent_32_63(line)			EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_DisableEvent_32_63(line)		EXTI_LINE_NOT_SUPP_ASSERT(line)

#if defined(CONFIG_SOC_SERIES_STM32MP13X)

#define LL_EXTI_IsActiveRisingFlag_64_95(line)		0
#define LL_EXTI_IsActiveFallingFlag_64_95(line)		0

#define LL_EXTI_ClearRisingFlag_64_95(line)		EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_ClearFallingFlag_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_GenerateSWI_64_95(line)			EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_EnableRisingTrig_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_DisableRisingTrig_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_EnableFallingTrig_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_DisableFallingTrig_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_EnableEvent_64_95(line)			EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_DisableEvent_64_95(line)		EXTI_LINE_NOT_SUPP_ASSERT(line)

#endif /* !CONFIG_SOC_SERIES_STM32MP13X */

#endif /* CONFIG_SOC_SERIES_STM32MP1X || CONFIG_SOC_SERIES_STM32MP13X */

/**
 * @brief The section below covers one or two cpu SoCs. At the moment there is
 * only stm32h7x containing two cores (Cortex-M7 and Cortex-M4).
 * IMPORTANT: Better appoach would be to destinguish between one core and many.
 *				To implement this certain KConfig options for single
 *				and multi-core SoCs should be created for stm32 platform.
 */
#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
#define CPU_NR _C2
#else /* CONFIG_SOC_SERIES_STM32H7X && CONFIG_CPU_CORTEX_M4 */
#define CPU_NR
#endif /* CONFIG_SOC_SERIES_STM32H7X && CONFIG_CPU_CORTEX_M4 */

/* Define general macros line-range independent */
#define EXTI_ENABLE_IT(line_range, line)			\
	CONCAT(LL, CPU_NR, _EXTI_EnableIT_##line_range)(line)
#define EXTI_DISABLE_IT(line_range, line)			\
	CONCAT(LL, CPU_NR, _EXTI_DisableIT_##line_range)(line)
#define EXTI_ENABLE_EVENT(line_range, line)			\
	CONCAT(LL, CPU_NR, _EXTI_EnableEvent_##line_range)(line)
#define EXTI_DISABLE_EVENT(line_range, line)		\
	CONCAT(LL, CPU_NR, _EXTI_DisableEvent_##line_range)(line)

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)

/* Defeine general macros line-range independent */
#define EXTI_IS_ACTIVE_RISING_FLAG(line_range, line)	\
	CONCAT(LL, CPU_NR, _EXTI_IsActiveRisingFlag_##line_range)(line)
#define EXTI_IS_ACTIVE_FALLING_FLAG(line_range, line)	\
	CONCAT(LL, CPU_NR, _EXTI_IsActiveFallingFlag_##line_range)(line)

#define EXTI_CLEAR_RISING_FLAG(line_range, line)	\
	CONCAT(LL, CPU_NR, _EXTI_ClearRisingFlag_##line_range)(line)
#define EXTI_CLEAR_FALLING_FLAG(line_range, line)	\
	CONCAT(LL, CPU_NR, _EXTI_ClearFallingFlag_##line_range)(line)

#define EXTI_CLEAR_FLAG(line_range, line)			\
	{							\
		EXTI_CLEAR_RISING_FLAG(line_range, (line));	\
		EXTI_CLEAR_FALLING_FLAG(line_range, (line));	\
	}
#define EXTI_IS_ACTIVE_FLAG(line_range, line)			\
	(EXTI_IS_ACTIVE_RISING_FLAG(line_range, (line)) ||	\
	 EXTI_IS_ACTIVE_FALLING_FLAG(line_range, (line)))

#else /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti) */

#define EXTI_CLEAR_FLAG(line_range, line)			\
	CONCAT(LL, CPU_NR, _EXTI_ClearFlag_##line_range)(line)
#define EXTI_IS_ACTIVE_FLAG(line_range, line)		\
	CONCAT(LL, CPU_NR, _EXTI_IsActiveFlag_##line_range)(line)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti) */

#define EXTI_ENABLE_RISING_TRIG(line_range, line)	\
	CONCAT(LL, _EXTI_EnableRisingTrig_##line_range)(line)
#define EXTI_ENABLE_FALLING_TRIG(line_range, line)	\
	CONCAT(LL, _EXTI_EnableFallingTrig_##line_range)(line)
#define EXTI_DISABLE_FALLING_TRIG(line_range, line)	\
	CONCAT(LL, _EXTI_DisableFallingTrig_##line_range)(line)
#define EXTI_DISABLE_RISING_TRIG(line_range, line)	\
	CONCAT(LL, _EXTI_DisableRisingTrig_##line_range)(line)
#define EXTI_GENERATE_SWI(line_range, line)			\
	CONCAT(LL, _EXTI_GenerateSWI_##line_range)(line)

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
