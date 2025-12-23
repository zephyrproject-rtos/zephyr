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

/**
 * NOTE: This implementation currently does not support configurations where a
 * single CPU has access to multiple EXTI instances. Supporting multiple EXTI
 * instances per CPU (such as possible on STM32MP2 series with both M33 and M0+
 * cores) will require changes to this macro and potentially other parts of the
 * driver.
 */
#define EXTI_NODE	DT_INST(0, st_stm32_exti)

/**
 * STM32MP1 has up to 96 EXTI lines (RM0475, Table 112. STM32MP13xx EXTI Events),
 * but some ranges contain only direct lines, so the LL functions that are
 * only valid for configurable lines are not provided for these ranges.
 * However, we are relying on all these functions being present as part of our
 * utility macros. Define dummy stubs for these functions to allow the main driver
 * to work properly on this special series.
 *
 * NOTE: These stubs are only for compatibility and will assert if called for
 * unsupported lines. Always ensure the correct line range is used.
 */

#define STM32_EXTI_TOTAL_LINES_NUM	DT_PROP(EXTI_NODE, num_lines)

/**
 * @brief Define CPU number suffix for STM32 series.
 */
#if (defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)) || \
	(defined(CONFIG_SOC_SERIES_STM32MP2X) && defined(CONFIG_CPU_CORTEX_M33))
#define CPU_NR	_C2
#elif defined(CONFIG_SOC_SERIES_STM32MP2X) && defined(CONFIG_CPU_CORTEX_M0)
#define CPU_NR	_C3
#else
/* NOTE: usually one CPU (e.g. C1) only is omitted and leaved empty */
#define CPU_NR
#endif

/**
 * @brief Define EXTI_LL_INST for STM32MP2X series, since it may have multiple instances
 * For all other than STM32MP2X series the EXTI instance is defined within LL driver
 * itself s.t. EXTI_LL_INST will be empty.
 */
#if defined(CONFIG_SOC_SERIES_STM32MP2X)
#define EXTI_LL_INST	((EXTI_TypeDef *)DT_REG_ADDR(EXTI_NODE)),
#else /* CONFIG_SOC_SERIES_STM32MP2X */
#define EXTI_LL_INST
#endif /* CONFIG_SOC_SERIES_STM32MP2X */


#define EXTI_LINE_NOT_SUPP_ASSERT(line)			\
{							\
	LOG_ERR("Unsupported line number %u", line);	\
	__ASSERT_NO_MSG(0);				\
}

#define EXTI_LINE_NOP(line)	\
{				\
	ARG_UNUSED(line);	\
}

#if defined(CONFIG_SOC_SERIES_STM32MP1X) || defined(CONFIG_SOC_SERIES_STM32MP13X)

/*
 * NOTE: There is currently no other option than to define LL_ prefixed functions
 * that are missing in the original drivers as stubs for unsupported EXTI lines.
 * This is required for compatibility across all STM32 series, until a more
 * flexible solution is available.
 */
#define LL_EXTI_IsActiveRisingFlag_32_63(line)	0
#define LL_EXTI_IsActiveFallingFlag_32_63(line)	0

#define LL_EXTI_ClearRisingFlag_32_63(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_ClearFallingFlag_32_63(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_GenerateSWI_32_63(line)		EXTI_LINE_NOT_SUPP_ASSERT(line)

#define LL_EXTI_EnableRisingTrig_32_63(line)	EXTI_LINE_NOP(line)
#define LL_EXTI_EnableFallingTrig_32_63(line)	EXTI_LINE_NOP(line)
#define LL_EXTI_DisableRisingTrig_32_63(line)	EXTI_LINE_NOP(line)
#define LL_EXTI_DisableFallingTrig_32_63(line)	EXTI_LINE_NOP(line)
#define LL_EXTI_EnableEvent_32_63(line)		EXTI_LINE_NOP(line)
#define LL_EXTI_DisableEvent_32_63(line)	EXTI_LINE_NOP(line)

#if defined(CONFIG_SOC_SERIES_STM32MP13X)

#define LL_EXTI_IsActiveRisingFlag_64_95(line)	0
#define LL_EXTI_IsActiveFallingFlag_64_95(line)	0

#define LL_EXTI_ClearRisingFlag_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_ClearFallingFlag_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_GenerateSWI_64_95(line)		EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_EnableRisingTrig_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_DisableRisingTrig_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_EnableFallingTrig_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_DisableFallingTrig_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_EnableEvent_64_95(line)		EXTI_LINE_NOT_SUPP_ASSERT(line)
#define LL_EXTI_DisableEvent_64_95(line)	EXTI_LINE_NOT_SUPP_ASSERT(line)

#endif /* !CONFIG_SOC_SERIES_STM32MP13X */

#endif /* CONFIG_SOC_SERIES_STM32MP1X || CONFIG_SOC_SERIES_STM32MP13X */


/* Define general macros line-range independent */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti) || defined(CONFIG_SOC_SERIES_STM32MP2X)

#if defined(CONFIG_SOC_SERIES_STM32MP2X)

#define EXTI_IS_ACTIVE_RISING_FLAG(line_range, line)	\
	CONCAT(LL, _EXTI_IsActiveRisingFlag_##line_range)(EXTI_LL_INST line)
#define EXTI_IS_ACTIVE_FALLING_FLAG(line_range, line)	\
	CONCAT(LL, _EXTI_IsActiveFallingFlag_##line_range)(EXTI_LL_INST line)

#define EXTI_CLEAR_RISING_FLAG(line_range, line)	\
	CONCAT(LL, _EXTI_ClearRisingFlag_##line_range)(EXTI_LL_INST line)
#define EXTI_CLEAR_FALLING_FLAG(line_range, line)	\
	CONCAT(LL, _EXTI_ClearFallingFlag_##line_range)(EXTI_LL_INST line)

#else /* !CONFIG_SOC_SERIES_STM32MP2X */

#define EXTI_IS_ACTIVE_RISING_FLAG(line_range, line)	\
	CONCAT(LL, CPU_NR, _EXTI_IsActiveRisingFlag_##line_range)(EXTI_LL_INST line)
#define EXTI_IS_ACTIVE_FALLING_FLAG(line_range, line)	\
	CONCAT(LL, CPU_NR, _EXTI_IsActiveFallingFlag_##line_range)(EXTI_LL_INST line)

#define EXTI_CLEAR_RISING_FLAG(line_range, line)	\
	CONCAT(LL, CPU_NR, _EXTI_ClearRisingFlag_##line_range)(EXTI_LL_INST line)
#define EXTI_CLEAR_FALLING_FLAG(line_range, line)	\
	CONCAT(LL, CPU_NR, _EXTI_ClearFallingFlag_##line_range)(EXTI_LL_INST line)

#endif /* CONFIG_SOC_SERIES_STM32MP2X */

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


/* Define general macros line-range independent */
#define EXTI_ENABLE_IT(line_range, line)		\
	CONCAT(LL, CPU_NR, _EXTI_EnableIT_##line_range)(EXTI_LL_INST line)
#define EXTI_DISABLE_IT(line_range, line)		\
	CONCAT(LL, CPU_NR, _EXTI_DisableIT_##line_range)(EXTI_LL_INST line)
#define EXTI_ENABLE_EVENT(line_range, line)		\
	CONCAT(LL, CPU_NR, _EXTI_EnableEvent_##line_range)(EXTI_LL_INST line)
#define EXTI_DISABLE_EVENT(line_range, line)		\
	CONCAT(LL, CPU_NR, _EXTI_DisableEvent_##line_range)(EXTI_LL_INST line)
#define EXTI_ENABLE_RISING_TRIG(line_range, line)	\
	CONCAT(LL, _EXTI_EnableRisingTrig_##line_range)(EXTI_LL_INST line)
#define EXTI_ENABLE_FALLING_TRIG(line_range, line)	\
	CONCAT(LL, _EXTI_EnableFallingTrig_##line_range)(EXTI_LL_INST line)
#define EXTI_DISABLE_FALLING_TRIG(line_range, line)	\
	CONCAT(LL, _EXTI_DisableFallingTrig_##line_range)(EXTI_LL_INST line)
#define EXTI_DISABLE_RISING_TRIG(line_range, line)	\
	CONCAT(LL, _EXTI_DisableRisingTrig_##line_range)(EXTI_LL_INST line)
#define EXTI_GENERATE_SWI(line_range, line)		\
	CONCAT(LL, _EXTI_GenerateSWI_##line_range)(EXTI_LL_INST line)

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
