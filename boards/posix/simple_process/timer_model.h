/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIMPLE_PROCESS_TIMER_MODEL_H
#define _SIMPLE_PROCESS_TIMER_MODEL_H


#ifdef __cplusplus
extern "C" {
#endif

void hwtimer_init(void);
void hwtimer_cleanup(void);
void hwtimer_timer_reached(void);


#ifdef __cplusplus
}
#endif

#endif /* _SIMPLE_PROCESS_TIMER_MODEL_H */
