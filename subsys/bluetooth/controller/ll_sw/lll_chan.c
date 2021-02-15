/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "hal/ccm.h"
#include "hal/radio.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_chan
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static uint8_t chan_sel_remap(uint8_t *chan_map, uint8_t chan_index);
#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
static uint16_t chan_prn(uint16_t counter, uint16_t chan_id);
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

#if defined(CONFIG_BT_CONN)
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
uint8_t lll_chan_sel_2(uint16_t counter, uint16_t chan_id, uint8_t *chan_map,
		    uint8_t chan_count)
{
	uint8_t chan_next;
	uint16_t prn_e;

	prn_e = chan_prn(counter, chan_id);
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
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

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
#if defined(RADIO_UNIT_TEST)
void lll_chan_sel_2_ut(void)
{
	uint8_t chan_map_1[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
	uint8_t chan_map_2[] = {0x00, 0x06, 0xE0, 0x00, 0x1E};
	uint8_t m;

	m = chan_sel_2(1, 0x305F, chan_map_1, 37);
	LL_ASSERT(m == 20U);

	m = chan_sel_2(2, 0x305F, chan_map_1, 37);
	LL_ASSERT(m == 6U);

	m = chan_sel_2(3, 0x305F, chan_map_1, 37);
	LL_ASSERT(m == 21U);

	m = chan_sel_2(6, 0x305F, chan_map_2, 9);
	LL_ASSERT(m == 23U);

	m = chan_sel_2(7, 0x305F, chan_map_2, 9);
	LL_ASSERT(m == 9U);

	m = chan_sel_2(8, 0x305F, chan_map_2, 9);
	LL_ASSERT(m == 34U);
}
#endif /* RADIO_UNIT_TEST */

/* Attribution:
 * http://graphics.stanford.edu/%7Eseander/bithacks.html#ReverseByteWith32Bits
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

static uint16_t chan_prn(uint16_t counter, uint16_t chan_id)
{
	uint8_t iterate;
	uint16_t prn_e;

	prn_e = counter ^ chan_id;

	for (iterate = 0U; iterate < 3; iterate++) {
		prn_e = chan_perm(prn_e);
		prn_e = chan_mam(prn_e, chan_id);
	}

	prn_e ^= chan_id;

	return prn_e;
}
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */
