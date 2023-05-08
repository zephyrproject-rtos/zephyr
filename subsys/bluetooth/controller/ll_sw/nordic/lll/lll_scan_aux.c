/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"
#include "hal/radio_df.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"
#include "util/mayfly.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_filter.h"
#include "lll_scan.h"
#include "lll_scan_aux.h"
#include "lll_df_types.h"
#include "lll_df_internal.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "lll_conn.h"
#include "lll_sched.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"
#include "lll_scan_internal.h"
#include "lll_sync_internal.h"

#include "ll_feat.h"

#include <soc.h>
#include <ull_scan_types.h>
#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_done(void *param);
static void isr_rx_ull_schedule(void *param);
static void isr_rx_lll_schedule(void *param);
static void isr_rx(struct lll_scan *lll, struct lll_scan_aux *lll_aux,
		   uint8_t phy_aux);
static int isr_rx_pdu(struct lll_scan *lll, struct lll_scan_aux *lll_aux,
		      struct node_rx_pdu *node_rx, struct pdu_adv *pdu,
		      uint8_t phy_aux, uint8_t phy_aux_flags_rx,
		      uint8_t devmatch_ok, uint8_t devmatch_id,
		      uint8_t irkmatch_ok, uint8_t irkmatch_id, uint8_t rl_idx,
		      uint8_t rssi_ready);
static void isr_tx_scan_req_ull_schedule(void *param);
static void isr_tx_scan_req_lll_schedule(void *param);
#if defined(CONFIG_BT_CENTRAL)
static void isr_tx_connect_req(void *param);
static void isr_rx_connect_rsp(void *param);
static bool isr_rx_connect_rsp_check(struct lll_scan *lll,
				     struct pdu_adv *pdu_tx,
				     struct pdu_adv *pdu_rx, uint8_t rl_idx);
static void isr_early_abort(void *param);
#endif /* CONFIG_BT_CENTRAL */

static uint16_t trx_cnt; /* TODO: move to a union in lll.c, common to all roles
			  */

int lll_scan_aux_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_scan_aux_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_scan_aux_prepare(void *param)
{
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	err = lll_prepare(is_abort_cb, abort_cb, prepare_cb, 0, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

uint8_t lll_scan_aux_setup(struct pdu_adv *pdu, uint8_t pdu_phy,
			   uint8_t pdu_phy_flags_rx, radio_isr_cb_t setup_cb,
			   void *param)
{
	struct pdu_adv_com_ext_adv *pri_com_hdr;
	struct pdu_adv_ext_hdr *pri_hdr;
	struct pdu_adv_aux_ptr *aux_ptr;
	struct pdu_cte_info *cte_info;
	struct node_rx_pdu *node_rx;
	uint32_t window_widening_us;
	uint32_t window_size_us;
	struct node_rx_ftr *ftr;
	uint32_t aux_offset_us;
	uint32_t overhead_us;
	uint8_t *pri_dptr;
	uint8_t phy;

	LL_ASSERT(pdu->type == PDU_ADV_TYPE_EXT_IND);

	/* Get reference to extended header */
	pri_com_hdr = (void *)&pdu->adv_ext_ind;
	if (!pdu->len || !pri_com_hdr->ext_hdr_len) {
		return 0U;
	}

	/* Get reference to flags and contents */
	pri_hdr = (void *)pri_com_hdr->ext_hdr_adv_data;
	pri_dptr = pri_hdr->data;

	/* traverse through adv_addr, if present */
	if (pri_hdr->adv_addr) {
		pri_dptr += BDADDR_SIZE;
	}

	/* traverse through tgt_addr, if present */
	if (pri_hdr->tgt_addr) {
		pri_dptr += BDADDR_SIZE;
	}

	/* traverse through cte_info, if present */
	if (pri_hdr->cte_info) {
		cte_info = (void *)pri_dptr;
		pri_dptr += sizeof(struct pdu_cte_info);
	} else {
		cte_info = NULL;
	}

	/* traverse through adi, if present */
	if (pri_hdr->adi) {
		pri_dptr += sizeof(struct pdu_adv_adi);
	}

	/* No need to scan further if no aux_ptr filled */
	aux_ptr = (void *)pri_dptr;
	if (unlikely(!pri_hdr->aux_ptr || !PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr) ||
		     (PDU_ADV_AUX_PTR_PHY_GET(aux_ptr) > EXT_ADV_AUX_PHY_LE_CODED))) {
		return 0;
	}

	/* Determine the window size */
	if (aux_ptr->offs_units) {
		window_size_us = OFFS_UNIT_300_US;
	} else {
		window_size_us = OFFS_UNIT_30_US;
	}

	/* Calculate the aux offset from start of the scan window */
	aux_offset_us = (uint32_t)PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr) * window_size_us;

	/* Calculate the window widening that needs to be deducted */
	if (aux_ptr->ca) {
		window_widening_us = SCA_DRIFT_50_PPM_US(aux_offset_us);
	} else {
		window_widening_us = SCA_DRIFT_500_PPM_US(aux_offset_us);
	}

	phy = BIT(PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));

	/* Calculate the minimum overhead to decide if LLL or ULL scheduling
	 * to be used for auxiliary PDU reception.
	 */
	overhead_us = PDU_AC_US(pdu->len, pdu_phy, pdu_phy_flags_rx);
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	/* Add CTE time if samples are available (8us unit) */
	/* TODO: check if CTE was actually enabled for rx */
	if (cte_info && radio_df_iq_samples_amount_get()) {
		overhead_us += cte_info->time << 3;
	}
#endif
	overhead_us += radio_rx_chain_delay_get(pdu_phy, pdu_phy_flags_rx);
	overhead_us += lll_radio_rx_ready_delay_get(phy, PHY_FLAGS_S8);
	overhead_us += window_widening_us;
	overhead_us += EVENT_TICKER_RES_MARGIN_US;
	overhead_us += EVENT_JITTER_US;

	/* Minimum prepare tick offset + minimum preempt tick offset are the
	 * overheads before ULL scheduling can setup radio for reception
	 */
	overhead_us +=
		HAL_TICKER_TICKS_TO_US(HAL_TICKER_CNTR_CMP_OFFSET_MIN << 1);

	/* CPU execution overhead to setup the radio for reception */
	overhead_us += EVENT_OVERHEAD_END_US + EVENT_OVERHEAD_START_US;

	/* Sufficient offset to ULL schedule the auxiliary PDU scan? */
	if (aux_offset_us > overhead_us) {
		return 0;
	}

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	/* Store the lll context, aux_ptr and start of PDU in footer */
	ftr = &(node_rx->hdr.rx_ftr);
	ftr->param = param;
	ftr->aux_ptr = aux_ptr;
	ftr->radio_end_us = radio_tmr_end_get() -
			    radio_rx_chain_delay_get(pdu_phy,
						     pdu_phy_flags_rx) -
			    PDU_AC_US(pdu->len, pdu_phy, pdu_phy_flags_rx);

	radio_isr_set(setup_cb, node_rx);
	radio_disable();

	return 1;
}

void lll_scan_aux_isr_aux_setup(void *param)
{
	struct pdu_adv_aux_ptr *aux_ptr;
	struct node_rx_pdu *node_rx;
	uint32_t window_widening_us;
	uint32_t window_size_us;
	struct node_rx_ftr *ftr;
	uint32_t aux_offset_us;
	uint32_t aux_start_us;
	struct lll_scan *lll;
	uint32_t start_us;
	uint8_t phy_aux;
	uint32_t hcto;

	lll_isr_status_reset();

	node_rx = param;
	ftr = &node_rx->hdr.rx_ftr;
	aux_ptr = ftr->aux_ptr;
	phy_aux = BIT(PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));
	ftr->aux_phy = phy_aux;

	lll = ftr->param;

	/* Determine the window size */
	if (aux_ptr->offs_units) {
		window_size_us = OFFS_UNIT_300_US;
	} else {
		window_size_us = OFFS_UNIT_30_US;
	}

	/* Calculate the aux offset from start of the scan window */
	aux_offset_us = (uint32_t)PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr) * window_size_us;

	/* Calculate the window widening that needs to be deducted */
	if (aux_ptr->ca) {
		window_widening_us = SCA_DRIFT_50_PPM_US(aux_offset_us);
	} else {
		window_widening_us = SCA_DRIFT_500_PPM_US(aux_offset_us);
	}

	/* Reset Tx/rx count */
	trx_cnt = 0U;

	/* Setup radio for auxiliary PDU scan */
	radio_phy_set(phy_aux, PHY_FLAGS_S8);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, LL_EXT_OCTETS_RX_MAX,
			    RADIO_PKT_CONF_PHY(phy_aux));
	lll_chan_set(aux_ptr->chan_idx);

	radio_pkt_rx_set(node_rx->pdu);

	/* FIXME: we could (?) use isr_rx_ull_schedule if already have aux
	 *        context allocated, i.e. some previous aux was scheduled from
	 *        ull already.
	 */
	radio_isr_set(isr_rx_lll_schedule, node_rx);

	/* setup tIFS switching */
	radio_tmr_tifs_set(EVENT_IFS_US);
	/* TODO: for passive scanning use complete_and_disable */
	radio_switch_complete_and_tx(phy_aux, 0, phy_aux, 1);

	/* TODO: skip filtering if AdvA was already found in previous PDU */

	if (0) {
#if defined(CONFIG_BT_CTLR_PRIVACY)
	} else if (ull_filter_lll_rl_enabled()) {
		const struct lll_filter *fal =
			ull_filter_lll_get((lll->filter_policy &
					    SCAN_FP_FILTER) != 0U);
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

		radio_filter_configure(fal->enable_bitmask,
				       fal->addr_type_bitmask,
				       (uint8_t *)fal->bdaddr);

		radio_ar_configure(count, irks, (phy_aux << 2) | BIT(1));
#endif /* CONFIG_BT_CTLR_PRIVACY */
	} else if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST) && lll->filter_policy) {
		/* Setup Radio Filter */
		const struct lll_filter *fal = ull_filter_lll_get(true);

		radio_filter_configure(fal->enable_bitmask,
				       fal->addr_type_bitmask,
				       (uint8_t *)fal->bdaddr);
	}

	/* Setup radio rx on micro second offset. Note that radio_end_us stores
	 * PDU start time in this case.
	 */
	aux_start_us = ftr->radio_end_us + aux_offset_us;
	aux_start_us -= lll_radio_rx_ready_delay_get(phy_aux, PHY_FLAGS_S8);
	aux_start_us -= window_widening_us;
	aux_start_us -= EVENT_JITTER_US;

	start_us = radio_tmr_start_us(0, aux_start_us);

	/* Setup header complete timeout */
	hcto = start_us;
	hcto += EVENT_JITTER_US;
	hcto += window_widening_us;
	hcto += lll_radio_rx_ready_delay_get(phy_aux, PHY_FLAGS_S8);
	hcto += window_size_us;
	hcto += radio_rx_chain_delay_get(phy_aux, PHY_FLAGS_S8);
	hcto += addr_us_get(phy_aux);
	radio_tmr_hcto_configure(hcto);

	/* capture end of Rx-ed PDU, extended scan to schedule auxiliary
	 * channel chaining, create connection or to create periodic sync.
	 */
	radio_tmr_end_capture();

	/* scanner always measures RSSI */
	radio_rssi_measure();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(phy_aux,
							  PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */
}

bool lll_scan_aux_addr_match_get(const struct lll_scan *lll,
				 const struct pdu_adv *pdu,
				 uint8_t *const devmatch_ok,
				 uint8_t *const devmatch_id,
				 uint8_t *const irkmatch_ok,
				 uint8_t *const irkmatch_id)
{
	const struct pdu_adv_ext_hdr *ext_hdr;

	ext_hdr = &pdu->adv_ext_ind.ext_hdr;
	if (!ext_hdr->adv_addr) {
		return false;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PRIVACY) && ull_filter_lll_rl_enabled()) {
		const struct lll_filter *fal =
			ull_filter_lll_get((lll->filter_policy &
					    SCAN_FP_FILTER) != 0);
		const uint8_t *adva = &ext_hdr->data[ADVA_OFFSET];

		*devmatch_ok = ull_filter_lll_fal_match(fal, pdu->tx_addr, adva,
							devmatch_id);
		if (!*devmatch_ok && pdu->tx_addr) {
			uint8_t count;

			(void)ull_filter_lll_irks_get(&count);
			if (count) {
				*irkmatch_ok = radio_ar_resolve(adva);
				*irkmatch_id = radio_ar_match_get();
			}
		}
	} else if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST) &&
		   lll->filter_policy) {
		const struct lll_filter *fal = ull_filter_lll_get(true);
		const uint8_t *adva = &ext_hdr->data[ADVA_OFFSET];

		*devmatch_ok = ull_filter_lll_fal_match(fal, pdu->tx_addr, adva,
							devmatch_id);
	}

	return true;
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	struct lll_scan_aux *lll_aux;
	struct node_rx_pdu *node_rx;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint32_t remainder_us;
	struct lll_scan *lll;
	struct ull_hdr *ull;
	uint8_t is_lll_scan;
	uint32_t remainder;
	uint32_t hcto;
	uint32_t aa;

	DEBUG_RADIO_START_O(1);

	lll_aux = p->param;
	lll = ull_scan_aux_lll_parent_get(lll_aux, &is_lll_scan);

	/* Check if this aux scan is for periodic advertising train */
	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && !is_lll_scan) {
		lll_sync_aux_prepare_cb((void *)lll, lll_aux);

		lll = NULL;

		goto sync_aux_prepare_done;
	}

#if defined(CONFIG_BT_CENTRAL)
	/* Check if stopped (on connection establishment race between
	 * LL and ULL.
	 */
	if (unlikely(lll->is_stop ||
		     (lll->conn &&
		      (lll->conn->central.initiated ||
		       lll->conn->central.cancelled)))) {
		radio_isr_set(isr_early_abort, lll_aux);
		radio_disable();

		return 0;
	}
#endif /* CONFIG_BT_CENTRAL */

	/* Initialize scanning state */
	lll_aux->state = 0U;

	/* Reset Tx/rx count */
	trx_cnt = 0U;

	/* Start setting up Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	radio_phy_set(lll_aux->phy, PHY_FLAGS_S8);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, LL_EXT_OCTETS_RX_MAX,
			    RADIO_PKT_CONF_PHY(lll_aux->phy));

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_pkt_rx_set(node_rx->pdu);

	aa = sys_cpu_to_le32(PDU_AC_ACCESS_ADDR);
	radio_aa_set((uint8_t *)&aa);
	radio_crc_configure(PDU_CRC_POLYNOMIAL,
				PDU_AC_CRC_IV);

	lll_chan_set(lll_aux->chan);

	radio_isr_set(isr_rx_ull_schedule, lll_aux);

	/* setup tIFS switching */
	radio_tmr_tifs_set(EVENT_IFS_US);
	/* TODO: for passive scanning use complete_and_disable */
	radio_switch_complete_and_tx(lll_aux->phy, 0, lll_aux->phy, 1);

	/* TODO: skip filtering if AdvA was already found in previous PDU */

	if (0) {
#if defined(CONFIG_BT_CTLR_PRIVACY)
	} else if (ull_filter_lll_rl_enabled()) {
		struct lll_filter *filter =
			ull_filter_lll_get((lll->filter_policy &
					    SCAN_FP_FILTER) != 0);
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

		radio_filter_configure(filter->enable_bitmask,
				       filter->addr_type_bitmask,
				       (uint8_t *) filter->bdaddr);

		radio_ar_configure(count, irks, (lll_aux->phy << 2) | BIT(1));
#endif /* CONFIG_BT_CTLR_PRIVACY */
	} else if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST) &&
		   lll->filter_policy) {
		/* Setup Radio Filter */
		struct lll_filter *fal = ull_filter_lll_get(true);

		radio_filter_configure(fal->enable_bitmask,
				       fal->addr_type_bitmask,
				       (uint8_t *)fal->bdaddr);
	}

sync_aux_prepare_done:
	/* Calculate event timings, coarse and fine */
	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll_aux);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0, ticks_at_start, remainder);

	hcto = remainder_us + lll_aux->window_size_us;
	hcto += radio_rx_ready_delay_get(lll_aux->phy, PHY_FLAGS_S8);
	hcto += addr_us_get(lll_aux->phy);
	hcto += radio_rx_chain_delay_get(lll_aux->phy, PHY_FLAGS_S8);
	radio_tmr_hcto_configure(hcto);

	/* capture end of Rx-ed PDU, extended scan to schedule auxiliary
	 * channel chaining, create connection or to create periodic sync.
	 */
	radio_tmr_end_capture();

	/* scanner always measures RSSI */
	radio_rssi_measure();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(lll_aux->phy,
							  PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	if (lll_preempt_calc(ull, (TICKER_ID_SCAN_AUX_BASE +
				   ull_scan_aux_lll_handle_get(lll_aux)),
				   ticks_at_event)) {
		radio_isr_set(isr_done, lll_aux);
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		uint32_t ret;

#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
		/* calc end of group in us for the anchor where next connection
		 * event to be placed.
		 */
		if (lll && lll->conn) {
			static memq_link_t link;
			static struct mayfly mfy_after_cen_offset_get = {
				0, 0, &link, NULL,
				ull_sched_mfy_after_cen_offset_get};

			/* NOTE: LLL scan instance passed, as done when
			 *       establishing legacy connections.
			 */
			p->param = lll;
			mfy_after_cen_offset_get.param = p;

			ret = mayfly_enqueue(TICKER_USER_ID_LLL,
					     TICKER_USER_ID_ULL_LOW, 1,
					     &mfy_after_cen_offset_get);
			LL_ASSERT(!ret);
		}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_CTLR_SCHED_ADVANCED */

		ret = lll_prepare_done(lll_aux);
		LL_ASSERT(!ret);
	}

	DEBUG_RADIO_START_O(1);

	return 0;
}

static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb)
{
	struct lll_scan *lll;

	/* Auxiliary context shall not resume when being preempted, i.e. they
	 * shall not use -EAGAIN as return value.
	 */
	ARG_UNUSED(resume_cb);

	/* Auxiliary event shall not overlap as they are not periodically
	 * scheduled.
	 */
	LL_ASSERT(next != curr);

	lll = ull_scan_lll_is_valid_get(next);
	if (lll) {
		/* Next event is scan context, let the current auxiliary scan
		 * continue.
		 */
		return 0;
	}

	/* Yield current auxiliary event to other than scan events */
	return -ECANCELED;
}

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
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

	lll_done(param);
}

static void isr_done(void *param)
{
	struct lll_sync *lll;
	uint8_t is_lll_scan;

	lll_isr_status_reset();

	if (param) {
		lll = ull_scan_aux_lll_parent_get(param, &is_lll_scan);
	} else {
		lll = NULL;
	}

	/* Check if this aux scan is for periodic advertising train */
	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) && lll && !is_lll_scan) {
		struct node_rx_pdu *node_rx;

		/* Generate message to release aux context and flag the report
		 * generated thereafter by HCI as incomplete.
		 */
		node_rx = ull_pdu_rx_alloc();
		LL_ASSERT(node_rx);

		node_rx->hdr.type = NODE_RX_TYPE_EXT_AUX_RELEASE;

		node_rx->hdr.rx_ftr.param = lll;
		node_rx->hdr.rx_ftr.aux_failed = 1U;

		ull_rx_put_sched(node_rx->hdr.link, node_rx);

	} else if (!trx_cnt) {
		struct event_done_extra *e;

		e = ull_done_extra_type_set(EVENT_DONE_EXTRA_TYPE_SCAN_AUX);
		LL_ASSERT(e);
	}

	lll_isr_cleanup(param);
}

static void isr_rx_ull_schedule(void *param)
{
	struct lll_scan_aux *lll_aux;
	struct lll_scan *lll;

	lll_aux = param;
	lll = ull_scan_aux_lll_parent_get(lll_aux, NULL);

	isr_rx(lll, lll_aux, lll_aux->phy);
}

static void isr_rx_lll_schedule(void *param)
{
	struct node_rx_pdu *node_rx;
	struct lll_scan *lll;
	uint8_t phy_aux;

	node_rx = param;
	lll = node_rx->hdr.rx_ftr.param;
	phy_aux = node_rx->hdr.rx_ftr.aux_phy; /* PHY remembered in node rx */

	/* scan context has used LLL scheduling for aux reception */
	if (lll->is_aux_sched) {
		isr_rx(lll, NULL, phy_aux);
	} else {
		/* `lll->lll_aux` would be allocated in ULL for LLL scheduled
		 * auxiliary PDU reception by scan context and for case
		 * where LLL scheduled chain PDU reception by aux context, it
		 * is assigned with the current aux context's LLL context.
		 */
		isr_rx(lll, lll->lll_aux, phy_aux);
	}
}

static void isr_rx(struct lll_scan *lll, struct lll_scan_aux *lll_aux,
		   uint8_t phy_aux)
{
	struct node_rx_pdu *node_rx;
	uint8_t phy_aux_flags_rx;
	uint8_t devmatch_ok;
	uint8_t devmatch_id;
	uint8_t irkmatch_ok;
	uint8_t irkmatch_id;
	struct pdu_adv *pdu;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t crc_ok;
	uint8_t rl_idx;
	bool has_adva;
	int err;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		devmatch_ok = radio_filter_has_match();
		devmatch_id = radio_filter_match_get();
		if (IS_ENABLED(CONFIG_BT_CTLR_PRIVACY)) {
			irkmatch_ok = radio_ar_has_match();
			irkmatch_id = radio_ar_match_get();
		} else {
			irkmatch_ok = 0U;
			irkmatch_id = FILTER_IDX_NONE;
		}
		rssi_ready = radio_rssi_is_ready();
		phy_aux_flags_rx = radio_phy_flags_rx_get();
	} else {
		crc_ok = devmatch_ok = irkmatch_ok = rssi_ready =
			phy_aux_flags_rx = 0U;
		devmatch_id = irkmatch_id = FILTER_IDX_NONE;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* No Rx */
	if (!trx_done || !crc_ok) {
		/* TODO: Combine the early exit with above if-then-else block
		 */
		err = -EINVAL;

		goto isr_rx_do_close;
	}

	node_rx = ull_pdu_rx_alloc_peek(3);
	if (!node_rx) {
		err = -ENOBUFS;

		goto isr_rx_do_close;
	}

	pdu = (void *)node_rx->pdu;
	if ((pdu->type != PDU_ADV_TYPE_EXT_IND) || !pdu->len) {
		err = -EINVAL;

		goto isr_rx_do_close;
	}

	has_adva = lll_scan_aux_addr_match_get(lll, pdu, &devmatch_ok,
					       &devmatch_id, &irkmatch_ok,
					       &irkmatch_id);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	rl_idx = devmatch_ok ?
		 ull_filter_lll_rl_idx(((lll->filter_policy &
					 SCAN_FP_FILTER) != 0U),
				       devmatch_id) :
		 irkmatch_ok ? ull_filter_lll_rl_irk_idx(irkmatch_id) :
		 FILTER_IDX_NONE;
#else
	rl_idx = FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	if (has_adva) {
		bool allow;

		allow = lll_scan_isr_rx_check(lll, irkmatch_ok, devmatch_ok,
					      rl_idx);
		if (false) {
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC) && \
	defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
		} else if (allow || lll->is_sync) {
			devmatch_ok = allow ? 1U : 0U;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC && CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */
		} else if (!allow) {
			err = -EINVAL;

			goto isr_rx_do_close;
		}
	}

	err = isr_rx_pdu(lll, lll_aux, node_rx, pdu, phy_aux, phy_aux_flags_rx,
			 devmatch_ok, devmatch_id, irkmatch_ok, irkmatch_ok,
			 rl_idx, rssi_ready);
	if (!err) {
		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_send();
		}

		return;
	}

isr_rx_do_close:
	if (lll_aux) {
		radio_isr_set(isr_done, NULL);
	} else {
		/* Send message to flush Auxiliary PDU list */
		if (lll->is_aux_sched && err != -ECANCELED) {
			struct node_rx_pdu *node_rx;

			node_rx = ull_pdu_rx_alloc();
			LL_ASSERT(node_rx);

			node_rx->hdr.type = NODE_RX_TYPE_EXT_AUX_RELEASE;

			/* Use LLL scan context pointer which will be resolved
			 * to LLL aux context in the `ull_scan_aux_release`
			 * function in ULL execution context.
			 * As ULL execution context is the one assigning the
			 * `lll->lll_aux`, if it has not been assigned then
			 * `ull_scan_aux_release` will not dereference it, but
			 * under race, if ULL execution did assign one, it will
			 * free it.
			 */
			node_rx->hdr.rx_ftr.param = lll;

			ull_rx_put_sched(node_rx->hdr.link, node_rx);
		}

		/* Check if LLL scheduled auxiliary PDU reception by scan
		 * context or auxiliary PDU reception by aux context
		 */
		if (lll->is_aux_sched) {
			lll->is_aux_sched = 0U;

			/* Go back to resuming primary channel scanning */
			radio_isr_set(lll_scan_isr_resume, lll);
		} else {
			/* auxiliary channel radio event done */
			radio_isr_set(isr_done, NULL);
		}
	}
	radio_disable();
}

static int isr_rx_pdu(struct lll_scan *lll, struct lll_scan_aux *lll_aux,
		      struct node_rx_pdu *node_rx, struct pdu_adv *pdu,
		      uint8_t phy_aux, uint8_t phy_aux_flags_rx,
		      uint8_t devmatch_ok, uint8_t devmatch_id,
		      uint8_t irkmatch_ok, uint8_t irkmatch_id, uint8_t rl_idx,
		      uint8_t rssi_ready)
{
	struct node_rx_ftr *ftr;

	bool dir_report = false;

	if (0) {
#if defined(CONFIG_BT_CENTRAL)
	/* Initiator */
	} else if (lll->conn && !lll->conn->central.cancelled &&
		   (pdu->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_CONN) &&
		   lll_scan_ext_tgta_check(lll, false, true, pdu,
					   rl_idx, NULL)) {
		struct lll_scan_aux *lll_aux_to_use;
		struct node_rx_ftr *ftr;
		struct node_rx_pdu *rx;
		struct pdu_adv *pdu_tx;
		uint32_t conn_space_us;
		struct ull_hdr *ull;
		uint32_t pdu_end_us;
		uint8_t init_tx_addr;
		uint8_t *init_addr;
#if defined(CONFIG_BT_CTLR_PRIVACY)
		bt_addr_t *lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		if (!lll_aux) {
			lll_aux_to_use = lll->lll_aux;
		} else {
			lll_aux_to_use = lll_aux;
		}

		if (!lll_aux_to_use) {
			/* Return -ECHILD, as ULL execution has not yet assigned
			 * an aux context. This can happen only under LLL
			 * scheduling where in LLL auxiliary channel PDU
			 * reception is spawn from LLL primary channel scanning
			 * and on completion will join back to resume primary
			 * channel PDU scanning.
			 */
			return -ECHILD;
		}

		/* Always use CSA#2 on secondary channel, we need 2 nodes for conn
		 * and CSA#2 events and 2 nodes are always reserved for connection.
		 */
		rx = ull_pdu_rx_alloc_peek(4);
		if (!rx) {
			return -ENOBUFS;
		}

		pdu_end_us = radio_tmr_end_get();
		if (!lll->ticks_window) {
			uint32_t scan_interval_us;

			/* FIXME: is this correct for continuous scanning? */
			scan_interval_us = lll->interval * SCAN_INT_UNIT_US;
			pdu_end_us %= scan_interval_us;
		}

		/* AUX_CONNECT_REQ is the same as CONNECT_IND */
		const uint8_t aux_connect_req_len =
			sizeof(struct pdu_adv_connect_ind);
		/* AUX_CONNECT_RSP has only AdvA and TargetA in extended common
		 * header
		 */
		const uint8_t aux_connect_rsp_len =
			PDU_AC_EXT_HEADER_SIZE_MIN +
			sizeof(struct pdu_adv_ext_hdr) +
			ADVA_SIZE + TARGETA_SIZE;

		ull = HDR_LLL2ULL(lll);
		if (pdu_end_us > (HAL_TICKER_TICKS_TO_US(ull->ticks_slot) -
				  EVENT_IFS_US -
				  PDU_AC_MAX_US(aux_connect_req_len, phy_aux) -
				  EVENT_IFS_US -
				  PDU_AC_MAX_US(aux_connect_rsp_len, phy_aux) -
				  EVENT_OVERHEAD_START_US -
				  EVENT_TICKER_RES_MARGIN_US)) {
			return -ETIME;
		}

#if defined(CONFIG_BT_CTLR_PRIVACY)
		lrpa = ull_filter_lll_lrpa_get(rl_idx);
		if (lll->rpa_gen && lrpa) {
			init_tx_addr = 1;
			init_addr = lrpa->val;
		} else {
#else
		if (1) {
#endif
			init_tx_addr = lll->init_addr_type;
			init_addr = lll->init_addr;
		}

		pdu_tx = radio_pkt_scratch_get();

		lll_scan_prepare_connect_req(lll, pdu_tx, phy_aux,
					     phy_aux_flags_rx, pdu->tx_addr,
					     pdu->adv_ext_ind.ext_hdr.data,
					     init_tx_addr, init_addr,
					     &conn_space_us);

		radio_pkt_tx_set(pdu_tx);

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();
		}

		/* capture end of Tx-ed PDU, used to calculate HCTO. */
		radio_tmr_end_capture();

		radio_tmr_tifs_set(EVENT_IFS_US);
		radio_switch_complete_and_rx(phy_aux);
		radio_isr_set(isr_tx_connect_req, lll_aux_to_use);

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			/* PA/LNA enable is overwriting packet end
			 * used in ISR profiling, hence back it up
			 * for later use.
			 */
			lll_prof_radio_end_backup();
		}
		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 EVENT_IFS_US -
					 radio_rx_chain_delay_get(phy_aux,
						phy_aux_flags_rx) -
					 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		if (rssi_ready) {
			lll->conn->rssi_latest =  radio_rssi_get();
		}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

		/* block CPU so that there is no CRC error on pdu tx,
		 * this is only needed if we want the CPU to sleep.
		 * while(!radio_has_disabled())
		 * {cpu_sleep();}
		 * radio_status_reset();
		 */

		/* Stop further connection initiation */
		lll->conn->central.initiated = 1U;

		/* Stop further initiating events */
		lll->is_stop = 1U;

		/* Populate the connection complete message */
		rx = ull_pdu_rx_alloc();
		rx->hdr.type = NODE_RX_TYPE_CONNECTION;
		rx->hdr.handle = 0xffff;

		(void)memcpy(rx->pdu, pdu_tx,
			     (offsetof(struct pdu_adv, connect_ind) +
			      sizeof(struct pdu_adv_connect_ind)));

		/* ChSel is RFU in AUX_ADV_IND but we do need to use CSA#2 for
		 * connections initiated on the secondary advertising channel
		 * thus overwrite chan_sel to make it work seamlessly.
		 */
		pdu = (void *)rx->pdu;
		pdu->chan_sel = 1;

		ftr = &(rx->hdr.rx_ftr);
		ftr->param = lll;
		ftr->ticks_anchor = radio_tmr_start_get();
		ftr->radio_end_us = conn_space_us;

#if defined(CONFIG_BT_CTLR_PRIVACY)
		ftr->rl_idx = irkmatch_ok ? rl_idx : FILTER_IDX_NONE;
		ftr->lrpa_used = lll->rpa_gen && lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		ftr->extra = ull_pdu_rx_alloc();

		/* Hold onto connection event message until after successful
		 * reception of CONNECT_RSP
		 */
		lll_aux_to_use->node_conn_rx = rx;

		/* Increase trx count so as to not generate done extra event
		 * when LLL scheduling of Auxiliary PDU reception
		 */
		if (!lll_aux) {
			trx_cnt++;
		}

		return 0;

	/* Active scanner */
	} else if (!lll->conn &&
		   lll->type &&
		   ((lll_aux && !lll_aux->state) ||
		    (lll->lll_aux && !lll->lll_aux->state)) &&
		   (pdu->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_SCAN) &&
		   lll_scan_ext_tgta_check(lll, false, false, pdu, rl_idx,
					   &dir_report)) {
#else /* !CONFIG_BT_CENTRAL */
	} else if (lll && lll->type &&
		   ((lll_aux && !lll_aux->state) ||
		    (lll->lll_aux && !lll->lll_aux->state)) &&
		   (pdu->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_SCAN) &&
		   lll_scan_ext_tgta_check(lll, false, false, pdu, rl_idx,
					   &dir_report)) {
#endif /* !CONFIG_BT_CENTRAL */
		struct node_rx_pdu *rx;
		struct pdu_adv *pdu_tx;
#if defined(CONFIG_BT_CTLR_PRIVACY)
		bt_addr_t *lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		/* Check if 4 nodes free, 2 will be utilized for aux PDU and
		 * scan response PDU; 2 more to ensure connections have them.
		 */
		rx = ull_pdu_rx_alloc_peek(4);
		if (!rx) {
			return -ENOBUFS;
		}

		/* setup tIFS switching */
		radio_tmr_tifs_set(EVENT_IFS_US);
		radio_switch_complete_and_rx(phy_aux);

		/* prepare the scan request packet */
		pdu_tx = (void *)radio_pkt_scratch_get();
		pdu_tx->type = PDU_ADV_TYPE_SCAN_REQ;
		pdu_tx->rx_addr = pdu->tx_addr;
		pdu_tx->len = sizeof(struct pdu_adv_scan_req);
#if defined(CONFIG_BT_CTLR_PRIVACY)
		lrpa = ull_filter_lll_lrpa_get(rl_idx);
		if (lll->rpa_gen && lrpa) {
			pdu_tx->tx_addr = 1;
			(void)memcpy(pdu_tx->scan_req.scan_addr, lrpa->val,
				     BDADDR_SIZE);
		} else {
#else
		if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
			pdu_tx->tx_addr = lll->init_addr_type;
			(void)memcpy(pdu_tx->scan_req.scan_addr, lll->init_addr,
				     BDADDR_SIZE);
		}
		(void)memcpy(pdu_tx->scan_req.adv_addr,
			     &pdu->adv_ext_ind.ext_hdr.data[ADVA_OFFSET],
			     BDADDR_SIZE);

		radio_pkt_tx_set(pdu_tx);

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();
		}

		/* capture end of Tx-ed PDU, used to calculate HCTO. */
		radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			/* PA/LNA enable is overwriting packet end
			 * used in ISR profiling, hence back it up
			 * for later use.
			 */
			lll_prof_radio_end_backup();
		}

		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 EVENT_IFS_US -
					 radio_rx_chain_delay_get(phy_aux,
						phy_aux_flags_rx) -
					 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

		(void)ull_pdu_rx_alloc();

		node_rx->hdr.type = NODE_RX_TYPE_EXT_AUX_REPORT;

		ftr = &(node_rx->hdr.rx_ftr);
		if (lll_aux) {
			ftr->param = lll_aux;
			radio_isr_set(isr_tx_scan_req_ull_schedule,
				      lll_aux);
			lll_aux->state = 1U;
		} else {
			ftr->param = lll;
			radio_isr_set(isr_tx_scan_req_lll_schedule,
				      node_rx);
			lll->lll_aux->state = 1U;
		}
		ftr->ticks_anchor = radio_tmr_start_get();
		ftr->radio_end_us = radio_tmr_end_get() -
				    radio_rx_chain_delay_get(phy_aux,
							     phy_aux_flags_rx);
		ftr->rssi = (rssi_ready) ? radio_rssi_get() :
			    BT_HCI_LE_RSSI_NOT_AVAILABLE;
		ftr->scan_req = 1U;
		ftr->scan_rsp = 0U;

#if defined(CONFIG_BT_CTLR_PRIVACY)
		ftr->rl_idx = irkmatch_ok ? rl_idx : FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
		ftr->direct = dir_report;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC) && \
	defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
		ftr->devmatch = devmatch_ok;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC && CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

		ftr->aux_lll_sched = 0U;

		ull_rx_put_sched(node_rx->hdr.link, node_rx);

		return 0;

	/* Passive scanner or scan responses */
#if defined(CONFIG_BT_CENTRAL)
	} else if (!lll->conn &&
		   ((lll_aux && lll_aux->is_chain_sched) ||
		    (lll->lll_aux && lll->lll_aux->is_chain_sched) ||
		    lll_scan_ext_tgta_check(lll, false, false, pdu, rl_idx,
					    &dir_report))) {
#else /* !CONFIG_BT_CENTRAL */
	} else if ((lll_aux && lll_aux->is_chain_sched) ||
		   (lll->lll_aux && lll->lll_aux->is_chain_sched) ||
		   lll_scan_ext_tgta_check(lll, false, false, pdu, rl_idx,
					   &dir_report)) {
#endif /* !CONFIG_BT_CENTRAL */

		ftr = &(node_rx->hdr.rx_ftr);
		if (lll_aux) {
			ftr->param = lll_aux;
			ftr->scan_rsp = lll_aux->state;

			/* Further auxiliary PDU reception will be chain PDUs */
			lll_aux->is_chain_sched = 1U;
		} else if (lll->lll_aux) {
			ftr->param = lll;
			ftr->scan_rsp = lll->lll_aux->state;

			/* Further auxiliary PDU reception will be chain PDUs */
			lll->lll_aux->is_chain_sched = 1U;
		} else {
			/* Return -ECHILD, as ULL execution has not yet assigned
			 * an aux context. This can happen only under LLL
			 * scheduling where in LLL auxiliary channel PDU
			 * reception is spawn from LLL primary channel scanning
			 * and on completion will join back to resume primary
			 * channel PDU scanning.
			 */
			return -ECHILD;
		}

		/* Allocate before `lll_scan_aux_setup` call, so that a new
		 * free PDU buffer is used to receive auxiliary PDU when using
		 * LLL scheduling.
		 */
		(void)ull_pdu_rx_alloc();

		ftr->ticks_anchor = radio_tmr_start_get();
		ftr->radio_end_us = radio_tmr_end_get() -
				    radio_rx_chain_delay_get(phy_aux,
							     phy_aux_flags_rx);
		ftr->phy_flags = phy_aux_flags_rx;
		ftr->rssi = (rssi_ready) ? radio_rssi_get() :
			    BT_HCI_LE_RSSI_NOT_AVAILABLE;
		ftr->scan_req = 0U;

#if defined(CONFIG_BT_CTLR_PRIVACY)
		ftr->rl_idx = irkmatch_ok ? rl_idx : FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
		ftr->direct = dir_report;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC) && \
	defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
		ftr->devmatch = devmatch_ok;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC && CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

		ftr->aux_lll_sched = lll_scan_aux_setup(pdu, phy_aux,
							phy_aux_flags_rx,
							lll_scan_aux_isr_aux_setup,
							lll);

		node_rx->hdr.type = NODE_RX_TYPE_EXT_AUX_REPORT;

		ull_rx_put_sched(node_rx->hdr.link, node_rx);

		/* Next aux scan is scheduled from LLL, we already handled radio
		 * disable so prevent caller from doing it again.
		 */
		if (ftr->aux_lll_sched) {
			if (!lll_aux) {
				lll->is_aux_sched = 1U;
			}

			if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
				lll_prof_cputime_capture();
			}

			return 0;
		}

		/* Increase trx count so as to not generate done extra event
		 * as a valid Auxiliary PDU node rx is being reported to ULL.
		 */
		trx_cnt++;

		return -ECANCELED;
	}

	return -EINVAL;
}

static void isr_tx(struct lll_scan_aux *lll_aux, void *pdu_rx,
		   void (*isr)(void *), void *param)
{
	uint32_t hcto;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Clear radio tx status and events */
	lll_isr_tx_status_reset();

	/* complete the reception and disable radio  */
	radio_switch_complete_and_disable();

	radio_pkt_rx_set(pdu_rx);

	/* assert if radio packet ptr is not set and radio started rx */
	LL_ASSERT(!radio_is_ready());

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_enabled()) {
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

		radio_ar_configure(count, irks, (lll_aux->phy << 2) | BIT(1));
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	/* +/- 2us active clock jitter, +1 us hcto compensation */
	hcto = radio_tmr_tifs_base_get() + EVENT_IFS_US +
	       (EVENT_CLOCK_JITTER_US << 1U) + RANGE_DELAY_US + 1U;
	hcto += radio_rx_chain_delay_get(lll_aux->phy, PHY_FLAGS_S8);
	hcto += addr_us_get(lll_aux->phy);
	hcto -= radio_tx_chain_delay_get(lll_aux->phy, PHY_FLAGS_S8);
	radio_tmr_hcto_configure(hcto);

	/* capture end of Rx-ed PDU, extended scan to schedule auxiliary
	 * channel chaining.
	 */
	radio_tmr_end_capture();

	/* scanner always measures RSSI */
	radio_rssi_measure();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* PA/LNA enable is overwriting packet end used in ISR
		 * profiling, hence back it up for later use.
		 */
		lll_prof_radio_end_backup();
	}

	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() + EVENT_IFS_US -
				 (EVENT_CLOCK_JITTER_US << 1U) -
				 radio_tx_chain_delay_get(lll_aux->phy,
							  PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

	radio_isr_set(isr, param);

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* NOTE: as scratch packet is used to receive, it is safe to
		 * generate profile event using rx nodes.
		 */
		lll_prof_send();
	}
}

static void isr_tx_scan_req_ull_schedule(void *param)
{
	struct node_rx_pdu *node_rx;

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	isr_tx(param, node_rx->pdu, isr_rx_ull_schedule, param);
}

static void isr_tx_scan_req_lll_schedule(void *param)
{
	struct node_rx_pdu *node_rx_adv = param;
	struct node_rx_pdu *node_rx;
	struct lll_scan *lll;

	lll = node_rx_adv->hdr.rx_ftr.param;

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	isr_tx(lll->lll_aux, node_rx->pdu, isr_rx_lll_schedule, param);
}

#if defined(CONFIG_BT_CENTRAL)
static void isr_tx_connect_req(void *param)
{
	struct node_rx_pdu *node_rx;

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	isr_tx(param, (void *)node_rx->pdu, isr_rx_connect_rsp, param);
}

static void isr_rx_connect_rsp(void *param)
{
	struct lll_scan_aux *lll_aux;
	struct pdu_adv *pdu_rx;
	struct node_rx_pdu *rx;
	struct lll_scan *lll;
	uint8_t irkmatch_ok;
	uint8_t irkmatch_id;
	uint8_t trx_done;
	uint8_t rl_idx;
	uint8_t crc_ok;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Read radio status */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		if (IS_ENABLED(CONFIG_BT_CTLR_PRIVACY)) {
			irkmatch_ok = radio_ar_has_match();
			irkmatch_id = radio_ar_match_get();
		} else {
			irkmatch_ok = 0U;
			irkmatch_id = FILTER_IDX_NONE;
		}
	} else {
		crc_ok = irkmatch_ok = 0U;
		irkmatch_id = FILTER_IDX_NONE;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* Get the reference to primary scanner's LLL context */
	lll_aux = param;
	lll = ull_scan_aux_lll_parent_get(lll_aux, NULL);

	/* Use the reserved/saved node rx to generate connection complete or
	 * release it if failed to receive AUX_CONNECT_RSP PDU.
	 */
	rx = lll_aux->node_conn_rx;
	LL_ASSERT(rx);
	lll_aux->node_conn_rx = NULL;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	rl_idx = irkmatch_ok ? ull_filter_lll_rl_irk_idx(irkmatch_id) :
			       FILTER_IDX_NONE;
#else
	rl_idx = FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	/* Check for PDU reception */
	if (trx_done && crc_ok) {
		struct node_rx_pdu *node_rx;
		struct pdu_adv *pdu_tx;

		pdu_tx = radio_pkt_scratch_get();

		node_rx = ull_pdu_rx_alloc_peek(1);
		LL_ASSERT(node_rx);
		pdu_rx = (void *)node_rx->pdu;

		trx_done = isr_rx_connect_rsp_check(lll, pdu_tx, pdu_rx,
						    rl_idx);
	} else {
		trx_done = 0U;
	}

	/* No Rx or invalid PDU received */
	if (!trx_done) {
		struct node_rx_ftr *ftr;

		/* Try again with connection initiation */
		lll->conn->central.initiated = 0U;

		/* Dont stop initiating events on primary channels */
		lll->is_stop = 0U;

		ftr = &(rx->hdr.rx_ftr);

		rx->hdr.type = NODE_RX_TYPE_RELEASE;
		ull_rx_put(rx->hdr.link, rx);

		rx = ftr->extra;
		rx->hdr.type = NODE_RX_TYPE_RELEASE;
		goto isr_rx_do_close;
	}

	/* Update the max Tx and Rx time; and connection PHY based on the
	 * extended advertising PHY used to establish the connection.
	 */
#if defined(CONFIG_BT_CTLR_PHY)
	struct lll_conn *conn_lll = lll->conn;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	conn_lll->dle.eff.max_tx_time = MAX(conn_lll->dle.eff.max_tx_time,
					    PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN,
							  lll_aux->phy));
	conn_lll->dle.eff.max_rx_time = MAX(conn_lll->dle.eff.max_rx_time,
					    PDU_DC_MAX_US(PDU_DC_PAYLOAD_SIZE_MIN,
							  lll_aux->phy));
#endif /* CONFIG_BT_CTLR_DATA_LENGTH*/

	conn_lll->phy_tx = lll_aux->phy;
	conn_lll->phy_tx_time = lll_aux->phy;
	conn_lll->phy_rx = lll_aux->phy;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (irkmatch_ok) {
		struct node_rx_ftr *ftr;
		struct pdu_adv *pdu;

		pdu = (void *)rx->pdu;
		pdu->rx_addr = pdu_rx->tx_addr;
		(void)memcpy(pdu->connect_ind.adv_addr,
			     &pdu_rx->adv_ext_ind.ext_hdr.data[ADVA_OFFSET],
			     BDADDR_SIZE);
		ftr = &(rx->hdr.rx_ftr);
		ftr->rl_idx = rl_idx;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

isr_rx_do_close:
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

	ull_rx_put_sched(rx->hdr.link, rx);

	if (lll->lll_aux) {
		struct node_rx_pdu *node_rx;

		/* Send message to flush Auxiliary PDU list */
		node_rx = ull_pdu_rx_alloc();
		LL_ASSERT(node_rx);

		node_rx->hdr.type = NODE_RX_TYPE_EXT_AUX_RELEASE;

		node_rx->hdr.rx_ftr.param = lll->lll_aux;

		ull_rx_put_sched(node_rx->hdr.link, node_rx);

		radio_isr_set(lll_scan_isr_resume, lll);
	} else {
		radio_isr_set(isr_done, NULL);
	}

	radio_disable();

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_send();
	}
}

static bool isr_rx_connect_rsp_check(struct lll_scan *lll,
				     struct pdu_adv *pdu_tx,
				     struct pdu_adv *pdu_rx, uint8_t rl_idx)
{
	if (pdu_rx->type != PDU_ADV_TYPE_AUX_CONNECT_RSP) {
		return false;
	}

	if (pdu_rx->len != offsetof(struct pdu_adv_com_ext_adv,
				    ext_hdr_adv_data) +
			   offsetof(struct pdu_adv_ext_hdr, data) + ADVA_SIZE +
			   TARGETA_SIZE) {
		return false;
	}

	if (pdu_rx->adv_ext_ind.adv_mode ||
	    !pdu_rx->adv_ext_ind.ext_hdr.adv_addr ||
	    !pdu_rx->adv_ext_ind.ext_hdr.tgt_addr) {
		return false;
	}

	return lll_scan_adva_check(lll, pdu_rx->tx_addr,
			&pdu_rx->adv_ext_ind.ext_hdr.data[ADVA_OFFSET],
			rl_idx) &&
	       (pdu_rx->rx_addr == pdu_tx->tx_addr) &&
	       (memcmp(&pdu_rx->adv_ext_ind.ext_hdr.data[TGTA_OFFSET],
		       pdu_tx->connect_ind.init_addr, BDADDR_SIZE) == 0);
}

static void isr_early_abort(void *param)
{
	struct event_done_extra *e;

	e = ull_done_extra_type_set(EVENT_DONE_EXTRA_TYPE_SCAN_AUX);
	LL_ASSERT(e);

	lll_isr_early_abort(param);
}
#endif /* CONFIG_BT_CENTRAL */
