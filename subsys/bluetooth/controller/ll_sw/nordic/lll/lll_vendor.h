/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define EVENT_OVERHEAD_XTAL_US        1500
#define EVENT_OVERHEAD_PREEMPT_US     0    /* if <= min, then dynamic preempt */
#define EVENT_OVERHEAD_PREEMPT_MIN_US 0
#define EVENT_OVERHEAD_PREEMPT_MAX_US EVENT_OVERHEAD_XTAL_US

/* Measurement based on drifting roles that can overlap leading to collision
 * resolutions that consume CPU time between radio events.
 * Value include max end, start and scheduling CPU usage times.
 * Measurements based on central_gatt_write and peripheral_gatt_write sample on
 * nRF52833 SoC.
 */
#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_CTLR_PHY_CODED)
/* Active connection in peripheral role with extended scanning on 1M and Coded
 * PHY, scheduling and receiving auxiliary PDUs.
 */
#define EVENT_OVERHEAD_START_US       458
#else /* !CONFIG_BT_CTLR_PHY_CODED */
/* Active connection in peripheral role with extended scanning on 1M only,
 * scheduling and receiving auxiliary PDUs.
 */
#define EVENT_OVERHEAD_START_US       428
#endif /* !CONFIG_BT_CTLR_PHY_CODED */
#else /* !CONFIG_BT_OBSERVER */
/* Active connection in peripheral role with legacy scanning on 1M.
 */
#define EVENT_OVERHEAD_START_US       275
#endif /* !CONFIG_BT_OBSERVER */
#else /* !CONFIG_BT_CTLR_ADV_EXT */
/* Active connection in peripheral role with additional advertising state.
 */
#define EVENT_OVERHEAD_START_US       275
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

/* Worst-case time margin needed after event end-time in the air
 * (done/preempt race margin + power-down/chain delay)
 */
#define EVENT_OVERHEAD_END_US         100

/* Sleep Clock Accuracy */
#define EVENT_JITTER_US               16

/* Inter-Event Space (IES) */
#define EVENT_TIES_US                 625

/* Ticker resolution margin
 * Needed due to the lack of fine timing resolution in ticker_start
 * and ticker_update. Set to 32 us, which is ~1 tick with 32768 Hz
 * clock.
 */
#define EVENT_TICKER_RES_MARGIN_US    32

#define EVENT_RX_JITTER_US(phy) 16    /* Radio Rx timing uncertainty */
#define EVENT_RX_TO_US(phy) ((((((phy)&0x03) + 4)<<3)/BIT((((phy)&0x3)>>1))) + \
				  EVENT_RX_JITTER_US(phy))

/* Turnaround time between RX and TX is based on CPU execution speed. It also
 * includes radio ramp up time. The value must meet hard deadline of `150 us`
 * imposed by BT Core spec for inter frame spacing (IFS). To include CPUs with
 * slow clock, the conservative approach was taken to use IFS value for all
 * cases.
 */
#define EVENT_RX_TX_TURNAROUND(phy)   150

/* Sub-microsecond conversion macros. With current timer resolution of ~30 us
 * per tick, conversion factor is 1, and macros map 1:1 between us_frac and us.
 * On sub-microsecond tick resolution architectures, a number of bits may be
 * used to represent fractions of a microsecond, to allow higher precision in
 * window widening.
 */
#define EVENT_US_TO_US_FRAC(us)             (us)
#define EVENT_US_FRAC_TO_US(us_frac)        (us_frac)
#define EVENT_TICKS_TO_US_FRAC(ticks)       HAL_TICKER_TICKS_TO_US(ticks)
#define EVENT_US_FRAC_TO_TICKS(us_frac)     HAL_TICKER_US_TO_TICKS(us_frac)
#define EVENT_US_FRAC_TO_REMAINDER(us_frac) HAL_TICKER_REMAINDER(us_frac)

/* Time needed to set up a CIS from ACL instant to prepare (incl. radio). Used
 * for CIS_Offset_Min.
 */
#define EVENT_OVERHEAD_CIS_SETUP_US         MAX(EVENT_OVERHEAD_START_US, 500U)
