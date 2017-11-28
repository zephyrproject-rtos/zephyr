/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIMPLE_PROCESS_TIMER_MODEL_H
#define _SIMPLE_PROCESS_TIMER_MODEL_H

#include "hw_models_top.h"

#ifdef __cplusplus
extern "C" {
#endif

void hwtimer_init(void);
void hwtimer_cleanup(void);
void hwtimer_timer_reached(void);
void hwtimer_wake_in_time(hwtime_t time);

#ifdef __cplusplus
}
#endif

#endif /* _SIMPLE_PROCESS_TIMER_MODEL_H */
