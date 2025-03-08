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

#include <stm32_ll_exti.h>

#ifdef __cplusplus
extern "C" {
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)

#define EXTI_IS_ACTIVE_RISING_FLAG_0_31(line)  LL_EXTI_IsActiveRisingFlag_0_31(line)
#define EXTI_IS_ACTIVE_FALLING_FLAG_0_31(line) LL_EXTI_IsActiveFallingFlag_0_31(line)
#define LL_EXTI_IsActiveFlag_0_31(line) \
	(EXTI_IS_ACTIVE_RISING_FLAG_0_31(line) || EXTI_IS_ACTIVE_FALLING_FLAG_0_31(line))

#define EXTI_CLEAR_RISING_FLAG_0_31(line)  LL_EXTI_ClearRisingFlag_0_31(line)
#define EXTI_CLEAR_FALLING_FLAG_0_31(line) LL_EXTI_ClearFallingFlag_0_31(line)
#define LL_EXTI_ClearFlag_0_31(line) \
	{ EXTI_CLEAR_RISING_FLAG_0_31(line); EXTI_CLEAR_FALLING_FLAG_0_31(line); }

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti) */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_comp) && defined(CONFIG_CPU_CORTEX_M4)
#define CPU_NR _C2
#else /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_comp) && defined(CONFIG_CPU_CORTEX_M4) */
#define CPU_NR
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_comp) && defined(CONFIG_CPU_CORTEX_M4) */

#define EXTI_ENABLE_IT_0_31(line)		CONCAT(LL, CPU_NR, _EXTI_EnableIT_0_31)(line)
#define EXTI_DISABLE_IT_0_31(line)		CONCAT(LL, CPU_NR, _EXTI_DisableIT_0_31)(line)
#define EXTI_ENABLE_EVENT_0_31(line)		CONCAT(LL, CPU_NR, _EXTI_EnableEvent_0_31)(line)
#define EXTI_DISABLE_EVENT_0_31(line)		CONCAT(LL, CPU_NR, _EXTI_DisableEvent_0_31)(line)
#define EXTI_CLEAR_FLAG_0_31(line)		CONCAT(LL, CPU_NR, _EXTI_ClearFlag_0_31)(line)
#define EXTI_IS_ACTIVE_FLAG_0_31(line)		CONCAT(LL, CPU_NR, _EXTI_IsActiveFlag_0_31)(line)
#define EXTI_ENABLE_RISING_TRIG_0_31(line)	CONCAT(LL, _EXTI_EnableRisingTrig_0_31)(line)
#define EXTI_ENABLE_FALLING_TRIG_0_31(line)	CONCAT(LL, _EXTI_EnableFallingTrig_0_31)(line)
#define EXTI_DISABLE_FALLING_TRIG_0_31(line)	CONCAT(LL, _EXTI_DisableFallingTrig_0_31)(line)
#define EXTI_DISABLE_RISING_TRIG_0_31(line)	CONCAT(LL, _EXTI_DisableRisingTrig_0_31)(line)


#ifdef CONFIG_EXTI_STM32_HAS_64_LINES

#define EXTI_ENABLE_IT_32_63(line)		CONCAT(LL, CPU_NR, _EXTI_EnableIT_32_63)(line)
#define EXTI_DISABLE_IT_32_63(line)		CONCAT(LL, CPU_NR, _EXTI_DisableIT_32_63)(line)
#define EXTI_ENABLE_EVENT_32_63(line)		CONCAT(LL, CPU_NR, _EXTI_EnableEvent_32_63)(line)
#define EXTI_DISABLE_EVENT_32_63(line)		CONCAT(LL, CPU_NR, _EXTI_DisableEvent_32_63)(line)
#define EXTI_CLEAR_FLAG_32_63(line)		CONCAT(LL, CPU_NR, _EXTI_ClearFlag_32_63)(line)
#define EXTI_IS_ACTIVE_FLAG_32_63(line)		CONCAT(LL, CPU_NR, _EXTI_IsActiveFlag_32_63)(line)
#define EXTI_ENABLE_RISING_TRIG_32_63(line)	CONCAT(LL, _EXTI_EnableRisingTrig_32_63)(line)
#define EXTI_ENABLE_FALLING_TRIG_32_63(line)	CONCAT(LL, _EXTI_EnableFallingTrig_32_63)(line)
#define EXTI_DISABLE_FALLING_TRIG_32_63(line)	CONCAT(LL, _EXTI_DisableFallingTrig_32_63)(line)
#define EXTI_DISABLE_RISING_TRIG_32_63(line)	CONCAT(LL, _EXTI_DisableRisingTrig_32_63)(line)

#endif /* CONFIG_EXTI_STM32_HAS_64_LINES */


#ifdef CONFIG_EXTI_STM32_HAS_96_LINES

#ifndef CONFIG_EXTI_STM32_HAS_64_LINES
#error "CONFIG_EXTI_STM32_HAS_96_LINES requires CONFIG_EXTI_STM32_HAS_64_LINES"
#endif

#define EXTI_ENABLE_IT_64_95(line)		CONCAT(LL, CPU_NR, _EXTI_EnableIT_64_95)(line)
#define EXTI_DISABLE_IT_64_95(line)		CONCAT(LL, CPU_NR, _EXTI_DisableIT_64_95)(line)
#define EXTI_ENABLE_EVENT_64_95(line)		CONCAT(LL, CPU_NR, _EXTI_EnableEvent_64_95)(line)
#define EXTI_DISABLE_EVENT_64_95(line)		CONCAT(LL, CPU_NR, _EXTI_DisableEvent_64_95)(line)
#define EXTI_CLEAR_FLAG_64_95(line)		CONCAT(LL, CPU_NR, _EXTI_ClearFlag_64_95)(line)
#define EXTI_IS_ACTIVE_FLAG_64_95(line)		CONCAT(LL, CPU_NR, _EXTI_IsActiveFlag_64_95)(line)
#define EXTI_ENABLE_RISING_TRIG_64_95(line)	CONCAT(LL, _EXTI_EnableRisingTrig_64_95)(line)
#define EXTI_ENABLE_FALLING_TRIG_64_95(line)	CONCAT(LL, _EXTI_EnableFallingTrig_64_95)(line)
#define EXTI_DISABLE_FALLING_TRIG_64_95(line)	CONCAT(LL, _EXTI_DisableFallingTrig_64_95)(line)
#define EXTI_DISABLE_RISING_TRIG_64_95(line)	CONCAT(LL, _EXTI_DisableRisingTrig_64_95)(line)

#endif /* CONFIG_EXTI_STM32_HAS_96_LINES */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_PRIV_H_ */
