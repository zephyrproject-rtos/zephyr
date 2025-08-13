/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_NRF_GRTC)
#define HAL_TICKER_CNTR_CLK_UNIT_FSEC 1000000000UL

/* Macro defines the h/w supported most significant bit */
#define HAL_TICKER_CNTR_MSBIT 31

/* Macro defining the HW supported counter bits */
#define HAL_TICKER_CNTR_MASK 0xFFFFFFFF

/* Macro defining the minimum counter compare offset */
#define HAL_TICKER_CNTR_CMP_OFFSET_MIN 1

/* Macro defining the max. counter update latency in ticks */
#define HAL_TICKER_CNTR_SET_LATENCY 4

#else /* !CONFIG_BT_CTLR_NRF_GRTC */
#define HAL_TICKER_CNTR_CLK_UNIT_FSEC 30517578125UL

/* Macro defines the h/w supported most significant bit */
#define HAL_TICKER_CNTR_MSBIT 23

/* Macro defining the HW supported counter bits */
#define HAL_TICKER_CNTR_MASK 0x00FFFFFF

/* Macro defining the minimum counter compare offset */
#define HAL_TICKER_CNTR_CMP_OFFSET_MIN 3

/* Macro defining the max. counter update latency in ticks */
#define HAL_TICKER_CNTR_SET_LATENCY 0
#endif /* !CONFIG_BT_CTLR_NRF_GRTC */

#define HAL_TICKER_FSEC_PER_USEC      1000000000UL
#define HAL_TICKER_PSEC_PER_USEC      1000000UL
#define HAL_TICKER_FSEC_PER_PSEC      1000UL

/* Macro to translate microseconds to tick units.
 * NOTE: This returns the floor value.
 */
#define HAL_TICKER_US_TO_TICKS(x) \
	( \
		((uint32_t)(((uint64_t) (x) * HAL_TICKER_FSEC_PER_USEC) / \
			    HAL_TICKER_CNTR_CLK_UNIT_FSEC)) & \
		HAL_TICKER_CNTR_MASK \
	)

/* Macro to translate microseconds to tick units.
 * NOTE: This returns the ceil value.
 */
#define HAL_TICKER_US_TO_TICKS_CEIL(x) \
	( \
		DIV_ROUND_UP(((uint64_t) (x) * HAL_TICKER_FSEC_PER_USEC), \
			     HAL_TICKER_CNTR_CLK_UNIT_FSEC) & \
		HAL_TICKER_CNTR_MASK \
	)

/* Macro to translate tick units to microseconds. */
#define HAL_TICKER_TICKS_TO_US_64BIT(x) \
	( \
		(((uint64_t)(x) * HAL_TICKER_CNTR_CLK_UNIT_FSEC) / \
		 HAL_TICKER_FSEC_PER_USEC) \
	)

#define HAL_TICKER_TICKS_TO_US(x) ((uint32_t)HAL_TICKER_TICKS_TO_US_64BIT(x))

/* Macro returning remainder in picoseconds (to fit in 32-bits) */
#define HAL_TICKER_REMAINDER(x) \
	( \
		( \
			((uint64_t) (x) * HAL_TICKER_FSEC_PER_USEC) \
			- ((uint64_t)HAL_TICKER_US_TO_TICKS(x) * \
			   HAL_TICKER_CNTR_CLK_UNIT_FSEC) \
		) \
		/ HAL_TICKER_FSEC_PER_PSEC \
	)

/* Macro defining the remainder resolution/range
 * ~ 1000000 * HAL_TICKER_TICKS_TO_US(1)
 */
#define HAL_TICKER_REMAINDER_RANGE \
	HAL_TICKER_TICKS_TO_US(HAL_TICKER_PSEC_PER_USEC)

/* Macro defining the margin for positioning re-scheduled nodes.
 *
 * NOTE: This is also used as the margin to detect short expire value and defer
 *       the ticker_job from processing bottom-halves, management and inquire
 *       requests.
 *
 * FIXME: Profile the application for worst-case ticker_resolve_collision() CPU
 *        execution time and use it as the margin.
 */
#define HAL_TICKER_RESCHEDULE_MARGIN_US 150U
#define HAL_TICKER_RESCHEDULE_MARGIN \
	HAL_TICKER_US_TO_TICKS(HAL_TICKER_RESCHEDULE_MARGIN_US)

/* Remove ticks and return positive remainder value in microseconds */
static inline void hal_ticker_remove_jitter(uint32_t *ticks,
					    uint32_t *remainder)
{
	/* Is remainder less than 1 us */
	if ((*remainder & BIT(31)) || !(*remainder / HAL_TICKER_PSEC_PER_USEC)) {
		*ticks -= 1U;
		*remainder += HAL_TICKER_CNTR_CLK_UNIT_FSEC / HAL_TICKER_FSEC_PER_PSEC;
	}

	/* pico seconds to micro seconds unit */
	*remainder /= HAL_TICKER_PSEC_PER_USEC;
}

/* Add ticks and return positive remainder value in microseconds */
static inline void hal_ticker_add_jitter(uint32_t *ticks, uint32_t *remainder)
{
	/* Is remainder less than 1 us */
	if ((*remainder & BIT(31)) || !(*remainder / HAL_TICKER_PSEC_PER_USEC)) {
		*ticks += 1U;
		*remainder += HAL_TICKER_CNTR_CLK_UNIT_FSEC / HAL_TICKER_FSEC_PER_PSEC;
	}

	/* pico seconds to micro seconds unit */
	*remainder /= HAL_TICKER_PSEC_PER_USEC;
}
