/*
 * Copyright 2024, Florian Grandel, grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Bluetooth clock hardware abstraction for nRF5x
 *
 * @details Currently, there is no way to access the Bluetooth clock
 * independently of its implementation. The following macros hide implementation
 * details from the application.
 */

#include <stdint.h>

#include <hal/nrf_rtc.h>

#define BT_CLOCK_HAL_CNTR_MASK      0x00FFFFFF
#define BT_CLOCK_HAL_CNTR_UNIT_FSEC 30517578125UL

#define BT_CLOCK_HAL_FSEC_PER_USEC 1000000000UL

#define BT_CLOCK_HAL_TICKS_TO_US(x)                                                                \
	(((uint32_t)(((uint64_t)(x) * BT_CLOCK_HAL_CNTR_UNIT_FSEC) / BT_CLOCK_HAL_FSEC_PER_USEC)))

#define BT_CLOCK_HAL_WRAPPING_POINT_US (BT_CLOCK_HAL_TICKS_TO_US(BT_CLOCK_HAL_CNTR_MASK))
#define BT_CLOCK_HAL_CYCLE_US          (BT_CLOCK_HAL_WRAPPING_POINT_US + 1)

#define hal_ticker_now_ticks() nrf_rtc_counter_get(NRF_RTC0)
#define hal_ticker_now_usec()  BT_CLOCK_HAL_TICKS_TO_US(hal_ticker_now_ticks())
