/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <soc.h>

#include <zephyr/sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"
#include "hal/radio_df.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_adv_sync.h"
#include "lll_adv_iso.h"
#include "lll_df_types.h"

#include "lll_internal.h"
#include "lll_adv_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"
#include "lll_df_internal.h"

#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_done(void *param);

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
static void isr_tx(void *param);
static int aux_ptr_get(struct pdu_adv *pdu, struct pdu_adv_aux_ptr **aux_ptr);
static void chain_pdu_aux_ptr_chan_idx_set(struct lll_adv_sync *lll);
static void aux_ptr_chan_idx_set(struct lll_adv_sync *lll, struct pdu_adv *pdu);
static void switch_radio_complete_and_b2b_tx(const struct lll_adv_sync *lll, uint8_t phy_s);
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

static bool is_instant_or_past(uint16_t event_counter, uint16_t instant)
{
	uint16_t instant_latency;

	instant_latency = (event_counter - instant) &
			  EVENT_INSTANT_MAX;

	return instant_latency <= EVENT_INSTANT_LATENCY_MAX;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	struct lll_adv_sync *lll;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint8_t data_chan_count;
	uint8_t *data_chan_map;
	uint16_t event_counter;
	uint8_t data_chan_use;
	struct pdu_adv *pdu;
	struct ull_hdr *ull;
	uint32_t cte_len_us;
	uint32_t remainder;
	uint32_t start_us;
	uint8_t phy_s;
	uint32_t ret;
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

	/* Process channel map update, if any */
	if ((lll->chm_first != lll->chm_last) &&
	    is_instant_or_past(event_counter, lll->chm_instant)) {
		/* At or past the instant, use channelMapNew */
		lll->chm_first = lll->chm_last;
	}

	/* Calculate the radio channel to use */
	data_chan_map = lll->chm[lll->chm_first].data_chan_map;
	data_chan_count = lll->chm[lll->chm_first].data_chan_count;
	data_chan_use = lll_chan_sel_2(event_counter, lll->data_chan_id,
				       data_chan_map, data_chan_count);

	/* Start setting up of Radio h/w */
	radio_reset();
#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->adv->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	phy_s = lll->adv->phy_s;

	/* TODO: if coded we use S8? */
	radio_phy_set(phy_s, lll->adv->phy_flags);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, PDU_AC_PAYLOAD_SIZE_MAX,
			    RADIO_PKT_CONF_PHY(phy_s));
	radio_aa_set(lll->access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL,
				sys_get_le24(lll->crc_init));
	lll_chan_set(data_chan_use);

	upd = 0U;
	pdu = lll_adv_sync_data_latest_get(lll, NULL, &upd);
	LL_ASSERT(pdu);

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	lll_df_cte_tx_enable(lll, pdu, &cte_len_us);
#else
	cte_len_us = 0U;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX) */

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
	if (pdu->adv_ext_ind.ext_hdr_len && pdu->adv_ext_ind.ext_hdr.aux_ptr) {
		/* Set the last used auxiliary PDU for transmission */
		lll->last_pdu = pdu;

		/* Populate chan idx for AUX_ADV_IND PDU */
		aux_ptr_chan_idx_set(lll, pdu);

		radio_isr_set(isr_tx, lll);
		radio_tmr_tifs_set(EVENT_SYNC_B2B_MAFS_US);
		switch_radio_complete_and_b2b_tx(lll, phy_s);
	} else {
		/* No chain PDU */
		lll->last_pdu = NULL;

#else /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK */
	{
#endif /* !CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK */

		radio_isr_set(isr_done, lll);
		radio_switch_complete_and_disable();
	}

#if defined(CONFIG_BT_CTLR_ADV_ISO) && defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	if (lll->iso) {
		ull_adv_iso_lll_biginfo_fill(pdu, lll);
	}
#endif /* CONFIG_BT_CTLR_ADV_ISO && CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

	/* Set the Radio Tx Packet */
	radio_pkt_tx_set(pdu);

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	start_us = radio_tmr_start(1, ticks_at_start, remainder);

#if defined(CONFIG_BT_CTLR_PROFILE_ISR) || \
	defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	/* capture end of AUX_SYNC_IND/AUX_CHAIN_IND PDU, used for calculating
	 * next PDU timestamp.
	 *
	 * In Periodic Advertising without chaining there is no need for LLL to
	 * get the end time from radio, hence there is no call to
	 * radio_tmr_end_capture() to capture the radio end time.
	 *
	 * With chaining the sw_switch used PPI/DPPI for back to back Tx, no
	 * radio end time capture is needed there either.
	 *
	 * For PA LNA (and ISR profiling), the radio end time is required to
	 * setup the GPIOTE using radio_gpio_pa_lna_enable which needs call to
	 * radio_tmr_tifs_base_get(), both PA/LNA and ISR profiling call
	 * radio_tmr_end_get().
	 */
	radio_tmr_end_capture();
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_setup();

	radio_gpio_pa_lna_enable(start_us + radio_tx_ready_delay_get(phy_s, 1) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
	ARG_UNUSED(start_us);
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	uint32_t overhead;

	overhead = lll_preempt_calc(ull, (TICKER_ID_ADV_SYNC_BASE +
					  ull_adv_sync_lll_handle_get(lll)), ticks_at_event);
	/* check if preempt to start has changed */
	if (overhead) {
		LL_ASSERT_OVERHEAD(overhead);

		radio_isr_set(lll_isr_abort, lll);
		radio_disable();

		return -ECANCELED;
	}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
	/* Populate chan idx for AUX_CHAIN_IND PDU */
	chain_pdu_aux_ptr_chan_idx_set(lll);
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK */

	ret = lll_prepare_done(lll);
	LL_ASSERT(!ret);

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
	struct lll_adv_sync *lll = param;

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (lll->cte_started) {
		lll_df_cte_tx_disable();
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* Signal thread mode to remove Channel Map Update Indication in the
	 * ACAD.
	 */
	if ((lll->chm_first != lll->chm_last) &&
	    is_instant_or_past(lll->event_counter, lll->chm_instant)) {
		struct node_rx_pdu *rx;

		/* Allocate, prepare and dispatch Channel Map Update
		 * complete message towards ULL, then subsequently to
		 * the thread context.
		 */
		rx = ull_pdu_rx_alloc();
		LL_ASSERT(rx);

		rx->hdr.type = NODE_RX_TYPE_SYNC_CHM_COMPLETE;
		rx->rx_ftr.param = lll;

		ull_rx_put_sched(rx->hdr.link, rx);
	}

	lll_isr_done(lll);
}

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
static void isr_tx(void *param)
{
	struct pdu_adv_aux_ptr *aux_ptr;
	struct lll_adv_sync *lll_sync;
	struct pdu_adv *pdu;
	struct lll_adv *lll;
	uint32_t cte_len_us;
	int err;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Clear radio tx status and events */
	lll_isr_tx_status_reset();

	/* Get reference to sync and primary advertising LLL contexts */
	lll_sync = param;
	lll = lll_sync->adv;

	/* Get reference to aux pointer structure */
	err = aux_ptr_get(lll_sync->last_pdu, &aux_ptr);
	LL_ASSERT(!err && aux_ptr);

	/* Use channel idx that was in aux_ptr */
	lll_chan_set(aux_ptr->chan_idx);

	/* Get reference to the auxiliary chain PDU */
	pdu = lll_adv_pdu_linked_next_get(lll_sync->last_pdu);
	LL_ASSERT(pdu);

	/* Set the last used auxiliary PDU for transmission */
	lll_sync->last_pdu = pdu;

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	lll_df_cte_tx_enable(lll_sync, pdu, &cte_len_us);
#else
	cte_len_us = 0;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* setup tIFS switching */
	if (pdu->adv_ext_ind.ext_hdr_len && pdu->adv_ext_ind.ext_hdr.aux_ptr) {
		radio_tmr_tifs_set(EVENT_SYNC_B2B_MAFS_US);
		radio_isr_set(isr_tx, lll_sync);
		switch_radio_complete_and_b2b_tx(lll_sync, lll->phy_s);
	} else {
		radio_isr_set(isr_done, lll_sync);
		radio_switch_complete_and_b2b_tx_disable();
	}

	radio_pkt_tx_set(pdu);

	/* assert if radio packet ptr is not set and radio started rx */
	LL_ASSERT(!radio_is_ready());

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

#if defined(CONFIG_BT_CTLR_PROFILE_ISR) || \
	defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	/* capture end of AUX_SYNC_IND/AUX_CHAIN_IND PDU, used for calculating
	 * next PDU timestamp.
	 */
	radio_tmr_end_capture();
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* PA/LNA enable is overwriting packet end used in ISR
		 * profiling, hence back it up for later use.
		 */
		lll_prof_radio_end_backup();
	}

	radio_gpio_pa_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
				 EVENT_SYNC_B2B_MAFS_US -
				 (EVENT_CLOCK_JITTER_US << 1) + cte_len_us -
				 radio_tx_chain_delay_get(lll->phy_s, 0) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

	/* Populate chan idx for AUX_CHAIN_IND PDU */
	chain_pdu_aux_ptr_chan_idx_set(lll_sync);

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_send();
	}
}

static int aux_ptr_get(struct pdu_adv *pdu, struct pdu_adv_aux_ptr **aux_ptr)
{
	struct pdu_adv_com_ext_adv *com_hdr;
	struct pdu_adv_ext_hdr *hdr;
	uint8_t *dptr;

	/* Get reference to common extended header */
	com_hdr = (void *)&pdu->adv_ext_ind;
	if (com_hdr->ext_hdr_len == 0U) {
		return -EINVAL;
	}

	/* Get reference to extended header flags and header fields */
	hdr = (void *)com_hdr->ext_hdr_adv_data;
	dptr = hdr->data;

	/* No traverse through of AdvA and TargetA.
	 * These are RFU for periodic advertising, is not set by local device.
	 */

	/* traverse through CTEInfo flag, if present */
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (hdr->cte_info) {
		dptr += sizeof(struct pdu_cte_info);
	}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

	/* traverse through adi, if present */
	if (hdr->adi) {
		dptr += sizeof(struct pdu_adv_adi);
	}

	/* check for aux_ptr flag */
	if (hdr->aux_ptr) {
		/* Return reference to aux pointer structure */
		*aux_ptr = (void *)dptr;
	} else {
		*aux_ptr = NULL;
	}

	return 0;
}

static void chain_pdu_aux_ptr_chan_idx_set(struct lll_adv_sync *lll)
{
	struct pdu_adv *chain_pdu;

	/* No chain PDU */
	if (!lll->last_pdu) {
		return;
	}

	/* Get reference to the auxiliary chain PDU */
	chain_pdu = lll_adv_pdu_linked_next_get(lll->last_pdu);

	/* Check if there is further chain PDU */
	if (chain_pdu && chain_pdu->adv_ext_ind.ext_hdr_len &&
	    chain_pdu->adv_ext_ind.ext_hdr.aux_ptr) {
		aux_ptr_chan_idx_set(lll, chain_pdu);
	}
}

static void aux_ptr_chan_idx_set(struct lll_adv_sync *lll, struct pdu_adv *pdu)
{
	struct pdu_adv_aux_ptr *aux_ptr;
	uint8_t chan_idx;
	int err;

	/* Get reference to aux pointer structure */
	err = aux_ptr_get(pdu, &aux_ptr);
	LL_ASSERT(!err && aux_ptr);

	/* Calculate a new channel index */
	chan_idx = lll_chan_sel_2(lll->data_chan_counter, lll->data_chan_id,
				  lll->chm[lll->chm_first].data_chan_map,
				  lll->chm[lll->chm_first].data_chan_count);

	/* Increment counter, for next channel index calculation */
	lll->data_chan_counter++;

	/* Set the channel index for the auxiliary chain PDU */
	aux_ptr->chan_idx = chan_idx;
}
static void switch_radio_complete_and_b2b_tx(const struct lll_adv_sync *lll,
					     uint8_t phy_s)
{
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	if (lll->cte_started) {
		radio_switch_complete_and_phy_end_b2b_tx(phy_s, 0, phy_s, 0);
	} else
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
	{
		radio_switch_complete_and_b2b_tx(phy_s, 0, phy_s, 0);
	}
}
#endif /* CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK */
