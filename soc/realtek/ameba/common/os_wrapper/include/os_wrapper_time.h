/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_TIME_H__
#define __OS_WRAPPER_TIME_H__

#define RTOS_TICK_RATE_HZ 1000UL
#define RTOS_TICK_RATE_MS (1000UL / RTOS_TICK_RATE_HZ)

#define RTOS_TIME_GET_PASSING_TIME_MS(start) (rtos_time_get_current_system_time_ms() - (start))

#define RTOS_TIME_GET_TIME_INTERVAL_MS(start, end) ((end) - (start))

/**
 * @brief  If the current system is in a scheduling and non-interrupted state, it will switch to
 * other tasks. Otherwise, the nop instruction will be executed.
 * @param  ms: Delay time in milliseconds
 */
void rtos_time_delay_ms(uint32_t ms);

/**
 * @brief  The system will execute the nop instruction without scheduling.
 * @param  us: Delay time in microseconds
 */
void rtos_time_delay_us(uint32_t us);

#endif
