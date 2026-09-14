/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_SOC_CLOCK_H_
#define ZEPHYR_SOC_REALTEK_RTS5912_SOC_CLOCK_H_

#include <stdint.h>

#if defined(CONFIG_PM)
void rts5912_clock_capture_low_freq_timer(void);
void rts5912_clock_compensate_system_timer(void);
uint64_t rts5912_clock_get_sleep_ticks(void);
#endif

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_SOC_CLOCK_H_ */
