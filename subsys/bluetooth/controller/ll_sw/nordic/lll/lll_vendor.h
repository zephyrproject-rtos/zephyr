/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define EVENT_OVERHEAD_XTAL_US        1500
#define EVENT_OVERHEAD_PREEMPT_US     0    /* if <= min, then dynamic preempt */
#define EVENT_OVERHEAD_PREEMPT_MIN_US 0
#define EVENT_OVERHEAD_PREEMPT_MAX_US EVENT_OVERHEAD_XTAL_US
#define EVENT_OVERHEAD_START_US       300
/* Worst-case time margin needed after event end-time in the air
 * (done/preempt race margin + power-down/chain delay)
 */
#define EVENT_OVERHEAD_END_US         40
#define EVENT_JITTER_US               16

#define EVENT_RX_JITTER_US(phy) 16    /* Radio Rx timing uncertainty */
#define EVENT_RX_TO_US(phy) ((((((phy)&0x03) + 4)<<3)/BIT((((phy)&0x3)>>1))) + \
				  EVENT_RX_JITTER_US(phy))

/* TODO - fix up numbers re. HW */
#define EVENT_RX_TX_TURNARROUND(phy)  ((phy) == 1?100:((phy) == 2 ? 80:900))

