/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_CHAN_METRICS_EVENT)
#define LLL_CHAN_METRICS
#else /* !CONFIG_BT_CTLR_CHAN_METRICS_EVENT */
#define LLL_CHAN_METRICS static __attribute__((always_inline)) inline
#endif /* !CONFIG_BT_CTLR_CHAN_METRICS_EVENT */

uint8_t lll_chan_sel_1(uint8_t *chan_use, uint8_t hop, uint16_t latency,
		       uint8_t *chan_map, uint8_t chan_count);

uint16_t lll_chan_id(uint8_t *access_addr);
uint8_t lll_chan_sel_2(uint16_t counter, uint16_t chan_id, uint8_t *chan_map,
		       uint8_t chan_count);

uint8_t lll_chan_iso_event(uint16_t counter, uint16_t chan_id,
			   const uint8_t *chan_map, uint8_t chan_count,
			   uint16_t *prn_s, uint16_t *remap_idx);
uint8_t lll_chan_iso_subevent(uint16_t chan_id, const uint8_t *chan_map,
			      uint8_t chan_count, uint16_t *prn_subevent_lu,
			      uint16_t *remap_idx);

LLL_CHAN_METRICS void lll_chan_metrics_init(void);
LLL_CHAN_METRICS void lll_chan_metrics_chan_bad(uint8_t chan_idx);
LLL_CHAN_METRICS void lll_chan_metrics_chan_good(uint8_t chan_idx);
LLL_CHAN_METRICS bool lll_chan_metrics_is_notify(void);
LLL_CHAN_METRICS bool lll_chan_metrics_notify_clear(void);
LLL_CHAN_METRICS void lll_chan_metrics_print(void);

void lll_chan_sel_2_ut(void);

#if !defined(CONFIG_BT_CTLR_CHAN_METRICS_EVENT)
LLL_CHAN_METRICS void lll_chan_metrics_init(void)
{
}

LLL_CHAN_METRICS void lll_chan_metrics_chan_bad(uint8_t chan_idx)
{
}

LLL_CHAN_METRICS void lll_chan_metrics_chan_good(uint8_t chan_idx)
{
}

LLL_CHAN_METRICS bool lll_chan_metrics_is_notify(void)
{
	return false;
}

LLL_CHAN_METRICS bool lll_chan_metrics_notify_clear(void)
{
	return false;
}

LLL_CHAN_METRICS void lll_chan_metrics_print(void)
{
}
#endif /* !CONFIG_BT_CTLR_CHAN_METRICS_EVENT */
