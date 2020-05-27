/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_prepare_done(void *param);
int lll_done(void *param);
bool lll_is_done(void *param);
int lll_clk_on(void);
int lll_clk_on_wait(void);
int lll_clk_off(void);
uint32_t lll_evt_offset_get(struct evt_hdr *evt);
uint32_t lll_preempt_calc(struct evt_hdr *evt, uint8_t ticker_id,
		uint32_t ticks_at_event);
void lll_chan_set(uint32_t chan);
uint8_t lll_entropy_get(uint8_t len, void *rand);
