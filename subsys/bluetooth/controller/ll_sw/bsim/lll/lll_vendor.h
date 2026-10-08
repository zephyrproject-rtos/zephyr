/*
 * Copyright (c) 2018-2025 Nordic Semiconductor ASA
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* A simulated CPU takes no time and the simulated clocks are always running,
 * so the overheads are the ones of the Nordic LLL on nRF52, for the ULL to
 * schedule events as it does on hardware.
 */
#define EVENT_OVERHEAD_XTAL_US        1500
#define EVENT_OVERHEAD_PREEMPT_US     0    /* if <= min, then dynamic preempt */
#define EVENT_OVERHEAD_PREEMPT_MIN_US 0
#define EVENT_OVERHEAD_PREEMPT_MAX_US EVENT_OVERHEAD_XTAL_US

#define EVENT_OVERHEAD_START_US       275

#define EVENT_OVERHEAD_END_US         40
#define EVENT_JITTER_US               16
#define EVENT_TIES_US                 625

/* ticker_start and ticker_update take whole ticks */
#define EVENT_TICKER_RES_MARGIN_US DIV_ROUND_UP(HAL_TICKER_CNTR_CLK_UNIT_FSEC, \
						HAL_TICKER_FSEC_PER_USEC)

#define EVENT_RX_JITTER_US(phy) 16
#define EVENT_RX_TO_US(phy) \
	((((((phy) & BIT_MASK(2)) + 4U) << 3) / BIT(((phy) & BIT_MASK(2)) >> 1)) + \
	 EVENT_RX_JITTER_US(phy))

#define EVENT_RX_TX_TURNAROUND(phy)   150

/* The ticker counts microseconds, so no fraction of a microsecond is kept */
#define EVENT_US_TO_US_FRAC(us)             (us)
#define EVENT_US_FRAC_TO_US(us_frac)        (us_frac)
#define EVENT_TICKS_TO_US_FRAC(ticks)       HAL_TICKER_TICKS_TO_US(ticks)
#define EVENT_US_FRAC_TO_TICKS(us_frac)     HAL_TICKER_US_TO_TICKS(us_frac)
#define EVENT_US_FRAC_TO_REMAINDER(us_frac) HAL_TICKER_REMAINDER(us_frac)
