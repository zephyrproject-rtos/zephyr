/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define HAL_TICKER_CNTR_CLK_FREQ_HZ 32768U

#define HAL_TICKER_CNTR_CMP_OFFSET_MIN 2

#define HAL_TICKER_CNTR_SET_LATENCY 1
/*
 * When the LPTMR is enabled, the first increment will take an additional
 * one or two prescaler clock cycles due to synchronization logic.
 */

#define HAL_TICKER_US_TO_TICKS(x) \
	( \
		((uint32_t)(((uint64_t) (x) * 1000000000UL) / 30517578125UL)) \
		& HAL_TICKER_CNTR_MASK \
	)

#define HAL_TICKER_REMAINDER(x) \
	( \
		( \
			((uint64_t) (x) * 1000000000UL) \
			- ((uint64_t)HAL_TICKER_US_TO_TICKS(x) * 30517578125UL) \
		) \
		/ 1000UL \
	)

#define HAL_TICKER_TICKS_TO_US(x) \
	((uint32_t)(((uint64_t)(x) * 30517578125UL) / 1000000000UL))

#define HAL_TICKER_CNTR_MSBIT 31

#define HAL_TICKER_CNTR_MASK 0xFFFFFFFF

/* Macro defining the remainder resolution/range
 * ~ 1000000 * HAL_TICKER_TICKS_TO_US(1)
 */
#define HAL_TICKER_REMAINDER_RANGE \
	HAL_TICKER_TICKS_TO_US(1000000)

/* Macro defining the margin for positioning re-scheduled nodes */
#define HAL_TICKER_RESCHEDULE_MARGIN \
	HAL_TICKER_US_TO_TICKS(150)
