/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define HAL_TICKER_CNTR_CLK_FREQ_HZ 32768U
#define HAL_TICKER_CNTR_CLK_UNIT_FS 30517578125UL

/* Macro defining the minimum counter compare offset */
#define HAL_TICKER_CNTR_CMP_OFFSET_MIN 3

/* Macro defining the max. counter update latency in ticks */
#define HAL_TICKER_CNTR_SET_LATENCY 0

/* Macro defines the h/w supported most significant bit */
#define HAL_TICKER_CNTR_MSBIT 23

/* Macro defining the HW supported counter bits */
#define HAL_TICKER_CNTR_MASK 0x00FFFFFF

/* Macro to translate microseconds to tick units.
 * NOTE: This returns the floor value.
 */
#define HAL_TICKER_US_TO_TICKS(x) \
	( \
		((uint32_t)(((uint64_t) (x) * 1000000000UL) / \
		 HAL_TICKER_CNTR_CLK_UNIT_FS)) & HAL_TICKER_CNTR_MASK \
	)

/* Macro to translate tick units to microseconds. */
#define HAL_TICKER_TICKS_TO_US(x) \
	( \
		((uint32_t)(((uint64_t)(x) * HAL_TICKER_CNTR_CLK_UNIT_FS) / \
		 1000000000UL)) \
	)

/* Macro returning remainder in picoseconds (to fit in 32-bits) */
#define HAL_TICKER_REMAINDER(x) \
	( \
		( \
			((uint64_t) (x) * 1000000000UL) \
			- ((uint64_t)HAL_TICKER_US_TO_TICKS(x) * \
			   HAL_TICKER_CNTR_CLK_UNIT_FS) \
		) \
		/ 1000UL \
	)

/* Macro defining the remainder resolution/range
 * ~ 1000000 * HAL_TICKER_TICKS_TO_US(1)
 */
#define HAL_TICKER_REMAINDER_RANGE \
	HAL_TICKER_TICKS_TO_US(1000000)

/* Macro defining the margin for positioning re-scheduled nodes */
#define HAL_TICKER_RESCHEDULE_MARGIN \
	HAL_TICKER_US_TO_TICKS(150)

/* Remove ticks and return positive remainder value in microseconds */
static inline void hal_ticker_remove_jitter(uint32_t *ticks,
					    uint32_t *remainder)
{
	/* Is remainder less than 1 us */
	if ((*remainder & BIT(31)) || !(*remainder / 1000000UL)) {
		*ticks -= 1U;
		*remainder += HAL_TICKER_CNTR_CLK_UNIT_FS / 1000UL;
	}

	/* pico seconds to micro seconds unit */
	*remainder /= 1000000UL;
}

/* Add ticks and return positive remainder value in microseconds */
static inline void hal_ticker_add_jitter(uint32_t *ticks, uint32_t *remainder)
{
	/* Is remainder less than 1 us */
	if ((*remainder & BIT(31)) || !(*remainder / 1000000UL)) {
		*ticks += 1U;
		*remainder += HAL_TICKER_CNTR_CLK_UNIT_FS / 1000UL;
	}

	/* pico seconds to micro seconds unit */
	*remainder /= 1000000UL;
}
