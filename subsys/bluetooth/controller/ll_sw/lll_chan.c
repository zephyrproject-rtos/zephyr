/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

#include "hal/ccm.h"
#include "hal/radio.h"

#include <soc.h>
#include "hal/debug.h"

static uint8_t chan_sel_remap(uint8_t *chan_map, uint8_t chan_index);
#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
static uint16_t chan_prn_s(uint16_t counter, uint16_t chan_id);
static uint16_t chan_prn_e(uint16_t counter, uint16_t chan_id);

#if defined(CONFIG_BT_CTLR_ISO)
static uint8_t chan_sel_remap_index(uint8_t *chan_map, uint8_t chan_index);
static uint16_t chan_prn_subevent_se(uint16_t chan_id,
				     uint16_t *prn_subevent_lu);
static uint8_t chan_d(uint8_t n);
#endif /* CONFIG_BT_CTLR_ISO */
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

#if defined(CONFIG_BT_CONN)
/* Refer to Bluetooth Specification v5.2 Vol 6, Part B, Section 4.5.8.2
 * Channel Selection algorithm #1
 */
uint8_t lll_chan_sel_1(uint8_t *chan_use, uint8_t hop, uint16_t latency, uint8_t *chan_map,
		    uint8_t chan_count)
{
	uint8_t chan_next;

	chan_next = ((*chan_use) + (hop * (1 + latency))) % 37;
	*chan_use = chan_next;

	if ((chan_map[chan_next >> 3] & (1 << (chan_next % 8))) == 0U) {
		uint8_t chan_index;

		chan_index = chan_next % chan_count;
		chan_next = chan_sel_remap(chan_map, chan_index);

	} else {
		/* channel can be used, return it */
	}

	return chan_next;
}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
/* Refer to Bluetooth Specification v5.2 Vol 6, Part B, Section 4.5.8.3.2
 * Inputs and basic components
 */
uint16_t lll_chan_id(uint8_t *access_addr)
{
	uint16_t aa_ls = ((uint16_t)access_addr[1] << 8) | access_addr[0];
	uint16_t aa_ms = ((uint16_t)access_addr[3] << 8) | access_addr[2];

	return aa_ms ^ aa_ls;
}

/* Refer to Bluetooth Specification v5.2 Vol 6, Part B, Section 4.5.8.3
 * Channel Selection algorithm #2, and Section 4.5.8.3.1 Overview
 * Below interface is used for ACL connections.
 */
uint8_t lll_chan_sel_2(uint16_t counter, uint16_t chan_id, uint8_t *chan_map,
		    uint8_t chan_count)
{
	uint8_t chan_next;
	uint16_t prn_e;

	prn_e = chan_prn_e(counter, chan_id);
	chan_next = prn_e % 37;

	if ((chan_map[chan_next >> 3] & (1 << (chan_next % 8))) == 0U) {
		uint8_t chan_index;

		chan_index = ((uint32_t)chan_count * prn_e) >> 16;
		chan_next = chan_sel_remap(chan_map, chan_index);

	} else {
		/* channel can be used, return it */
	}

	return chan_next;
}

#if defined(CONFIG_BT_CTLR_ISO)
/* Refer to Bluetooth Specification v5.2 Vol 6, Part B, Section 4.5.8.3
 * Channel Selection algorithm #2, and Section 4.5.8.3.1 Overview
 *
 * Below interface is used for ISO first subevent.
 */
uint8_t lll_chan_iso_event(uint16_t counter, uint16_t chan_id,
			   uint8_t *chan_map, uint8_t chan_count,
			   uint16_t *prn_s, uint16_t *remap_idx)
{
	uint8_t chan_idx;
	uint16_t prn_e;

	*prn_s = chan_prn_s(counter, chan_id);
	prn_e = *prn_s ^ chan_id;
	chan_idx = prn_e % 37;

	if ((chan_map[chan_idx >> 3] & (1 << (chan_idx % 8))) == 0U) {
		*remap_idx = ((uint32_t)chan_count * prn_e) >> 16;
		chan_idx = chan_sel_remap(chan_map, *remap_idx);

	} else {
		*remap_idx = chan_sel_remap_index(chan_map, chan_idx);
	}

	return chan_idx;
}

/* Refer to Bluetooth Specification v5.2 Vol 6, Part B, Section 4.5.8.3
 * Channel Selection algorithm #2, and Section 4.5.8.3.1 Overview
 *
 * Below interface is used for ISO next subevent.
 */
uint8_t lll_chan_iso_subevent(uint16_t chan_id, uint8_t *chan_map,
			      uint8_t chan_count, uint16_t *prn_subevent_lu,
			      uint16_t *remap_idx)
{
	uint16_t prn_subevent_se;
	uint8_t d;
	uint8_t x;

	prn_subevent_se = chan_prn_subevent_se(chan_id, prn_subevent_lu);

	d = chan_d(chan_count);

	/* Sub-expression to get natural number (N - 2d + 1) to be used in the
	 * calculation of d.
	 */
	if ((chan_count + 1) > (d << 1)) {
		x = (chan_count + 1) - (d << 1);
	} else {
		x = 0;
	}

	*remap_idx = ((((uint32_t)prn_subevent_se * x) >> 16) +
		      d + *remap_idx) % chan_count;

	return chan_sel_remap(chan_map, *remap_idx);
}
#endif /* CONFIG_BT_CTLR_ISO */
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

/* Refer to Bluetooth Specification v5.2 Vol 6, Part B, Section 4.5.8.3
 * Channel Selection algorithm #2, and Section 4.5.8.3.4 Event mapping to used
 * channel index
 */
static uint8_t chan_sel_remap(uint8_t *chan_map, uint8_t chan_index)
{
	uint8_t chan_next;
	uint8_t byte_count;

	chan_next = 0U;
	byte_count = 5U;
	while (byte_count--) {
		uint8_t bite;
		uint8_t bit_count;

		bite = *chan_map;
		bit_count = 8U;
		while (bit_count--) {
			if (bite & 0x01) {
				if (chan_index == 0U) {
					break;
				}
				chan_index--;
			}
			chan_next++;
			bite >>= 1;
		}

		if (bit_count < 8) {
			break;
		}

		chan_map++;
	}

	return chan_next;
}

#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
/* Attribution:
 * http://graphics.stanford.edu/%7Eseander/bithacks.html#ReverseByteWith32Bits
 */
/* Refer to Bluetooth Specification v5.2 Vol 6, Part B, Section 4.5.8.3.2
 * Inputs and basic components, for below operations
 */
static uint8_t chan_rev_8(uint8_t b)
{
	b = (((uint32_t)b * 0x0802LU & 0x22110LU) |
	     ((uint32_t)b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;

	return b;
}

static uint16_t chan_perm(uint16_t i)
{
	return (chan_rev_8((i >> 8) & 0xFF) << 8) | chan_rev_8(i & 0xFF);
}

static uint16_t chan_mam(uint16_t a, uint16_t b)
{
	return ((uint32_t)a * 17U + b) & 0xFFFF;
}

static uint16_t chan_prn_s(uint16_t counter, uint16_t chan_id)
{
	uint8_t iterate;
	uint16_t prn_s;

	prn_s = counter ^ chan_id;

	for (iterate = 0U; iterate < 3; iterate++) {
		prn_s = chan_perm(prn_s);
		prn_s = chan_mam(prn_s, chan_id);
	}

	return prn_s;
}

static uint16_t chan_prn_e(uint16_t counter, uint16_t chan_id)
{
	uint16_t prn_e;

	prn_e = chan_prn_s(counter, chan_id);
	prn_e ^= chan_id;

	return prn_e;
}

#if defined(CONFIG_BT_CTLR_ISO)
/* Refer to Bluetooth Specification v5.2 Vol 6, Part B, Section 4.5.8.3
 * Channel Selection algorithm #2, and Section 4.5.8.3.4 Event mapping to used
 * channel index
 *
 * Below function is used in the context of next subevent, return remapping
 * index.
 */
static uint8_t chan_sel_remap_index(uint8_t *chan_map, uint8_t chan_index)
{
	uint8_t octet_count;
	uint8_t remap_index;

	remap_index = 0U;
	octet_count = 5U;
	while (octet_count--) {
		uint8_t octet;
		uint8_t bit_count;

		octet = *chan_map;
		bit_count = 8U;
		while (bit_count--) {
			if (!chan_index) {
				return remap_index;
			}
			chan_index--;

			if (octet & BIT(0)) {
				remap_index++;
			}
			octet >>= 1;
		}

		chan_map++;
	}

	return 0;
}

/* Refer to Bluetooth Specification v5.2 Vol 6, Part B, Section 4.5.8.3
 * Channel Selection algorithm #2, and Section 4.5.8.3.5 Subevent pseudo-random
 * number generation
 */
static uint16_t chan_prn_subevent_se(uint16_t chan_id,
				     uint16_t *prn_subevent_lu)
{
	uint16_t prn_subevent_se;
	uint16_t lu;

	lu = *prn_subevent_lu;
	lu = chan_perm(lu);
	lu = chan_mam(lu, chan_id);

	*prn_subevent_lu = lu;

	prn_subevent_se = lu ^ chan_id;

	return prn_subevent_se;
}

/* Refer to Bluetooth Specification v5.2 Vol 6, Part B, Section 4.5.8.3
 * Channel Selection algorithm #2, and Section 4.5.8.3.6 Subevent mapping to
 * used channel index
 */
static uint8_t chan_d(uint8_t n)
{
	uint8_t x, y;

	/* Sub-expression to get natural number (N - 5) to be used in the
	 * calculation of d.
	 */
	if (n > 5) {
		x = n - 5;
	} else {
		x = 0;
	}

	/* Sub-expression to get natural number ((N - 10) / 2) to be used in the
	 * calculation of d.
	 */
	if (n > 10) {
		y = (n - 10) >> 1;
	} else {
		y = 0;
	}

	/* Calculate d using the above sub expressions */
	return MAX(1, MAX(MIN(3, x), MIN(11, y)));
}
#endif /* CONFIG_BT_CTLR_ISO */

#if defined(CONFIG_BT_CTLR_TEST)
/* Refer to Bluetooth Specification v5.2 Vol 6, Part C, Section 3 LE Channel
 * Selection algorithm #2 sample data
 */
void lll_chan_sel_2_ut(void)
{
	uint8_t chan_map_1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
	uint8_t const chan_map_1_37_used = 37U;
	uint8_t chan_map_2[] = {0x00, 0x06, 0xE0, 0x00, 0x1E};
	uint8_t const chan_map_2_9_used = 9U;
	uint8_t chan_map_3[] = {0x06, 0x00, 0x00, 0x00, 0x00};
	uint8_t const chan_map_3_2_used = 2U;
	uint16_t const chan_id = 0x305F;
	uint8_t m;

	/* Tests when ISO not supported */
	/* Section 3.1 Sample Data 1 (37 used channels) */
	m = lll_chan_sel_2(0U, chan_id, chan_map_1, chan_map_1_37_used);
	LL_ASSERT(m == 25U);

	m = lll_chan_sel_2(1U, chan_id, chan_map_1, chan_map_1_37_used);
	LL_ASSERT(m == 20U);

	m = lll_chan_sel_2(2U, chan_id, chan_map_1, chan_map_1_37_used);
	LL_ASSERT(m == 6U);

	m = lll_chan_sel_2(3U, chan_id, chan_map_1, chan_map_1_37_used);
	LL_ASSERT(m == 21U);

	/* Section 3.2 Sample Data 2 (9 used channels) */
	m = lll_chan_sel_2(6U, chan_id, chan_map_2, chan_map_2_9_used);
	LL_ASSERT(m == 23U);

	m = lll_chan_sel_2(7U, chan_id, chan_map_2, chan_map_2_9_used);
	LL_ASSERT(m == 9U);

	m = lll_chan_sel_2(8U, chan_id, chan_map_2, chan_map_2_9_used);
	LL_ASSERT(m == 34U);

	/* FIXME: Use Sample Data from Spec, if one is added.
	 *        Below is a random sample to cover implementation in this file.
	 */
	/* Section x.x Sample Data 3 (2 used channels) */
	m = lll_chan_sel_2(11U, chan_id, chan_map_3, chan_map_3_2_used);
	LL_ASSERT(m == 1U);

	m = lll_chan_sel_2(12U, chan_id, chan_map_3, chan_map_3_2_used);
	LL_ASSERT(m == 2U);

	m = lll_chan_sel_2(13U, chan_id, chan_map_3, chan_map_3_2_used);
	LL_ASSERT(m == 1U);

#if defined(CONFIG_BT_CTLR_ISO)
	uint16_t prn_subevent_lu;
	uint16_t prn_subevent_se;
	uint16_t remap_idx;
	uint16_t prn_s;

	/* BIS subevent 2, event counter 0, test prnSubEvent_se */
	prn_s = 56857U ^ chan_id;
	prn_subevent_lu = prn_s;
	prn_subevent_se = chan_prn_subevent_se(chan_id, &prn_subevent_lu);
	LL_ASSERT(prn_subevent_se == 11710U);

	/* BIS subevent 3, event counter 0 */
	prn_subevent_se = chan_prn_subevent_se(chan_id, &prn_subevent_lu);
	LL_ASSERT(prn_subevent_se == 16649U);

	/* BIS subevent 4, event counter 0 */
	prn_subevent_se = chan_prn_subevent_se(chan_id, &prn_subevent_lu);
	LL_ASSERT(prn_subevent_se == 38198U);

	/* Section 3.1 Sample Data 1 (37 used channels) */
	/* BIS subevent 1, event counter 0 */
	m = lll_chan_iso_event(0U, chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT((prn_s ^ chan_id) == 56857U);
	LL_ASSERT(m == 25U);
	LL_ASSERT(remap_idx == 25U);

	/* BIS subvent 2 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 1U);
	LL_ASSERT(m == 1U);

	/* BIS subvent 3 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 16U);
	LL_ASSERT(m == 16U);

	/* BIS subvent 4 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 36U);
	LL_ASSERT(m == 36U);

	/* BIS subevent 1, event counter 1 */
	m = lll_chan_iso_event(1U, chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT((prn_s ^ chan_id) == 1685U);
	LL_ASSERT(m == 20U);
	LL_ASSERT(remap_idx == 20U);

	/* BIS subvent 2 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 36U);
	LL_ASSERT(m == 36U);

	/* BIS subvent 3 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 12U);
	LL_ASSERT(m == 12U);

	/* BIS subvent 4 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 34U);
	LL_ASSERT(m == 34U);

	/* BIS subevent 1, event counter 2 */
	m = lll_chan_iso_event(2U, chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT((prn_s ^ chan_id) == 38301U);
	LL_ASSERT(m == 6U);
	LL_ASSERT(remap_idx == 6U);

	/* BIS subvent 2 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 18U);
	LL_ASSERT(m == 18U);

	/* BIS subvent 3 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 32U);
	LL_ASSERT(m == 32U);

	/* BIS subvent 4 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 21U);
	LL_ASSERT(m == 21U);

	/* BIS subevent 1, event counter 3 */
	m = lll_chan_iso_event(3U, chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT((prn_s ^ chan_id) == 27475U);
	LL_ASSERT(m == 21U);
	LL_ASSERT(remap_idx == 21U);

	/* BIS subvent 2 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 4U);
	LL_ASSERT(m == 4U);

	/* BIS subvent 3 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 22U);
	LL_ASSERT(m == 22U);

	/* BIS subvent 4 */
	m = lll_chan_iso_subevent(chan_id, chan_map_1, chan_map_1_37_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 8U);
	LL_ASSERT(m == 8U);

	/* Section 3.2 Sample Data 2 (9 used channels) */
	/* BIS subevent 1, event counter 6 */
	m = lll_chan_iso_event(6U, chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT((prn_s ^ chan_id) == 10975U);
	LL_ASSERT(remap_idx == 4U);
	LL_ASSERT(m == 23U);

	/* BIS subvent 2 */
	m = lll_chan_iso_subevent(chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 7U);
	LL_ASSERT(m == 35U);

	/* BIS subvent 3 */
	m = lll_chan_iso_subevent(chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 2U);
	LL_ASSERT(m == 21U);

	/* BIS subvent 4 */
	m = lll_chan_iso_subevent(chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 8U);
	LL_ASSERT(m == 36U);

	/* BIS subevent 1, event counter 7 */
	m = lll_chan_iso_event(7U, chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT((prn_s ^ chan_id) == 5490U);
	LL_ASSERT(remap_idx == 0U);
	LL_ASSERT(m == 9U);

	/* BIS subvent 2 */
	m = lll_chan_iso_subevent(chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 3U);
	LL_ASSERT(m == 22U);

	/* BIS subvent 3 */
	m = lll_chan_iso_subevent(chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 8U);
	LL_ASSERT(m == 36U);

	/* BIS subvent 4 */
	m = lll_chan_iso_subevent(chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 5U);
	LL_ASSERT(m == 33U);

	/* BIS subevent 1, event counter 8 */
	m = lll_chan_iso_event(8U, chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT((prn_s ^ chan_id) == 46970U);
	LL_ASSERT(remap_idx == 6U);
	LL_ASSERT(m == 34U);

	/* BIS subvent 2 */
	m = lll_chan_iso_subevent(chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 0U);
	LL_ASSERT(m == 9U);

	/* BIS subvent 3 */
	m = lll_chan_iso_subevent(chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 5U);
	LL_ASSERT(m == 33U);

	/* BIS subvent 4 */
	m = lll_chan_iso_subevent(chan_id, chan_map_2, chan_map_2_9_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 1U);
	LL_ASSERT(m == 10U);

	/* FIXME: Use Sample Data from Spec, if one is added.
	 *        Below is a random sample to cover implementation in this file.
	 */
	/* Section x.x Sample Data 3 (2 used channels) */
	/* BIS subevent 1, event counter 11 */
	m = lll_chan_iso_event(11U, chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT((prn_s ^ chan_id) == 8628U);
	LL_ASSERT(remap_idx == 0U);
	LL_ASSERT(m == 1U);

	/* BIS subvent 2 */
	m = lll_chan_iso_subevent(chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 1U);
	LL_ASSERT(m == 2U);

	/* BIS subvent 3 */
	m = lll_chan_iso_subevent(chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 0U);
	LL_ASSERT(m == 1U);

	/* BIS subvent 4 */
	m = lll_chan_iso_subevent(chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 1U);
	LL_ASSERT(m == 2U);

	/* BIS subevent 1, event counter 12 */
	m = lll_chan_iso_event(12U, chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT((prn_s ^ chan_id) == 34748U);
	LL_ASSERT(remap_idx == 1U);
	LL_ASSERT(m == 2U);

	/* BIS subvent 2 */
	m = lll_chan_iso_subevent(chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 0U);
	LL_ASSERT(m == 1U);

	/* BIS subvent 3 */
	m = lll_chan_iso_subevent(chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 1U);
	LL_ASSERT(m == 2U);

	/* BIS subvent 4 */
	m = lll_chan_iso_subevent(chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 0U);
	LL_ASSERT(m == 1U);

	/* BIS subevent 1, event counter 13 */
	m = lll_chan_iso_event(13U, chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT((prn_s ^ chan_id) == 22072U);
	LL_ASSERT(remap_idx == 0U);
	LL_ASSERT(m == 1U);

	/* BIS subvent 2 */
	m = lll_chan_iso_subevent(chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 1U);
	LL_ASSERT(m == 2U);

	/* BIS subvent 3 */
	m = lll_chan_iso_subevent(chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 0U);
	LL_ASSERT(m == 1U);

	/* BIS subvent 4 */
	m = lll_chan_iso_subevent(chan_id, chan_map_3, chan_map_3_2_used, &prn_s, &remap_idx);
	LL_ASSERT(remap_idx == 1U);
	LL_ASSERT(m == 2U);

#endif /* CONFIG_BT_CTLR_ISO */
}
#endif /* CONFIG_BT_CTLR_TEST */
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */
