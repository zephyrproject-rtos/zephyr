/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API to the native_posix (Real) Time Clock
 */


#ifndef _NATIVE_POSIX_RTC_H
#define _NATIVE_POSIX_RTC_H

#include "hw_models_top.h"
#include <stdbool.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Types of clocks this RTC provides:
 */
/** Time since boot, cannot be offset. Microsecond resolution */
#define RTC_CLOCK_BOOT               0
/** Persistent clock, can be offset. Microsecond resolution */
#define RTC_CLOCK_REALTIME           1
/**
 * Pseudo-host real time clock (Please see documentation).
 * Nanosecond resolution
 */
#define RTC_CLOCK_PSEUDOHOSTREALTIME 2

/**
 * @brief Get the value of a clock in microseconds
 *
 * @param clock_type Which clock to measure from
 *
 * @return Number of microseconds
 */
uint64_t native_rtc_gettime_us(int clock_type);

/**
 * @brief Get the value of a clock split in nsec and seconds
 *
 * @param clock_type Which clock to measure from
 * @param nsec Pointer to store the nanoseconds
 * @param nsec Pointer to store the seconds
 */
void native_rtc_gettime(int clock_type, uint32_t *nsec, uint64_t *sec);

/**
 * @brief Offset the real time clock by a number of microseconds.
 * Note that this only affects the RTC_CLOCK_REALTIME and
 * RTC_CLOCK_PSEUDOHOSTREALTIME clocks.
 *
 * @param delta_us Number of microseconds to offset. The value is added to all
 * offsetable clocks.
 */
void native_rtc_offset(int64_t delta_us);

/**
 * @brief Adjust the speed of the clock source by a multiplicative factor
 *
 * @param clock_correction Factor by which to correct the clock speed
 */
void native_rtc_adjust_clock(double clock_correction);

#ifdef __cplusplus
}
#endif

#endif /* _NATIVE_POSIX_RTC_H */
