/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NATIVE_POSIX_HW_COUNTER_H
#define _NATIVE_POSIX_HW_COUNTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hw_counter_init(void);
void hw_counter_triggered(void);

void hw_counter_set_period(uint64_t period);
void hw_counter_set_target(uint64_t counter_target);
void hw_counter_start(void);
void hw_counter_stop(void);
uint64_t hw_counter_get_value(void);

#ifdef __cplusplus
}
#endif

#endif /* _NATIVE_POSIX_HW_COUNTER_H */
