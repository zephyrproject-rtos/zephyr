/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <sys/byteorder.h>
#include <sys/util.h>

#include <bluetooth/hci.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/memq.h"
#include "util/mayfly.h"

#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_filter.h"
#include "lll_scan.h"
#include "lll_scan_aux.h"
#include "lll_conn.h"
#include "lll_sched.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"
#include "lll_scan_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_scan_aux
#include "common/log.h"
#include <soc.h>
#include <ull_scan_types.h>
#include "hal/debug.h"

#if defined(CONFIG_BT_CENTRAL)
static uint8_t lll_scan_connect_req_pdu[PDU_AC_LL_HEADER_SIZE +
					sizeof(struct pdu_adv_connect_ind)];
#endif /* CONFIG_BT_CENTRAL */

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_done(void *param);
static void isr_rx(void *param);
static int isr_rx_pdu(struct lll_scan_aux *lll, uint8_t devmatch_ok,
		      uint8_t devmatch_id, uint8_t irkmatch_ok,
		      uint8_t irkmatch_id, uint8_t rl_idx, uint8_t rssi_ready);
#if defined(CONFIG_BT_CENTRAL)
static bool isr_scan_connect_rsp_check(struct lll_scan *lll,
				       struct pdu_adv *pdu_tx,
				       struct pdu_adv *pdu_rx, uint8_t rl_idx);
static void isr_tx_connect_req(void *param);
static void isr_rx_connect_rsp(void *param);
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

	err = lll_prepare(lll_is_abort_cb, abort_cb, prepare_cb, 0, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	struct node_rx_pdu *node_rx;
	struct ll_scan_aux_set *aux_set;
	struct ll_scan_set *scan_set;
	struct lll_scan *lll_scan;
	struct lll_scan_aux *lll;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	struct ull_hdr *ull;
	uint32_t remainder_us;
	uint32_t remainder;
	uint32_t hcto;
	uint32_t aa;

	DEBUG_RADIO_START_O(1);

	lll = p->param;
	aux_set = HDR_LLL2ULL(lll);
	scan_set = HDR_LLL2ULL(aux_set->rx_head->rx_ftr.param);
	lll_scan = &scan_set->lll;

#if defined(CONFIG_BT_CENTRAL)
	/* Check if stopped (on connection establishment race between LLL and
	 * ULL.
	 */
	if (unlikely(lll_scan->is_stop ||
		     (lll_scan->conn &&
		      (lll_scan->conn->master.initiated ||
		       lll_scan->conn->master.cancelled)))) {
		radio_isr_set(isr_early_abort, lll);
		radio_disable();

		return 0;
	}
#endif /* CONFIG_BT_CENTRAL */

	/* Start setting up Radio h/w */
	radio_reset();

	/* Reset Tx/rx count */
	trx_cnt = 0U;

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	radio_phy_set(lll->phy, 1);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (lll->phy << 1));

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_pkt_rx_set(node_rx->pdu);

	aa = sys_cpu_to_le32(PDU_AC_ACCESS_ADDR);
	radio_aa_set((uint8_t *)&aa);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    0x555555);

	lll_chan_set(lll->chan);

	radio_isr_set(isr_rx, lll);

	/* setup tIFS switching */
	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_tx(lll->phy, 0, lll->phy, 1);

	/* TODO: skip filtering if AdvA was already found in previous PDU */

	if (0) {
#if defined(CONFIG_BT_CTLR_PRIVACY)
	} else if (ull_filter_lll_rl_enabled()) {
		struct lll_filter *filter = ull_filter_lll_get(
			!!(lll_scan->filter_policy & 0x1));
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

		radio_filter_configure(filter->enable_bitmask,
				       filter->addr_type_bitmask,
				       (uint8_t *) filter->bdaddr);

		radio_ar_configure(count, irks, (lll->phy << 2) | BIT(1));
#endif /* CONFIG_BT_CTLR_PRIVACY */
	} else if (IS_ENABLED(CONFIG_BT_CTLR_FILTER) &&
		   lll_scan->filter_policy) {
		/* Setup Radio Filter */
		struct lll_filter *wl = ull_filter_lll_get(true);

		radio_filter_configure(wl->enable_bitmask,
				       wl->addr_type_bitmask,
				       (uint8_t *)wl->bdaddr);
	}

	/* Calculate event timings, coarse and fine */
	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0, ticks_at_start, remainder);

	hcto = remainder_us + lll->window_size_us;
	hcto += radio_rx_ready_delay_get(lll->phy, 1);
	hcto += addr_us_get(lll->phy);
	hcto += radio_rx_chain_delay_get(lll->phy, 1);
	radio_tmr_hcto_configure(hcto);

	/* capture end of Rx-ed PDU, extended scan to schedule auxiliary
	 * channel chaining or to create periodic sync.
	 */
	radio_tmr_end_capture();

	/* scanner always measures RSSI */
	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(lll->phy, 1) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	if (lll_preempt_calc(ull, (TICKER_ID_SCAN_AUX_BASE +
				   ull_scan_aux_lll_handle_get(lll)),
			     ticks_at_event)) {
		radio_isr_set(isr_done, lll);
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		uint32_t ret;

#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
		/* calc end of group in us for the anchor where next connection
		 * event to be placed.
		 */
		if (lll_scan->conn) {
			static memq_link_t link;
			static struct mayfly mfy_after_mstr_offset_get = {
				0, 0, &link, NULL,
				ull_sched_mfy_after_mstr_offset_get};

			/* NOTE: LLL scan instance passed, as done when
			 *       establishing legacy connections.
			 */
			p->param = lll_scan;
			mfy_after_mstr_offset_get.param = p;

			ret = mayfly_enqueue(TICKER_USER_ID_LLL,
					     TICKER_USER_ID_ULL_LOW, 1,
					     &mfy_after_mstr_offset_get);
			LL_ASSERT(!ret);
		}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_CTLR_SCHED_ADVANCED */

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	DEBUG_RADIO_START_O(1);

	return 0;
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
	struct event_done_extra *e;

	lll_isr_status_reset();

	if (!trx_cnt) {
		e = ull_event_done_extra_get();
		LL_ASSERT(e);

		e->type = EVENT_DONE_EXTRA_TYPE_SCAN_AUX;
	}

	lll_isr_cleanup(param);
}

static void isr_rx(void *param)
{
	struct lll_scan_aux *lll_aux;
	struct ll_scan_aux_set *aux;
	struct ll_scan_set *scan;
	struct lll_scan *lll;
	uint8_t devmatch_ok;
	uint8_t devmatch_id;
	uint8_t irkmatch_ok;
	uint8_t irkmatch_id;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t crc_ok;
	uint8_t rl_idx;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		devmatch_ok = radio_filter_has_match();
		devmatch_id = radio_filter_match_get();
		irkmatch_ok = radio_ar_has_match();
		irkmatch_id = radio_ar_match_get();
		rssi_ready = radio_rssi_is_ready();
	} else {
		crc_ok = devmatch_ok = irkmatch_ok = rssi_ready = 0U;
		devmatch_id = irkmatch_id = 0xFF;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	lll_aux = param;
	aux = HDR_LLL2ULL(lll_aux);
	scan = HDR_LLL2ULL(aux->rx_head->rx_ftr.param);
	lll = &scan->lll;

	/* No Rx */
	if (!trx_done) {
		/* TODO: Combine the early exit with above if-then-else block
		 */
		goto isr_rx_do_close;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	rl_idx = devmatch_ok ?
		 ull_filter_lll_rl_idx(!!(lll->filter_policy & 0x01),
				       devmatch_id) :
		 irkmatch_ok ? ull_filter_lll_rl_irk_idx(irkmatch_id) :
		 FILTER_IDX_NONE;
#else
	rl_idx = FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	if (crc_ok) {
		int err;

		err = isr_rx_pdu(lll_aux, devmatch_ok, devmatch_id,
				 irkmatch_ok, irkmatch_ok, rl_idx, rssi_ready);
		if (!err) {
			if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
				lll_prof_send();
			}

			return;
		}
	}

isr_rx_do_close:
	radio_isr_set(isr_done, lll_aux);
	radio_disable();
}

static int isr_rx_pdu(struct lll_scan_aux *lll, uint8_t devmatch_ok,
		      uint8_t devmatch_id, uint8_t irkmatch_ok,
		      uint8_t irkmatch_id, uint8_t rl_idx, uint8_t rssi_ready)
{
	struct ll_scan_aux_set *aux;
	struct ll_scan_set *scan;
	struct node_rx_pdu *node_rx;
	struct node_rx_ftr *ftr;
	struct lll_scan *lll_scan;
	struct pdu_adv *pdu;

	node_rx = ull_pdu_rx_alloc_peek(3);
	if (!node_rx) {
		return -ENOBUFS;
	}

	pdu = (void *)node_rx->pdu;
	if ((pdu->type != PDU_ADV_TYPE_EXT_IND) || !pdu->len) {
		return -EINVAL;
	}

	aux = HDR_LLL2ULL(lll);
	scan = HDR_LLL2ULL(aux->rx_head->rx_ftr.param);
	lll_scan = &scan->lll;

	if (0) {
#if defined(CONFIG_BT_CENTRAL)
	/* Initiator */
	} else if (lll_scan->conn && !lll_scan->conn->master.cancelled &&
		   (pdu->adv_ext_ind.adv_mode & BT_HCI_LE_ADV_PROP_CONN) &&
		   lll_scan_ext_tgta_check(lll_scan, false, true, pdu,
					   rl_idx)) {
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

		/* Always use CSA#2 on secondary channel, we need 2 nodes for conn
		 * and CSA#2 events and 2 nodes are always reserved for connection.
		 */
		rx = ull_pdu_rx_alloc_peek(4);

		if (!rx) {
			return -ENOBUFS;
		}

		pdu_end_us = radio_tmr_end_get();
		if (!lll_scan->ticks_window) {
			uint32_t scan_interval_us;

			/* FIXME: is this correct for continuous scanning? */
			scan_interval_us = lll_scan->interval * SCAN_INT_UNIT_US;
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

		ull = HDR_LLL2ULL(lll_scan);
		if (pdu_end_us > (HAL_TICKER_TICKS_TO_US(ull->ticks_slot) -
				  EVENT_IFS_US -
				  PKT_AC_US(aux_connect_req_len, lll->phy) -
				  EVENT_IFS_US -
				  PKT_AC_US(aux_connect_rsp_len, lll->phy) -
				  EVENT_OVERHEAD_START_US -
				  EVENT_TICKER_RES_MARGIN_US)) {
			return -ETIME;
		}

#if defined(CONFIG_BT_CTLR_PRIVACY)
		lrpa = ull_filter_lll_lrpa_get(rl_idx);
		if (lll_scan->rpa_gen && lrpa) {
			init_tx_addr = 1;
			init_addr = lrpa->val;
		} else {
#else
		if (1) {
#endif
			init_tx_addr = lll_scan->init_addr_type;
			init_addr = lll_scan->init_addr;
		}

		pdu_tx = (void *)lll_scan_connect_req_pdu;

		lll_scan_prepare_connect_req(lll_scan, pdu_tx, lll->phy,
					     pdu->tx_addr,
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
		radio_switch_complete_and_rx(lll->phy);
		radio_isr_set(isr_tx_connect_req, lll);

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
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
					 radio_rx_chain_delay_get(lll->phy, 1) -
					 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		if (rssi_ready) {
			lll_scan->conn->rssi_latest =  radio_rssi_get();
		}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

		/* block CPU so that there is no CRC error on pdu tx,
		 * this is only needed if we want the CPU to sleep.
		 * while(!radio_has_disabled())
		 * {cpu_sleep();}
		 * radio_status_reset();
		 */

		/* Stop further connection initiation */
		lll_scan->conn->master.initiated = 1U;

		/* Stop further initiating events */
		lll_scan->is_stop = 1U;

		rx = ull_pdu_rx_alloc();

		rx->hdr.type = NODE_RX_TYPE_CONNECTION;
		rx->hdr.handle = 0xffff;

		memcpy(rx->pdu, pdu_tx, (offsetof(struct pdu_adv, connect_ind) +
					 sizeof(struct pdu_adv_connect_ind)));

		/* ChSel is RFU in AUX_ADV_IND but we do need to use CSA#2 for
		 * connections initiated on the secondary advertising channel
		 * thus overwrise chan_sel to make it work seamlessly.
		 */
		pdu = (void *)rx->pdu;
		pdu->chan_sel = 1;

		ftr = &(rx->hdr.rx_ftr);

		ftr->param = lll_scan;
		ftr->ticks_anchor = radio_tmr_start_get();
		ftr->radio_end_us = conn_space_us -
				    radio_rx_chain_delay_get(lll->phy, 1);

#if defined(CONFIG_BT_CTLR_PRIVACY)
		ftr->rl_idx = irkmatch_ok ? rl_idx : FILTER_IDX_NONE;
		ftr->lrpa_used = lll_scan->rpa_gen && lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		ftr->extra = ull_pdu_rx_alloc();

		lll->node_conn_rx = rx;

		return 0;

	} else if (!lll_scan->conn &&
		   lll_scan_ext_tgta_check(lll_scan, false, false, pdu,
					   rl_idx)) {
#else /* !CONFIG_BT_CENTRAL */
	} else if (lll_scan_ext_tgta_check(lll_scan, false, false, pdu,
					   rl_idx)) {
#endif /* !CONFIG_BT_CENTRAL */
		ull_pdu_rx_alloc();

		trx_cnt++;

		node_rx->hdr.type = NODE_RX_TYPE_EXT_AUX_REPORT;

		ftr = &(node_rx->hdr.rx_ftr);
		ftr->param = lll;
		ftr->ticks_anchor = radio_tmr_start_get();
		ftr->radio_end_us = radio_tmr_end_get() -
				    radio_rx_chain_delay_get(lll->phy, 1);

		ftr->rssi = (rssi_ready) ? radio_rssi_get() :
			    BT_HCI_LE_RSSI_NOT_AVAILABLE;

#if defined(CONFIG_BT_CTLR_PRIVACY)
		ftr->rl_idx = irkmatch_ok ? rl_idx : FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		ull_rx_put(node_rx->hdr.link, node_rx);
		ull_rx_sched();
	}

	return -ECANCELED;
}

#if defined(CONFIG_BT_CENTRAL)
static void isr_tx_connect_req(void *param)
{
	struct lll_scan_aux *lll_aux;
	uint32_t hcto;

	/* Clear radio tx status and events */
	lll_isr_tx_status_reset();

	lll_aux = param;

	radio_pkt_rx_set(radio_pkt_scratch_get());

	/* assert if radio packet ptr is not set and radio started rx */
	LL_ASSERT(!radio_is_ready());

	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_disable();
	radio_isr_set(isr_rx_connect_rsp, param);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_enabled()) {
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

		radio_ar_configure(count, irks, (lll_aux->phy << 2) | BIT(1));
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	/* +/- 2us active clock jitter, +1 us hcto compensation */
	hcto = radio_tmr_tifs_base_get() + EVENT_IFS_US + 4 + 1;
	hcto += radio_rx_chain_delay_get(lll_aux->phy, 1);
	hcto += addr_us_get(lll_aux->phy);
	hcto -= radio_tx_chain_delay_get(lll_aux->phy, 1);
	radio_tmr_hcto_configure(hcto);

	if (IS_ENABLED(CONFIG_BT_CTLR_SCAN_REQ_RSSI) ||
	    IS_ENABLED(CONFIG_BT_CTLR_CONN_RSSI)) {
		radio_rssi_measure();
	}

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* PA/LNA enable is overwriting packet end used in ISR
		 * profiling, hence back it up for later use.
		 */
		lll_prof_radio_end_backup();
	}

	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() + EVENT_IFS_US - 4 -
				 radio_tx_chain_delay_get(lll_aux->phy, 1) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* NOTE: as scratch packet is used to receive, it is safe to
		 * generate profile event using rx nodes.
		 */
		lll_prof_send();
	}
}

static void isr_rx_connect_rsp(void *param)
{
	struct lll_scan_aux *lll_aux;
	struct ll_scan_aux_set *aux;
	struct ll_scan_set *scan;
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
		irkmatch_ok = radio_ar_has_match();
		irkmatch_id = radio_ar_match_get();
	} else {
		crc_ok = irkmatch_ok = 0U;
		irkmatch_id = 0xFF;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* Get the reference to primary scanner's LLL context */
	lll_aux = param;
	aux = HDR_LLL2ULL(lll_aux);
	scan = HDR_LLL2ULL(aux->rx_head->rx_ftr.param);
	lll = &scan->lll;

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
		pdu_rx = radio_pkt_scratch_get();
		trx_done = isr_scan_connect_rsp_check(lll,
				(void *)lll_scan_connect_req_pdu, pdu_rx,
				rl_idx);
	} else {
		trx_done = 0U;
	}

	/* No Rx or invalid PDU received */
	if (!trx_done) {
		struct node_rx_ftr *ftr;

		/* Try again with connection initiation */
		lll->conn->master.initiated = 0U;

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
	conn_lll->max_tx_time =
		MAX(conn_lll->max_tx_time,
		    PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, lll_aux->phy));
	conn_lll->max_rx_time =
		MAX(conn_lll->max_rx_time,
		    PKT_US(PDU_DC_PAYLOAD_SIZE_MIN, lll_aux->phy));
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
		memcpy(pdu->connect_ind.adv_addr,
		       &pdu_rx->adv_ext_ind.ext_hdr.data[ADVA_OFFSET],
		       BDADDR_SIZE);
		ftr = &(rx->hdr.rx_ftr);
		ftr->rl_idx = rl_idx;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

isr_rx_do_close:
	ull_rx_put(rx->hdr.link, rx);
	ull_rx_sched();

	radio_isr_set(isr_done, lll_aux);
	radio_disable();
}

bool isr_scan_connect_rsp_check(struct lll_scan *lll, struct pdu_adv *pdu_tx,
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

	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SCAN_AUX;

	lll_isr_early_abort(param);
}
#endif /* CONFIG_BT_CENTRAL */
