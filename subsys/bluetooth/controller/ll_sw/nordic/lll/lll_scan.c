/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <toolchain.h>

#include <soc.h>

#include <sys/byteorder.h>
#include <bluetooth/hci.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_scan.h"
#include "lll_conn.h"
#include "lll_chan.h"
#include "lll_filter.h"
#include "lll_sched.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_scan
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static int is_abort_cb(void *next, int prio, void *curr,
		       lll_prepare_cb_t *resume_cb, int *resume_prio);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void ticker_stop_cb(uint32_t ticks_at_expire, uint32_t remainder, uint16_t lazy,
			   void *param);
static void ticker_op_start_cb(uint32_t status, void *param);
static void isr_rx(void *param);
static void isr_tx(void *param);
static void isr_done(void *param);
static void isr_window(void *param);
static void isr_abort(void *param);
static void isr_done_cleanup(void *param);
static void isr_cleanup(void *param);

static inline bool isr_rx_scan_check(struct lll_scan *lll, uint8_t irkmatch_ok,
				     uint8_t devmatch_ok, uint8_t rl_idx);
static inline int isr_rx_pdu(struct lll_scan *lll, struct pdu_adv *pdu_adv_rx,
			     uint8_t devmatch_ok, uint8_t devmatch_id,
			     uint8_t irkmatch_ok, uint8_t irkmatch_id,
			     uint8_t rl_idx, uint8_t rssi_ready);
static inline bool isr_scan_init_check(struct lll_scan *lll,
				       struct pdu_adv *pdu, uint8_t rl_idx);
static inline bool isr_scan_init_adva_check(struct lll_scan *lll,
					    struct pdu_adv *pdu, uint8_t rl_idx);
static inline bool isr_scan_tgta_check(struct lll_scan *lll, bool init,
				       struct pdu_adv *pdu, uint8_t rl_idx,
				       bool *dir_report);
static inline bool isr_scan_tgta_rpa_check(struct lll_scan *lll,
					   struct pdu_adv *pdu,
					   bool *dir_report);
static inline bool isr_scan_rsp_adva_matches(struct pdu_adv *srsp);
static int isr_rx_scan_report(struct lll_scan *lll, uint8_t rssi_ready,
			      uint8_t rl_idx, bool dir_report);

int lll_scan_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_scan_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_scan_prepare(void *param)
{
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	err = lll_prepare(is_abort_cb, abort_cb, prepare_cb, 0, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	uint32_t ticks_at_event, ticks_at_start;
	struct node_rx_pdu *node_rx;
	uint32_t remainder_us;
	struct lll_scan *lll;
	struct evt_hdr *evt;
	uint32_t remainder;
	uint32_t aa;

	DEBUG_RADIO_START_O(1);

	lll = p->param;

	/* Check if stopped (on connection establishment race between LLL and
	 * ULL.
	 */
	if (unlikely(lll_is_stop(lll))) {
		int err;

		err = lll_hfclock_off();
		LL_ASSERT(err >= 0);

		lll_done(NULL);

		DEBUG_RADIO_CLOSE_O(0);
		return 0;
	}

	/* Initialize scanning state */
	lll->state = 0U;

	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* TODO: if coded we use S8? */
	radio_phy_set(lll->phy, 1);
	radio_pkt_configure(8, PDU_AC_LEG_PAYLOAD_SIZE_MAX, (lll->phy << 1));

	lll->is_adv_ind = 0U;
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	radio_phy_set(0, 0);
	radio_pkt_configure(8, PDU_AC_LEG_PAYLOAD_SIZE_MAX, 0);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_pkt_rx_set(node_rx->pdu);

	aa = sys_cpu_to_le32(PDU_AC_ACCESS_ADDR);
	radio_aa_set((uint8_t *)&aa);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    0x555555);

	lll_chan_set(37 + lll->chan);

	radio_isr_set(isr_rx, lll);

	/* setup tIFS switching */
	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_tx(0, 0, 0, 0);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_enabled()) {
		struct lll_filter *filter =
			ull_filter_lll_get(!!(lll->filter_policy & 0x1));
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

		radio_filter_configure(filter->enable_bitmask,
				       filter->addr_type_bitmask,
				       (uint8_t *)filter->bdaddr);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		radio_ar_configure(count, irks, (lll->phy << 2));
#else
		radio_ar_configure(count, irks, 0);
#endif
	} else
#endif /* CONFIG_BT_CTLR_PRIVACY */

	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER) && lll->filter_policy) {
		/* Setup Radio Filter */
		struct lll_filter *wl = ull_filter_lll_get(true);

		radio_filter_configure(wl->enable_bitmask,
				       wl->addr_type_bitmask,
				       (uint8_t *)wl->bdaddr);
	}

	ticks_at_event = p->ticks_at_expire;
	evt = HDR_LLL2EVT(lll);
	ticks_at_event += lll_evt_offset_get(evt);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0, ticks_at_start, remainder);

	/* capture end of Rx-ed PDU, for initiator to calculate first
	 * master event or extended scan to schedule auxiliary channel
	 * reception.
	 */
	radio_tmr_end_capture();

	/* scanner always measures RSSI */
	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */
	ARG_UNUSED(remainder_us);
#endif /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	if (lll_preempt_calc(evt, (TICKER_ID_SCAN_BASE +
				   ull_scan_lll_handle_get(lll)),
			     ticks_at_event)) {
		radio_isr_set(isr_abort, lll);
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		uint32_t ret;

		if (lll->ticks_window) {
			/* start window close timeout */
			ret = ticker_start(TICKER_INSTANCE_ID_CTLR,
					   TICKER_USER_ID_LLL,
					   TICKER_ID_SCAN_STOP,
					   ticks_at_event, lll->ticks_window,
					   TICKER_NULL_PERIOD,
					   TICKER_NULL_REMAINDER,
					   TICKER_NULL_LAZY, TICKER_NULL_SLOT,
					   ticker_stop_cb, lll,
					   ticker_op_start_cb,
					   (void *)__LINE__);
			LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
				  (ret == TICKER_STATUS_BUSY));
		}

#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
		/* calc next group in us for the anchor where first connection
		 * event to be placed.
		 */
		if (lll->conn) {
			static memq_link_t link;
			static struct mayfly mfy_after_mstr_offset_get = {
				0, 0, &link, NULL,
				ull_sched_mfy_after_mstr_offset_get};
			uint32_t retval;

			mfy_after_mstr_offset_get.param = p;

			retval = mayfly_enqueue(TICKER_USER_ID_LLL,
						TICKER_USER_ID_ULL_LOW, 1,
						&mfy_after_mstr_offset_get);
			LL_ASSERT(!retval);
		}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_CTLR_SCHED_ADVANCED */

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	DEBUG_RADIO_START_O(1);

	return 0;
}

static int resume_prepare_cb(struct lll_prepare_param *p)
{
	struct evt_hdr *evt;

	evt = HDR_LLL2EVT(p->param);
	p->ticks_at_expire = ticker_ticks_now_get() - lll_evt_offset_get(evt);
	p->remainder = 0;
	p->lazy = 0;

	return prepare_cb(p);
}

static int is_abort_cb(void *next, int prio, void *curr,
		       lll_prepare_cb_t *resume_cb, int *resume_prio)
{
	struct lll_scan *lll = curr;

	/* TODO: check prio */
	if (next != curr) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		if (unlikely(!lll->duration_reload || lll->duration_expire))
#endif /* CONFIG_BT_CTLR_ADV_EXT */
		{
			int err;

			/* wrap back after the pre-empter */
			*resume_cb = resume_prepare_cb;
			*resume_prio = 0; /* TODO: */

			/* Retain HF clock */
			err = lll_hfclock_on();
			LL_ASSERT(err >= 0);

			return -EAGAIN;
		}
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (unlikely(lll->duration_reload && !lll->duration_expire)) {
		radio_isr_set(isr_done_cleanup, lll);
	} else
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	{
		radio_isr_set(isr_window, lll);
	}

	radio_disable();

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
		if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) && lll_is_stop(param)) {
			while (!radio_has_disabled()) {
				cpu_sleep();
			}
		} else {
			radio_isr_set(isr_abort, param);
			radio_disable();
		}
		return;
	}

	/* NOTE: Else clean the top half preparations of the aborted event
	 * currently in preparation pipeline.
	 */
	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	lll_done(param);
}

static void ticker_stop_cb(uint32_t ticks_at_expire, uint32_t remainder, uint16_t lazy,
			   void *param)
{
	radio_isr_set(isr_done_cleanup, param);
	radio_disable();
}

static void ticker_op_start_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

static void isr_rx(void *param)
{
	struct lll_scan *lll;
	struct node_rx_pdu *node_rx;
	struct pdu_adv *pdu_adv_rx;
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

	/* Clear radio status and events */
	lll_isr_status_reset();

	lll = param;

	/* No Rx */
	if (!trx_done) {
		goto isr_rx_do_close;
	}

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	pdu_adv_rx = (void *)node_rx->pdu;

#if defined(CONFIG_BT_CTLR_PRIVACY)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (!irkmatch_ok && (pdu_adv_rx->type == PDU_ADV_TYPE_EXT_IND) &&
	    (pdu_adv_rx->tx_addr)) {
		radio_ar_resolve(&pdu_adv_rx->payload[2]);
		irkmatch_ok = radio_ar_has_match();
		irkmatch_id = radio_ar_match_get();
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	rl_idx = devmatch_ok ?
		 ull_filter_lll_rl_idx(!!(lll->filter_policy & 0x01),
				       devmatch_id) :
		 irkmatch_ok ? ull_filter_lll_rl_irk_idx(irkmatch_id) :
			       FILTER_IDX_NONE;
#else
	rl_idx = FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */
	if (crc_ok && isr_rx_scan_check(lll, irkmatch_ok, devmatch_ok,
					rl_idx)) {
		int err;

		err = isr_rx_pdu(lll, pdu_adv_rx, devmatch_ok, devmatch_id,
				 irkmatch_ok, irkmatch_id, rl_idx, rssi_ready);
		if (!err) {
			if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
				lll_prof_send();
			}

			return;
		}
	}

isr_rx_do_close:
	radio_isr_set(isr_done, lll);
	radio_disable();
}

static void isr_tx(void *param)
{
#if defined(CONFIG_BT_CTLR_PRIVACY) && defined(CONFIG_BT_CTLR_ADV_EXT)
	struct lll_scan *lll = param;
#endif
	struct node_rx_pdu *node_rx;
	uint32_t hcto;

	/* Clear radio status and events */
	lll_isr_tx_status_reset();

	/* setup tIFS switching */
	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_tx(0, 0, 0, 0);

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);
	radio_pkt_rx_set(node_rx->pdu);

	/* assert if radio packet ptr is not set and radio started rx */
	LL_ASSERT(!radio_is_ready());

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_enabled()) {
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		radio_ar_configure(count, irks, (lll->phy << 2));
#else
		radio_ar_configure(count, irks, 0);
#endif
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	/* +/- 2us active clock jitter, +1 us hcto compensation */
	hcto = radio_tmr_tifs_base_get() + EVENT_IFS_US + 4 + 1;
	hcto += radio_rx_chain_delay_get(0, 0);
	hcto += addr_us_get(0);
	hcto -= radio_tx_chain_delay_get(0, 0);

	radio_tmr_hcto_configure(hcto);

	radio_rssi_measure();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() + EVENT_IFS_US - 4 -
				 radio_tx_chain_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

	radio_isr_set(isr_rx, param);
}

static void isr_common_done(void *param)
{
	struct node_rx_pdu *node_rx;
	struct lll_scan *lll;

	/* Clear radio status and events */
	lll_isr_status_reset();

	/* Reset scanning state */
	lll = param;
	lll->state = 0U;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	lll->is_adv_ind = 0U;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	/* setup tIFS switching */
	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_tx(0, 0, 0, 0);

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);
	radio_pkt_rx_set(node_rx->pdu);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_enabled()) {
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		radio_ar_configure(count, irks, (lll->phy << 2));
#else
		ARG_UNUSED(lll);
		radio_ar_configure(count, irks, 0);
#endif
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	radio_rssi_measure();

	radio_isr_set(isr_rx, param);
}

static void isr_done(void *param)
{
	isr_common_done(param);

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	uint32_t start_us = radio_tmr_start_now(0);

	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */
	radio_rx_enable();
#endif /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */

	/* capture end of Rx-ed PDU, for initiator to calculate first
	 * master event.
	 */
	radio_tmr_end_capture();
}

static void isr_window(void *param)
{
	uint32_t ticks_at_start;
	uint32_t remainder_us;
	struct lll_scan *lll;

	isr_common_done(param);

	lll = param;

	/* Next radio channel to scan, round-robin 37, 38, and 39. */
	if (++lll->chan == 3U) {
		lll->chan = 0U;
	}
	lll_chan_set(37 + lll->chan);

#if defined(CONFIG_BT_CENTRAL)
	bool is_sched_advanced = IS_ENABLED(CONFIG_BT_CTLR_SCHED_ADVANCED) &&
				 lll->conn && lll->conn_win_offset_us;
	uint32_t ticks_anchor_prev;

	if (is_sched_advanced) {
		/* Get the ticks_anchor when the offset to free time space for
		 * a new central event was last calculated at the start of the
		 * initiator window. This can be either the previous full window
		 * start or remainder resume start of the continuous initiator
		 * after it was preempted.
		 */
		ticks_anchor_prev = radio_tmr_start_get();
	} else {
		ticks_anchor_prev = 0U;
	}
#endif /* CONFIG_BT_CENTRAL */

	ticks_at_start = ticker_ticks_now_get() +
			 HAL_TICKER_CNTR_CMP_OFFSET_MIN;
	remainder_us = radio_tmr_start_tick(0, ticks_at_start);

	/* capture end of Rx-ed PDU, for initiator to calculate first
	 * master event.
	 */
	radio_tmr_end_capture();

#if defined(CONFIG_BT_CTLR_GPIO_LNA_PIN)
	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#else /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */
	ARG_UNUSED(remainder_us);
#endif /* !CONFIG_BT_CTLR_GPIO_LNA_PIN */

#if defined(CONFIG_BT_CENTRAL)
	if (is_sched_advanced) {
		uint32_t ticks_anchor_new, ticks_delta, ticks_delta_us;

		/* Calculation to reduce the conn_win_offset_us, as a new
		 * window is started here and the reference ticks_anchor is
		 * now at the start of this new window.
		 */
		ticks_anchor_new = radio_tmr_start_get();
		ticks_delta = ticker_ticks_diff_get(ticks_anchor_new,
						    ticks_anchor_prev);
		ticks_delta_us = HAL_TICKER_TICKS_TO_US(ticks_delta);

		/* Underflow is accepted, as it will be corrected at the time of
		 * connection establishment by incrementing it in connection
		 * interval units until it is in the future.
		 */
		lll->conn_win_offset_us -= ticks_delta_us;
	}
#endif /* CONFIG_BT_CENTRAL */
}

static void isr_abort(void *param)
{
	/* Clear radio status and events */
	lll_isr_status_reset();

	/* Scanner stop can expire while here in this ISR.
	 * Deferred attempt to stop can fail as it would have
	 * expired, hence ignore failure.
	 */
	ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_LLL,
		    TICKER_ID_SCAN_STOP, NULL, NULL);

	/* Under race conditions, radio could get started while entering ISR */
	radio_disable();

	isr_cleanup(param);
}

static void isr_done_cleanup(void *param)
{
	if (lll_is_done(param)) {
		return;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	struct event_done_extra *extra;

	extra = ull_event_done_extra_get();
	LL_ASSERT(extra);

	extra->type = EVENT_DONE_EXTRA_TYPE_SCAN;
#endif  /* CONFIG_BT_CTLR_ADV_EXT */

	isr_cleanup(param);
}

static void isr_cleanup(void *param)
{
	struct lll_scan *lll;

	radio_filter_disable();

	lll = param;
	if (++lll->chan == 3U) {
		lll->chan = 0U;
	}

#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (_radio.advertiser.is_enabled && _radio.advertiser.is_mesh &&
	    !_radio.advertiser.retry) {
		mayfly_mesh_stop(NULL);
	}
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
	struct node_rx_hdr *node_rx = ull_pdu_rx_alloc_peek(3);

	if (node_rx) {
		ull_pdu_rx_alloc();

		/* TODO: add other info by defining a payload struct */
		node_rx->type = NODE_RX_TYPE_SCAN_INDICATION;

		ull_rx_put(node_rx->link, node_rx);
		ull_rx_sched();
	}
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

	lll_isr_cleanup(param);
}

static inline bool isr_rx_scan_check(struct lll_scan *lll, uint8_t irkmatch_ok,
				     uint8_t devmatch_ok, uint8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	return (((lll->filter_policy & 0x01) == 0) &&
		 (!devmatch_ok || ull_filter_lll_rl_idx_allowed(irkmatch_ok,
								rl_idx))) ||
		(((lll->filter_policy & 0x01) != 0) &&
		 (devmatch_ok || ull_filter_lll_irk_whitelisted(rl_idx)));
#else
	return ((lll->filter_policy & 0x01) == 0U) ||
		devmatch_ok;
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

static inline int isr_rx_pdu(struct lll_scan *lll, struct pdu_adv *pdu_adv_rx,
			     uint8_t devmatch_ok, uint8_t devmatch_id,
			     uint8_t irkmatch_ok, uint8_t irkmatch_id,
			     uint8_t rl_idx, uint8_t rssi_ready)
{
	bool dir_report = false;

	if (0) {
#if defined(CONFIG_BT_CENTRAL)
	/* Initiator */
	} else if ((lll->conn) &&
		   isr_scan_init_check(lll, pdu_adv_rx, rl_idx)) {
		struct lll_conn *lll_conn;
		struct node_rx_ftr *ftr;
		struct node_rx_pdu *rx;
		struct pdu_adv *pdu_tx;
		uint32_t conn_interval_us;
		uint32_t conn_offset_us;
		uint32_t conn_space_us;
		struct evt_hdr *evt;
		uint32_t pdu_end_us;
#if defined(CONFIG_BT_CTLR_PRIVACY)
		bt_addr_t *lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */
		int ret;

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			rx = ull_pdu_rx_alloc_peek(4);
		} else {
			rx = ull_pdu_rx_alloc_peek(3);
		}

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
		evt = HDR_LLL2EVT(lll);
		if (pdu_end_us > (HAL_TICKER_TICKS_TO_US(evt->ticks_slot) -
				  EVENT_IFS_US - 352 - EVENT_OVERHEAD_START_US -
				  EVENT_TICKER_RES_MARGIN_US)) {
			return -ETIME;
		}

		radio_switch_complete_and_disable();

		/* Acquire the connection context */
		lll_conn = lll->conn;

		/* Tx the connect request packet */
		pdu_tx = (void *)radio_pkt_scratch_get();
		pdu_tx->type = PDU_ADV_TYPE_CONNECT_IND;

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			pdu_tx->chan_sel = 1;
		} else {
			pdu_tx->chan_sel = 0;
		}

		pdu_tx->rx_addr = pdu_adv_rx->tx_addr;
		pdu_tx->len = sizeof(struct pdu_adv_connect_ind);
#if defined(CONFIG_BT_CTLR_PRIVACY)
		lrpa = ull_filter_lll_lrpa_get(rl_idx);
		if (lll->rpa_gen && lrpa) {
			pdu_tx->tx_addr = 1;
			memcpy(&pdu_tx->connect_ind.init_addr[0], lrpa->val,
			       BDADDR_SIZE);
		} else {
#else
		if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
			pdu_tx->tx_addr = lll->init_addr_type;
			memcpy(&pdu_tx->connect_ind.init_addr[0],
			       &lll->init_addr[0], BDADDR_SIZE);
		}
		memcpy(&pdu_tx->connect_ind.adv_addr[0],
		       &pdu_adv_rx->adv_ind.addr[0], BDADDR_SIZE);
		memcpy(&pdu_tx->connect_ind.access_addr[0],
		       &lll_conn->access_addr[0], 4);
		memcpy(&pdu_tx->connect_ind.crc_init[0],
		       &lll_conn->crc_init[0], 3);
		pdu_tx->connect_ind.win_size = 1;

		conn_interval_us = (uint32_t)lll_conn->interval *
			CONN_INT_UNIT_US;
		conn_offset_us = radio_tmr_end_get() + 502 +
			CONN_INT_UNIT_US;

		if (!IS_ENABLED(CONFIG_BT_CTLR_SCHED_ADVANCED) ||
		    lll->conn_win_offset_us == 0U) {
			conn_space_us = conn_offset_us;
			pdu_tx->connect_ind.win_offset = sys_cpu_to_le16(0);
		} else {
			conn_space_us = lll->conn_win_offset_us;
			while ((conn_space_us & ((uint32_t)1 << 31)) ||
			       (conn_space_us < conn_offset_us)) {
				conn_space_us += conn_interval_us;
			}
			pdu_tx->connect_ind.win_offset =
				sys_cpu_to_le16((conn_space_us -
						 conn_offset_us) /
				CONN_INT_UNIT_US);
			pdu_tx->connect_ind.win_size++;
		}

		pdu_tx->connect_ind.interval =
			sys_cpu_to_le16(lll_conn->interval);
		pdu_tx->connect_ind.latency =
			sys_cpu_to_le16(lll_conn->latency);
		pdu_tx->connect_ind.timeout =
			sys_cpu_to_le16(lll->conn_timeout);
		memcpy(&pdu_tx->connect_ind.chan_map[0],
		       &lll_conn->data_chan_map[0],
		       sizeof(pdu_tx->connect_ind.chan_map));
		pdu_tx->connect_ind.hop = lll_conn->data_chan_hop;
		pdu_tx->connect_ind.sca = lll_clock_sca_local_get();

		radio_pkt_tx_set(pdu_tx);

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();
		}

		radio_isr_set(isr_done_cleanup, lll);

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
					 radio_rx_chain_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		if (rssi_ready) {
			lll_conn->rssi_latest =  radio_rssi_get();
		}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

		/* block CPU so that there is no CRC error on pdu tx,
		 * this is only needed if we want the CPU to sleep.
		 * while(!radio_has_disabled())
		 * {cpu_sleep();}
		 * radio_status_reset();
		 */

		/* Stop further LLL radio events */
		ret = lll_stop(lll);
		LL_ASSERT(!ret);

		rx = ull_pdu_rx_alloc();

		rx->hdr.type = NODE_RX_TYPE_CONNECTION;
		rx->hdr.handle = 0xffff;

		uint8_t pdu_adv_rx_chan_sel = pdu_adv_rx->chan_sel;
		memcpy(rx->pdu, pdu_tx, (offsetof(struct pdu_adv, connect_ind) +
					  sizeof(struct pdu_adv_connect_ind)));

		/* Overwrite the sent chan sel with received chan sel, when
		 * giving this PDU to the higher layer. */
		pdu_adv_rx = (void *)rx->pdu;
		pdu_adv_rx->chan_sel = pdu_adv_rx_chan_sel;

		ftr = &(rx->hdr.rx_ftr);

		ftr->param = lll;
		ftr->ticks_anchor = radio_tmr_start_get();
		ftr->radio_end_us = conn_space_us -
				    radio_tx_chain_delay_get(0, 0);

#if defined(CONFIG_BT_CTLR_PRIVACY)
		ftr->rl_idx = irkmatch_ok ? rl_idx : FILTER_IDX_NONE;
		ftr->lrpa_used = lll->rpa_gen && lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			ftr->extra = ull_pdu_rx_alloc();
		}

		ull_rx_put(rx->hdr.link, rx);
		ull_rx_sched();

		return 0;
#endif /* CONFIG_BT_CENTRAL */

	/* Active scanner */
	} else if (((pdu_adv_rx->type == PDU_ADV_TYPE_ADV_IND) ||
		    (pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_IND)) &&
		   (pdu_adv_rx->len <= sizeof(struct pdu_adv_adv_ind)) &&
		   lll->type &&
#if defined(CONFIG_BT_CENTRAL)
		   !lll->conn) {
#else /* !CONFIG_BT_CENTRAL */
		   1) {
#endif /* !CONFIG_BT_CENTRAL */
		struct pdu_adv *pdu_tx;
#if defined(CONFIG_BT_CTLR_PRIVACY)
		bt_addr_t *lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */
		int err;

		/* setup tIFS switching */
		radio_tmr_tifs_set(EVENT_IFS_US);
		radio_switch_complete_and_rx(0);

		/* save the adv packet */
		err = isr_rx_scan_report(lll, rssi_ready,
					 irkmatch_ok ? rl_idx : FILTER_IDX_NONE,
					 false);
		if (err) {
			return err;
		}

		/* prepare the scan request packet */
		pdu_tx = (void *)radio_pkt_scratch_get();
		pdu_tx->type = PDU_ADV_TYPE_SCAN_REQ;
		pdu_tx->rx_addr = pdu_adv_rx->tx_addr;
		pdu_tx->len = sizeof(struct pdu_adv_scan_req);
#if defined(CONFIG_BT_CTLR_PRIVACY)
		lrpa = ull_filter_lll_lrpa_get(rl_idx);
		if (lll->rpa_gen && lrpa) {
			pdu_tx->tx_addr = 1;
			memcpy(&pdu_tx->scan_req.scan_addr[0], lrpa->val,
			       BDADDR_SIZE);
		} else {
#else
		if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
			pdu_tx->tx_addr = lll->init_addr_type;
			memcpy(&pdu_tx->scan_req.scan_addr[0],
			       &lll->init_addr[0], BDADDR_SIZE);
		}
		memcpy(&pdu_tx->scan_req.adv_addr[0],
		       &pdu_adv_rx->adv_ind.addr[0], BDADDR_SIZE);

		radio_pkt_tx_set(pdu_tx);

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();

		}

		/* capture end of Tx-ed PDU, used to calculate HCTO. */
		radio_tmr_end_capture();

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
					 radio_rx_chain_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */

		/* switch scanner state to active */
		lll->state = 1U;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		if (pdu_adv_rx->type == PDU_ADV_TYPE_ADV_IND) {
			lll->is_adv_ind = 1U;
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

		radio_isr_set(isr_tx, lll);

		return 0;
	}
	/* Passive scanner or scan responses */
	else if (((((pdu_adv_rx->type == PDU_ADV_TYPE_ADV_IND) ||
		    (pdu_adv_rx->type == PDU_ADV_TYPE_NONCONN_IND) ||
		    (pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_IND)) &&
		   (pdu_adv_rx->len <= sizeof(struct pdu_adv_adv_ind))) ||
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_DIRECT_IND) &&
		   (pdu_adv_rx->len == sizeof(struct pdu_adv_direct_ind)) &&
		   (/* allow directed adv packets addressed to this device */
		    isr_scan_tgta_check(lll, false, pdu_adv_rx, rl_idx,
					&dir_report))) ||
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_EXT_IND) &&
		   (lll->phy)) ||
#endif /* CONFIG_BT_CTLR_ADV_EXT */
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_RSP) &&
		   (pdu_adv_rx->len <= sizeof(struct pdu_adv_scan_rsp)) &&
		   (lll->state != 0U) &&
		   isr_scan_rsp_adva_matches(pdu_adv_rx))) &&
		 (pdu_adv_rx->len != 0) &&
#if defined(CONFIG_BT_CENTRAL)
		   !lll->conn) {
#else /* !CONFIG_BT_CENTRAL */
		   1) {
#endif /* !CONFIG_BT_CENTRAL */
		uint32_t err;

		/* save the scan response packet */
		err = isr_rx_scan_report(lll, rssi_ready,
					 irkmatch_ok ? rl_idx :
						       FILTER_IDX_NONE,
					 dir_report);
		if (err) {
			return err;
		}
	}
	/* invalid PDU */
	else {
		/* ignore and close this rx/tx chain ( code below ) */
		return -EINVAL;
	}

	return -ECANCELED;
}

static inline bool isr_scan_init_check(struct lll_scan *lll,
				       struct pdu_adv *pdu, uint8_t rl_idx)
{
	return ((((lll->filter_policy & 0x01) != 0U) ||
		 isr_scan_init_adva_check(lll, pdu, rl_idx)) &&
		(((pdu->type == PDU_ADV_TYPE_ADV_IND) &&
		  (pdu->len <= sizeof(struct pdu_adv_adv_ind))) ||
		 ((pdu->type == PDU_ADV_TYPE_DIRECT_IND) &&
		  (pdu->len == sizeof(struct pdu_adv_direct_ind)) &&
		  (/* allow directed adv packets addressed to this device */
		   isr_scan_tgta_check(lll, true, pdu, rl_idx, NULL)))));
}

static inline bool isr_scan_init_adva_check(struct lll_scan *lll,
					    struct pdu_adv *pdu, uint8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* Only applies to initiator with no whitelist */
	if (rl_idx != FILTER_IDX_NONE) {
		return (rl_idx == lll->rl_idx);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */
	return ((lll->adv_addr_type == pdu->tx_addr) &&
		!memcmp(lll->adv_addr, &pdu->adv_ind.addr[0], BDADDR_SIZE));
}

static inline bool isr_scan_tgta_check(struct lll_scan *lll, bool init,
				       struct pdu_adv *pdu, uint8_t rl_idx,
				       bool *dir_report)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_addr_resolve(pdu->rx_addr,
					   pdu->direct_ind.tgt_addr, rl_idx)) {
		return true;
	} else if (init && lll->rpa_gen &&
		   ull_filter_lll_lrpa_get(rl_idx)) {
		/* Initiator generating RPAs, and could not resolve TargetA:
		 * discard
		 */
		return false;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	return (((lll->init_addr_type == pdu->rx_addr) &&
		!memcmp(lll->init_addr, pdu->direct_ind.tgt_addr,
			BDADDR_SIZE))) ||
		  /* allow directed adv packets where TargetA address
		   * is resolvable private address (scanner only)
		   */
	       isr_scan_tgta_rpa_check(lll, pdu, dir_report);
}

static inline bool isr_scan_tgta_rpa_check(struct lll_scan *lll,
					   struct pdu_adv *pdu,
					   bool *dir_report)
{
	if (((lll->filter_policy & 0x02) != 0U) &&
	    (pdu->rx_addr != 0) &&
	    ((pdu->direct_ind.tgt_addr[5] & 0xc0) == 0x40)) {

		if (dir_report) {
			*dir_report = true;
		}

		return true;
	}

	return false;
}

static inline bool isr_scan_rsp_adva_matches(struct pdu_adv *srsp)
{
	struct pdu_adv *sreq = (void *)radio_pkt_scratch_get();

	return ((sreq->rx_addr == srsp->tx_addr) &&
		(memcmp(&sreq->scan_req.adv_addr[0],
			&srsp->scan_rsp.addr[0], BDADDR_SIZE) == 0));
}

static int isr_rx_scan_report(struct lll_scan *lll, uint8_t rssi_ready,
				uint8_t rl_idx, bool dir_report)
{
	struct node_rx_pdu *node_rx;

	node_rx = ull_pdu_rx_alloc_peek(3);
	if (!node_rx) {
		return -ENOBUFS;
	}
	ull_pdu_rx_alloc();

	/* Prepare the report (adv or scan resp) */
	node_rx->hdr.handle = 0xffff;
	if (0) {

#if defined(CONFIG_BT_HCI_MESH_EXT)
	} else if (_radio.advertiser.is_enabled &&
		   _radio.advertiser.is_mesh) {
		node_rx->hdr.type = NODE_RX_TYPE_MESH_REPORT;
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (lll->phy) {
		struct pdu_adv *pdu_adv_rx;

		switch (lll->phy) {
		case PHY_1M:
			node_rx->hdr.type = NODE_RX_TYPE_EXT_1M_REPORT;
			break;

		case PHY_CODED:
			node_rx->hdr.type = NODE_RX_TYPE_EXT_CODED_REPORT;
			break;

		default:
			LL_ASSERT(0);
			break;
		}

		pdu_adv_rx = (void *)node_rx->pdu;
		switch (pdu_adv_rx->type) {
		case PDU_ADV_TYPE_SCAN_RSP:
			if (lll->is_adv_ind) {
				pdu_adv_rx->type =
					PDU_ADV_TYPE_ADV_IND_SCAN_RSP;
			}
			break;

		case PDU_ADV_TYPE_EXT_IND:
			{
				struct node_rx_ftr *ftr;

				ftr = &(node_rx->hdr.rx_ftr);
				ftr->param = lll;
				ftr->ticks_anchor = radio_tmr_start_get();
				ftr->radio_end_us =
					radio_tmr_end_get() -
					radio_rx_chain_delay_get(lll->phy, 1);
			}
			break;
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	} else {
		node_rx->hdr.type = NODE_RX_TYPE_REPORT;
	}

	node_rx->hdr.rx_ftr.rssi = (rssi_ready) ? radio_rssi_get() :
						  BT_HCI_LE_RSSI_NOT_AVAILABLE;
#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* save the resolving list index. */
	node_rx->hdr.rx_ftr.rl_idx = rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */
#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	/* save the directed adv report flag */
	node_rx->hdr.rx_ftr.direct = dir_report;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */
#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (node_rx->hdr.type == NODE_RX_TYPE_MESH_REPORT) {
		/* save channel and anchor point ticks. */
		node_rx->hdr.rx_ftr.chan = _radio.scanner.chan - 1;
		node_rx->hdr.rx_ftr.ticks_anchor = _radio.ticks_anchor;
	}
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

	ull_rx_put(node_rx->hdr.link, node_rx);
	ull_rx_sched();

	return 0;
}
