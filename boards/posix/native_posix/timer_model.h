/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NATIVE_POSIX_TIMER_MODEL_H
#define _NATIVE_POSIX_TIMER_MODEL_H

#include "hw_models_top.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void hwtimer_init(void);
void hwtimer_cleanup(void);
void hwtimer_set_real_time_mode(bool new_rt);
void hwtimer_timer_reached(void);
void hwtimer_wake_in_time(u64_t time);
void hwtimer_set_silent_ticks(s64_t sys_ticks);
void hwtimer_enable(u64_t period);
s64_t hwtimer_get_pending_silent_ticks(void);

void hwtimer_reset_rtc(void);
void hwtimer_set_rtc_offset(s64_t offset);
void hwtimer_set_rt_ratio(double ratio);

void hwtimer_adjust_rtc_offset(s64_t offset_delta);
void hwtimer_adjust_rt_ratio(double ratio_correction);
s64_t hwtimer_get_simu_rtc_time(void);
void hwtimer_get_pseudohost_rtc_time(u32_t *nsec, u64_t *sec);

#ifdef __cplusplus
}
#endif

#endif /* _NATIVE_POSIX_TIMER_MODEL_H */
