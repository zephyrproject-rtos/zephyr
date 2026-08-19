/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ITE_EC_SOC_TIMER_H_
#define _ITE_EC_SOC_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

bool ite_it8xxx2_timer_block_idle(void);

#ifdef CONFIG_PM
void ite_ec_clock_capture_low_freq_timer(void);
void ite_ec_clock_compensate_system_timer(void);
uint64_t ite_ec_clock_get_sleep_ticks(void);
#else
static inline void ite_ec_clock_capture_low_freq_timer(void)
{
}
static inline void ite_ec_clock_compensate_system_timer(void)
{
}
static inline uint64_t ite_ec_clock_get_sleep_ticks(void)
{
	return 0;
}
#endif /* CONFIG_PM */

#ifdef __cplusplus
}
#endif

#endif /* _ITE_EC_SOC_TIMER_H_ */
