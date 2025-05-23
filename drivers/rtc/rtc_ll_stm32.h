/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_RTC_LL_STM32_H_
#define ZEPHYR_DRIVERS_RTC_RTC_LL_STM32_H_

/**
 * ES0584 / ES0631 §2.5.2; ES0632 §2.6.2 (both Rev. 2)
 * """
 * RTC interrupts cannot be reliably used for real-time
 * control functions, since some occurences of RTC
 * interrupts may be missed.
 * """
 * Since alarm IRQs are unreliable, don't allow RTC alarm
 * to be used on STM32WB0 series. For this, we have to
 * create a #define only valid when both the Kconfig is
 * enabled, and we're on a supported series. This must
 * be done because the RTC driver has to build properly
 * on all targets regardless of which Kconfig options have
 * been enabled.
 */
#if defined(CONFIG_RTC_ALARM) && !defined(CONFIG_SOC_SERIES_STM32WB0X)
#define STM32_RTC_ALARM_ENABLED	1

/* STM32 RTC alarms, A & B, LL masks are equal */
#define RTC_STM32_ALRM_MASK_ALL		LL_RTC_ALMA_MASK_ALL
#define RTC_STM32_ALRM_MASK_SECONDS	LL_RTC_ALMA_MASK_SECONDS
#define RTC_STM32_ALRM_MASK_MINUTES	LL_RTC_ALMA_MASK_MINUTES
#define RTC_STM32_ALRM_MASK_HOURS	LL_RTC_ALMA_MASK_HOURS
#define RTC_STM32_ALRM_MASK_DATEWEEKDAY	LL_RTC_ALMA_MASK_DATEWEEKDAY

#define RTC_STM32_ALRM_DATEWEEKDAYSEL_WEEKDAY	LL_RTC_ALMA_DATEWEEKDAYSEL_WEEKDAY
#define RTC_STM32_ALRM_DATEWEEKDAYSEL_DATE	LL_RTC_ALMA_DATEWEEKDAYSEL_DATE

static inline void ll_func_exti_enable_rtc_alarm_it(uint32_t exti_line)
{
#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_EnableIT_0_31(exti_line);
	LL_EXTI_EnableRisingTrig_0_31(exti_line);
#elif defined(CONFIG_SOC_SERIES_STM32U5X) || defined(CONFIG_SOC_SERIES_STM32WBAX)
	/* in STM32U5 & STM32WBAX series, RTC Alarm event is not routed to EXTI */
#else
	LL_EXTI_EnableIT_0_31(exti_line);
	LL_EXTI_EnableRisingTrig_0_31(exti_line);
#endif /* CONFIG_SOC_SERIES_STM32H7X and CONFIG_CPU_CORTEX_M4 */
}

static inline void ll_func_exti_clear_rtc_alarm_flag(uint32_t exti_line)
{
#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_ClearFlag_0_31(exti_line);
#elif defined(CONFIG_SOC_SERIES_STM32U5X) || defined(CONFIG_SOC_SERIES_STM32WBAX)
	/* in STM32U5 & STM32WBAX series, RTC Alarm event is not routed to EXTI */
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)
	LL_EXTI_ClearRisingFlag_0_31(exti_line);
#else
	LL_EXTI_ClearFlag_0_31(exti_line);
#endif /* CONFIG_SOC_SERIES_STM32H7X and CONFIG_CPU_CORTEX_M4 */
}
#endif /* CONFIG_RTC_ALARM && !CONFIG_SOC_SERIES_STM32WB0X */

#endif	/* ZEPHYR_DRIVERS_RTC_RTC_LL_STM32_H_ */
