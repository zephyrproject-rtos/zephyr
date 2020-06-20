/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_prepare(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
		lll_prepare_cb_t prepare_cb, int prio,
		struct lll_prepare_param *prepare_param);
int lll_prepare_done(void *param);
int lll_done(void *param);
bool lll_is_done(void *param);
int lll_is_abort_cb(void *next, int prio, void *curr,
			 lll_prepare_cb_t *resume_cb, int *resume_prio);

static inline int lll_is_stop(void *lll)
{
	struct lll_hdr *hdr = lll;

	return !!hdr->is_stop;
}

int lll_clk_on(void);
int lll_clk_on_wait(void);
int lll_clk_off(void);
uint32_t lll_evt_offset_get(struct evt_hdr *evt);
uint32_t lll_preempt_calc(struct evt_hdr *evt, uint8_t ticker_id,
		uint32_t ticks_at_event);
void lll_chan_set(uint32_t chan);
