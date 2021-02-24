/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <soc.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"
#include "hal/radio_df.h"

#include "util/util.h"
#include "util/memq.h"

#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_adv_sync.h"
#include "lll_df_types.h"

#include "lll_internal.h"
#include "lll_adv_internal.h"
#include "lll_tim_internal.h"
#include "lll_df_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_adv_sync
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static void isr_done(void *param);

int lll_adv_sync_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_adv_sync_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_adv_sync_prepare(void *param)
{
	struct lll_prepare_param *p;
	struct lll_adv_sync *lll;
	uint16_t elapsed;
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	p = param;

	/* Instants elapsed */
	elapsed = p->lazy + 1;

	lll = p->param;

	/* Save the (latency + 1) for use in event */
	lll->latency_prepare += elapsed;

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, lll_abort_cb, prepare_cb, 0, p);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	struct lll_df_adv_cfg *df_cfg;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
	struct lll_adv_sync *lll;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint16_t event_counter;
	uint8_t data_chan_use;
	struct pdu_adv *pdu;
	struct evt_hdr *evt;
	uint32_t remainder;
	uint32_t start_us;
	void *extra_data;
	uint8_t phy_s;
	uint8_t upd;

	DEBUG_RADIO_START_A(1);

	lll = p->param;

	/* Deduce the latency */
	lll->latency_event = lll->latency_prepare - 1;

	/* Calculate the current event counter value */
	event_counter = lll->event_counter + lll->latency_event;

	/* Update event counter to next value */
	lll->event_counter = lll->event_counter + lll->latency_prepare;

	/* Reset accumulated latencies */
	lll->latency_prepare = 0;

	/* Calculate the radio channel to use */
	data_chan_use = lll_chan_sel_2(event_counter, lll->data_chan_id,
				       &lll->data_chan_map[0],
				       lll->data_chan_count);

	/* Start setting up of Radio h/w */
	radio_reset();
#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	phy_s = lll->adv->phy_s;

	/* TODO: if coded we use S8? */
	radio_phy_set(phy_s, 1);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (phy_s << 1));
	radio_aa_set(lll->access_addr);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    (((uint32_t)lll->crc_init[2] << 16) |
			     ((uint32_t)lll->crc_init[1] << 8) |
			     ((uint32_t)lll->crc_init[0])));
	lll_chan_set(data_chan_use);

	pdu = lll_adv_sync_data_latest_get(lll, &extra_data, &upd);
	LL_ASSERT(pdu);

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (extra_data) {
		df_cfg = (struct lll_df_adv_cfg *)extra_data;
		lll_df_conf_cte_tx_enable(df_cfg->cte_type, df_cfg->cte_length,
					  df_cfg->ant_sw_len, df_cfg->ant_ids);
		lll->cte_started = 1U;
	} else {
		df_cfg = NULL;
		lll->cte_started = 0U;
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	radio_pkt_tx_set(pdu);

	/* TODO: chaining */
	radio_isr_set(lll_isr_done, lll);
	radio_isr_set(isr_done, lll);

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (df_cfg) {
		radio_switch_complete_and_phy_end_disable();
	} else
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
	{
		radio_switch_complete_and_disable();
	}

	ticks_at_event = p->ticks_at_expire;
	evt = HDR_LLL2EVT(lll);
	ticks_at_event += lll_evt_offset_get(evt);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	start_us = radio_tmr_start(1, ticks_at_start, remainder);

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
	radio_gpio_pa_setup();

	radio_gpio_pa_lna_enable(start_us + radio_tx_ready_delay_get(phy_s, 1) -
				 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_PA_PIN */
	ARG_UNUSED(start_us);
#endif /* !CONFIG_BT_CTLR_GPIO_PA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	if (lll_preempt_calc(evt, (TICKER_ID_ADV_SYNC_BASE +
				   ull_adv_sync_lll_handle_get(lll)),
			     ticks_at_event)) {
		radio_isr_set(lll_isr_abort, lll);
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		uint32_t ret;

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	DEBUG_RADIO_START_A(1);

	return 0;
}

static void isr_done(void *param)
{
	struct lll_adv_sync *lll;

	lll = param;
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (lll->cte_started) {
		lll_df_conf_cte_tx_disable();
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
	lll_isr_done(lll);
}
