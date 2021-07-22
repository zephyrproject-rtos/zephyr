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
#include "util/mem.h"
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
#include "lll_prof_internal.h"
#include "lll_df_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_adv_sync
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
#define ADV_SYNC_PDU_B2B_AFS  (CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK_AFS)
#endif

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_done(void *param);
#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
static void isr_tx(void *param);
static void pdu_b2b_update(struct lll_adv_sync *lll, struct pdu_adv *pdu);
static void pdu_b2b_aux_ptr_update(struct pdu_adv *pdu, uint8_t phy,
				   uint8_t flags, uint8_t chan_idx,
				   uint32_t tifs);
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK */

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
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, abort_cb, prepare_cb, 0, param);
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
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t start_us;
	void *extra_data;
	uint8_t phy_s;
	uint8_t upd;

	DEBUG_RADIO_START_A(1);

	lll = p->param;

	/* Calculate the current event latency */
	lll->latency_event = lll->latency_prepare + p->lazy;

	/* Calculate the current event counter value */
	event_counter = lll->event_counter + lll->latency_event;

	/* Update event counter to next value */
	lll->event_counter = (event_counter + 1);

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

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
	if (upd) {
		/* AuxPtr offsets for b2b TX are fixed for given chain so we can
		 * calculate them here in advance.
		 */
		pdu_b2b_update(lll, pdu);
	}
#endif

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

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
	if (pdu->adv_ext_ind.ext_hdr_len && pdu->adv_ext_ind.ext_hdr.aux_ptr) {
		lll->last_pdu = pdu;

		radio_isr_set(isr_tx, lll);
		radio_tmr_tifs_set(ADV_SYNC_PDU_B2B_AFS);
		radio_switch_complete_and_b2b_tx(phy_s, 0, phy_s, 0);
#else
	if (0) {
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK */
	} else {
		radio_isr_set(isr_done, lll);
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
		if (df_cfg) {
			radio_switch_complete_and_phy_end_disable();
		} else
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
		{
			radio_switch_complete_and_disable();
		}
	}

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

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
	if (lll_preempt_calc(ull, (TICKER_ID_ADV_SYNC_BASE +
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

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	struct lll_adv_sync *lll;
	int err;

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		/* Perform event abort here.
		 * After event has been cleanly aborted, clean up resources
		 * and dispatch event done.
		 */
		radio_isr_set(isr_done, param);
		radio_disable();
		return;
	}

	/* NOTE: Else clean the top half preparations of the aborted event
	 * currently in preparation pipeline.
	 */
	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	/* Accumulate the latency as event is aborted while being in pipeline */
	lll = prepare_param->param;
	lll->latency_prepare += (prepare_param->lazy + 1);

	lll_done(param);
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

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
static void isr_tx(void *param)
{
	struct lll_adv_sync *lll_sync;
	struct pdu_adv *pdu;
	struct lll_adv *lll;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Clear radio tx status and events */
	lll_isr_tx_status_reset();

	lll_sync = param;
	lll = lll_sync->adv;

	/* TODO: do not hardcode to single value */
	lll_chan_set(0);

	pdu = lll_adv_pdu_linked_next_get(lll_sync->last_pdu);
	LL_ASSERT(pdu);
	lll_sync->last_pdu = pdu;

	/* setup tIFS switching */
	if (pdu->adv_ext_ind.ext_hdr_len && pdu->adv_ext_ind.ext_hdr.aux_ptr) {
		radio_tmr_tifs_set(ADV_SYNC_PDU_B2B_AFS);
		radio_isr_set(isr_tx, lll_sync);
		radio_switch_complete_and_b2b_tx(lll->phy_s, 0, lll->phy_s, 0);
	} else {
		radio_isr_set(lll_isr_done, lll);
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
		if (lll_sync->cte_started) {
			radio_switch_complete_and_phy_end_disable();
		} else
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
		{
			radio_switch_complete_and_disable();
		}
	}

	radio_pkt_tx_set(pdu);

	/* assert if radio packet ptr is not set and radio started rx */
	LL_ASSERT(!radio_is_ready());

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

	/* capture end of AUX_SYNC_IND/AUX_CHAIN_IND PDU, used for calculating
	 * next PDU timestamp.
	 */
	radio_tmr_end_capture();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* PA/LNA enable is overwriting packet end used in ISR
		 * profiling, hence back it up for later use.
		 */
		lll_prof_radio_end_backup();
	}

	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
				 ADV_SYNC_PDU_B2B_AFS - 4 -
				 radio_tx_chain_delay_get(lll->phy_s, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_send();
	}
}

static void pdu_b2b_update(struct lll_adv_sync *lll, struct pdu_adv *pdu)
{
	while (pdu) {
		pdu_b2b_aux_ptr_update(pdu, lll->adv->phy_s, 0, 0,
				       ADV_SYNC_PDU_B2B_AFS);
		pdu = lll_adv_pdu_linked_next_get(pdu);
	}
}

static void pdu_b2b_aux_ptr_update(struct pdu_adv *pdu, uint8_t phy,
				   uint8_t flags, uint8_t chan_idx,
				   uint32_t tifs)
{
	struct pdu_adv_com_ext_adv *com_hdr;
	struct pdu_adv_ext_hdr *hdr;
	struct pdu_adv_aux_ptr *aux;
	uint32_t offs;
	uint8_t *dptr;

	com_hdr = &pdu->adv_ext_ind;
	hdr = &com_hdr->ext_hdr;
	/* Skip flags */
	dptr = hdr->data;

	if (!com_hdr->ext_hdr_len || !hdr->aux_ptr) {
		return;
	}

	LL_ASSERT(!hdr->adv_addr);
	LL_ASSERT(!hdr->tgt_addr);

	if (hdr->cte_info) {
		dptr++;
	}

	LL_ASSERT(!hdr->adi);

	/* Update AuxPtr */
	aux = (void *)dptr;
	offs = PKT_AC_US(pdu->len, phy) + tifs;
	offs = offs / OFFS_UNIT_30_US;
	if ((offs >> 13) != 0) {
		aux->offs = offs / (OFFS_UNIT_300_US / OFFS_UNIT_30_US);
		aux->offs_units = 1U;
	} else {
		aux->offs = offs;
		aux->offs_units = 0U;
	}
	aux->chan_idx = chan_idx;
	aux->ca = 0;
	aux->phy = find_lsb_set(phy) - 1;
}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK */
