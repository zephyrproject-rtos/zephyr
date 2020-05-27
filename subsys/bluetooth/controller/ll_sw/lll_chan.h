/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

uint8_t lll_chan_sel_1(uint8_t *chan_use, uint8_t hop, uint16_t latency, uint8_t *chan_map,
		    uint8_t chan_count);
uint8_t lll_chan_sel_2(uint16_t counter, uint16_t chan_id, uint8_t *chan_map,
		    uint8_t chan_count);
