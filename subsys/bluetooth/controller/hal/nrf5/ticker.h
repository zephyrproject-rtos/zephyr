/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Macro to translate microseconds to tick units.
 * NOTE: This returns the floor value.
 */
#define HAL_TICKER_US_TO_TICKS(x) \
	( \
		((u32_t)(((u64_t) (x) * 1000000000UL) / 30517578125UL)) \
		& 0x00FFFFFF \
	)

/* Macro returning remainder in nanoseconds */
#define HAL_TICKER_REMAINDER(x) \
	( \
		( \
			((u64_t) (x) * 1000000000UL) \
			- ((u64_t)HAL_TICKER_US_TO_TICKS(x) * 30517578125UL) \
		) \
		/ 1000UL \
	)

/* Macro to translate tick units to microseconds. */
#define HAL_TICKER_TICKS_TO_US(x) \
	((u32_t)(((u64_t) (x) * 30517578125UL) / 1000000000UL))

/* Macro defines the h/w supported most significant bit */
#define HAL_TICKER_MSBIT 23
