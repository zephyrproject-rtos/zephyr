/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_RTC_LL_STM32_H_
#define ZEPHYR_DRIVERS_RTC_RTC_LL_STM32_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_RTC_ALARM

/* STM32 RTC alarms, A & B, LL masks are equal */
#define RTC_STM32_ALRM_MASK_ALL		LL_RTC_ALMA_MASK_ALL
#define RTC_STM32_ALRM_MASK_SECONDS	LL_RTC_ALMA_MASK_SECONDS
#define RTC_STM32_ALRM_MASK_MINUTES	LL_RTC_ALMA_MASK_MINUTES
#define RTC_STM32_ALRM_MASK_HOURS	LL_RTC_ALMA_MASK_HOURS
#define RTC_STM32_ALRM_MASK_DATEWEEKDAY	LL_RTC_ALMA_MASK_DATEWEEKDAY

#define RTC_STM32_ALRM_DATEWEEKDAYSEL_WEEKDAY	LL_RTC_ALMA_DATEWEEKDAYSEL_WEEKDAY
#define RTC_STM32_ALRM_DATEWEEKDAYSEL_DATE	LL_RTC_ALMA_DATEWEEKDAYSEL_DATE

static inline void ll_func_exti_enable_rtc_alarm_it(uint32_t linenum);
static inline void ll_func_exti_clear_rtc_alarm_flag(uint32_t linenum);

#endif /* CONFIG_RTC_ALARM */

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_DRIVERS_RTC_RTC_LL_STM32_H_ */
