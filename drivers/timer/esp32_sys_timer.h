/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_TIMER_ESP32_SYS_TIMER_H_
#define ZEPHYR_DRIVERS_TIMER_ESP32_SYS_TIMER_H_

#include <stdint.h>

/* Callback hooks on low power mode entry/exit */

uint64_t esp32_lptim_hook_on_lpm_entry(uint64_t max_lpm_time_us);
uint64_t esp32_lptim_hook_on_lpm_exit(void);
uint64_t esp32_lptim_hook_get_freq(void);

#endif /* ZEPHYR_DRIVERS_TIMER_ESP32_SYS_TIMER_H_ */
