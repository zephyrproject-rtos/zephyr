/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NATIVE_POSIX_802154_UTILS_H__
#define NATIVE_POSIX_802154_UTILS_H__

#include <assert.h>
#include <stdint.h>

/**
 * @defgroup nrf_802154_utils Utils definitions used in the 802.15.4 driver
 * @{
 * @ingroup nrf_802154
 * @brief Definitions of utils used in the 802.15.4 driver.
 */

/**@brief RTC clock frequency. */
#define NRF_802154_RTC_FREQUENCY 32768UL

/**@brief Defines the number of microseconds in one second. */
#define NRF_802154_US_PER_S 1000000ULL

/**@brief Number of bits to shift RTC_FREQUENCY and US_PER_S to achieve the
 * division by greatest common divisor.
 */
#define NRF_802154_FREQUENCY_US_PER_S_GCD_BITS 6

/**@brief Ceil division helper. */
#define NRF_802154_DIVIDE_AND_CEIL(A, B) (((A) + (B)-1) / (B))

/**@brief Macro to get the number of elements in an array.
 *
 * @param[in] X   Array.
 */
#define NUMELTS(X) (sizeof((X)) / sizeof(X[0]))

/**
 *@}
 **/

#endif /* NRF_802154_UTILS_H__ */
