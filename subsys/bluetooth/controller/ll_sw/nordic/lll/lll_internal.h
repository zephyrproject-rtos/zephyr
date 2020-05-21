/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_prepare_done(void *param);
int lll_done(void *param);
bool lll_is_done(void *param);
u32_t lll_evt_offset_get(struct evt_hdr *evt);
u32_t lll_preempt_calc(struct evt_hdr *evt, u8_t ticker_id,
		u32_t ticks_at_event);
void lll_chan_set(u32_t chan);
