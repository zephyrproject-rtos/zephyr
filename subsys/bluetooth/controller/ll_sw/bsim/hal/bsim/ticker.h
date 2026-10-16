/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The ticker counts the free running 32-bit 1MHz counter of the radio model,
 * which takes a compare value at least one tick ahead of the counter.
 */
#define HAL_TICKER_CNTR_CLK_UNIT_FSEC 1000000000UL
#define HAL_TICKER_CNTR_MSBIT 31
#define HAL_TICKER_CNTR_MASK UINT32_MAX
#define HAL_TICKER_CNTR_CMP_OFFSET_MIN 1
#define HAL_TICKER_CNTR_SET_LATENCY 5

#define HAL_TICKER_FSEC_PER_USEC      1000000000UL
#define HAL_TICKER_PSEC_PER_USEC      1000000UL
#define HAL_TICKER_FSEC_PER_PSEC      1000UL

/* Rounds down */
#define HAL_TICKER_US_TO_TICKS(x) \
	( \
		((uint32_t)(((uint64_t)(x) * HAL_TICKER_FSEC_PER_USEC) / \
			    HAL_TICKER_CNTR_CLK_UNIT_FSEC)) & \
		HAL_TICKER_CNTR_MASK \
	)

/* Rounds up */
#define HAL_TICKER_US_TO_TICKS_CEIL(x) \
	( \
		DIV_ROUND_UP(((uint64_t)(x) * HAL_TICKER_FSEC_PER_USEC), \
			     HAL_TICKER_CNTR_CLK_UNIT_FSEC) & \
		HAL_TICKER_CNTR_MASK \
	)

#define HAL_TICKER_TICKS_TO_US_64BIT(x) \
	( \
		(((uint64_t)(x) * HAL_TICKER_CNTR_CLK_UNIT_FSEC) / \
		 HAL_TICKER_FSEC_PER_USEC) \
	)

#define HAL_TICKER_TICKS_TO_US(x) ((uint32_t)HAL_TICKER_TICKS_TO_US_64BIT(x))

/* In picoseconds, to fit in 32 bits */
#define HAL_TICKER_REMAINDER(x) \
	( \
		( \
			((uint64_t)(x) * HAL_TICKER_FSEC_PER_USEC) \
			- ((uint64_t)HAL_TICKER_US_TO_TICKS(x) * \
			   HAL_TICKER_CNTR_CLK_UNIT_FSEC) \
		) \
		/ HAL_TICKER_FSEC_PER_PSEC \
	)

#define HAL_TICKER_REMAINDER_RANGE \
	HAL_TICKER_TICKS_TO_US(HAL_TICKER_PSEC_PER_USEC)

#define HAL_TICKER_RESCHEDULE_MARGIN_US 150U
#define HAL_TICKER_RESCHEDULE_MARGIN \
	HAL_TICKER_US_TO_TICKS(HAL_TICKER_RESCHEDULE_MARGIN_US)

static inline void hal_ticker_remove_jitter(uint32_t *ticks,
					    uint32_t *remainder)
{
	if (((*remainder & BIT(31)) != 0U) || ((*remainder / HAL_TICKER_PSEC_PER_USEC) == 0U)) {
		*ticks -= 1U;
		*remainder += HAL_TICKER_CNTR_CLK_UNIT_FSEC / HAL_TICKER_FSEC_PER_PSEC;
	}

	*remainder /= HAL_TICKER_PSEC_PER_USEC;
}

static inline void hal_ticker_add_jitter(uint32_t *ticks, uint32_t *remainder)
{
	if (((*remainder & BIT(31)) != 0U) || ((*remainder / HAL_TICKER_PSEC_PER_USEC) == 0U)) {
		*ticks += 1U;
		*remainder += HAL_TICKER_CNTR_CLK_UNIT_FSEC / HAL_TICKER_FSEC_PER_PSEC;
	}

	*remainder /= HAL_TICKER_PSEC_PER_USEC;
}
