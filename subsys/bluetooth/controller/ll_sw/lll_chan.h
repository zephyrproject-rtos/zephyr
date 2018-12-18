/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void lll_chan_set(u32_t chan);
u8_t lll_chan_sel_1(u8_t *chan_use, u8_t hop, u16_t latency, u8_t *chan_map,
		    u8_t chan_count);
u8_t lll_chan_sel_2(u16_t counter, u16_t chan_id, u8_t *chan_map,
		    u8_t chan_count);
