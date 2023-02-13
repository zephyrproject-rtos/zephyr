/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

uint8_t lll_chan_sel_1(uint8_t *chan_use, uint8_t hop, uint16_t latency, uint8_t *chan_map,
		    uint8_t chan_count);

uint16_t lll_chan_id(uint8_t *access_addr);
uint8_t lll_chan_sel_2(uint16_t counter, uint16_t chan_id, uint8_t *chan_map,
		    uint8_t chan_count);

uint8_t lll_chan_iso_event(uint16_t counter, uint16_t chan_id,
			   uint8_t *chan_map, uint8_t chan_count,
			   uint16_t *prn_s, uint16_t *remap_idx);
uint8_t lll_chan_iso_subevent(uint16_t chan_id, uint8_t *chan_map,
			      uint8_t chan_count, uint16_t *prn_subevent_lu,
			      uint16_t *remap_idx);

void lll_chan_sel_2_ut(void);
