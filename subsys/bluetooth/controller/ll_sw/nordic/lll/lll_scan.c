/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci_types.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_df_types.h"
#include "lll_scan.h"
#include "lll_sync.h"
#include "lll_conn.h"
#include "lll_chan.h"
#include "lll_filter.h"
#include "lll_sched.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"
#include "lll_scan_internal.h"

#include "hal/debug.h"

/* Maximum primary Advertising Radio Channels to scan */
#define ADV_CHAN_MAX 3U

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static int resume_prepare_cb(struct lll_prepare_param *p);
static int common_prepare_cb(struct lll_prepare_param *p, bool is_resume);
static int is_abort_cb(void *next, void *curr,
		       lll_prepare_cb_t *resume_cb);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void ticker_stop_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			   uint32_t remainder, uint16_t lazy, uint8_t force,
			   void *param);
static void ticker_op_start_cb(uint32_t status, void *param);
static void isr_rx(void *param);
static void isr_tx(void *param);
static void isr_done(void *param);
static void isr_window(void *param);
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
static void isr_abort(void *param);
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED &&
	* (EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	*/
static void isr_done_cleanup(void *param);

static inline int isr_rx_pdu(struct lll_scan *lll, struct pdu_adv *pdu_adv_rx,
			     uint8_t devmatch_ok, uint8_t devmatch_id,
			     uint8_t irkmatch_ok, uint8_t irkmatch_id,
			     uint8_t rl_idx, uint8_t rssi_ready,
			     uint8_t phy_flags_rx);

#if defined(CONFIG_BT_CENTRAL)
static inline bool isr_scan_init_check(const struct lll_scan *lll,
				       const struct pdu_adv *pdu,
				       uint8_t rl_idx);
#endif /* CONFIG_BT_CENTRAL */

static bool isr_scan_tgta_check(const struct lll_scan *lll, bool init,
				uint8_t addr_type, const uint8_t *addr,
				uint8_t rl_idx, bool *const dir_report);
static inline bool isr_scan_tgta_rpa_check(const struct lll_scan *lll,
					   uint8_t addr_type,
					   const uint8_t *addr,
					   bool *const dir_report);
static inline bool isr_scan_rsp_adva_matches(struct pdu_adv *srsp);
static int isr_rx_scan_report(struct lll_scan *lll, uint8_t devmatch_ok,
			      uint8_t irkmatch_ok, uint8_t rl_idx,
			      uint8_t rssi_ready, uint8_t phy_flags_rx,
			      bool dir_report);

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

void lll_scan_isr_resume(void *param)
{
	static struct lll_prepare_param p;

	/* Clear radio status and events */
	lll_isr_status_reset();

	p.param = param;
	resume_prepare_cb(&p);
}

bool lll_scan_isr_rx_check(const struct lll_scan *lll, uint8_t irkmatch_ok,
			   uint8_t devmatch_ok, uint8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	return (((lll->filter_policy & SCAN_FP_FILTER) == 0U) &&
		(!devmatch_ok || ull_filter_lll_rl_idx_allowed(irkmatch_ok,
							       rl_idx))) ||
	       (((lll->filter_policy & SCAN_FP_FILTER) != 0U) &&
		(devmatch_ok || ull_filter_lll_irk_in_fal(rl_idx)));
#else
	return ((lll->filter_policy & SCAN_FP_FILTER) == 0U) ||
		devmatch_ok;
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_CTLR_ADV_EXT)
bool lll_scan_adva_check(const struct lll_scan *lll, uint8_t addr_type,
			 const uint8_t *addr, uint8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* Only applies to initiator with no filter accept list */
	if (rl_idx != FILTER_IDX_NONE) {
		return (rl_idx == lll->rl_idx);
	} else if (!ull_filter_lll_rl_addr_allowed(addr_type, addr, &rl_idx)) {
		return false;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	/* NOTE: This function to be used only to check AdvA when intiating,
	 *       hence, otherwise we should not use the return value.
	 *       This function is referenced in lll_scan_ext_tgta_check, but
	 *       is not used when not being an initiator, hence return false
	 *       is never reached.
	 */
#if defined(CONFIG_BT_CENTRAL)
	return ((lll->adv_addr_type == addr_type) &&
		!memcmp(lll->adv_addr, addr, BDADDR_SIZE));
#else /* CONFIG_BT_CENTRAL */
	return false;
#endif /* CONFIG_BT_CENTRAL */
}
#endif /* CONFIG_BT_CENTRAL || CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
bool lll_scan_ext_tgta_check(const struct lll_scan *lll, bool pri, bool is_init,
			     const struct pdu_adv *pdu, uint8_t rl_idx,
			     bool *const dir_report)
{
	const uint8_t *adva;
	const uint8_t *tgta;
	uint8_t is_directed;
	uint8_t tx_addr;
	uint8_t rx_addr;

	if (pri && !pdu->adv_ext_ind.ext_hdr.adv_addr) {
		return true;
	}

	if (pdu->len <
	    PDU_AC_EXT_HEADER_SIZE_MIN + sizeof(struct pdu_adv_ext_hdr) +
	    ADVA_SIZE) {
		return false;
	}

	is_directed = pdu->adv_ext_ind.ext_hdr.tgt_addr;
	if (is_directed && (pdu->len < PDU_AC_EXT_HEADER_SIZE_MIN +
				       sizeof(struct pdu_adv_ext_hdr) +
				       ADVA_SIZE + TARGETA_SIZE)) {
		return false;
	}

	tx_addr = pdu->tx_addr;
	rx_addr = pdu->rx_addr;
	adva = &pdu->adv_ext_ind.ext_hdr.data[ADVA_OFFSET];
	tgta = &pdu->adv_ext_ind.ext_hdr.data[TGTA_OFFSET];
	return ((!is_init ||
		 ((lll->filter_policy & SCAN_FP_FILTER) != 0U) ||
		 lll_scan_adva_check(lll, tx_addr, adva, rl_idx)) &&
		((!is_directed) ||
		 (is_directed &&
		  isr_scan_tgta_check(lll, is_init, rx_addr, tgta, rl_idx,
				      dir_report))));
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CENTRAL)
void lll_scan_prepare_connect_req(struct lll_scan *lll, struct pdu_adv *pdu_tx,
				  uint8_t phy, uint8_t phy_flags_rx,
				  uint8_t adv_tx_addr, uint8_t *adv_addr,
				  uint8_t init_tx_addr, uint8_t *init_addr,
				  uint32_t *conn_space_us)
{
	struct lll_conn *lll_conn;
	uint32_t conn_interval_us;
	uint32_t conn_offset_us;

	lll_conn = lll->conn;

	/* Note: this code is also valid for AUX_CONNECT_REQ */
	pdu_tx->type = PDU_ADV_TYPE_CONNECT_IND;

	if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
		pdu_tx->chan_sel = 1;
	} else {
		pdu_tx->chan_sel = 0;
	}

	pdu_tx->tx_addr = init_tx_addr;
	pdu_tx->rx_addr = adv_tx_addr;
	pdu_tx->len = sizeof(struct pdu_adv_connect_ind);
	memcpy(&pdu_tx->connect_ind.init_addr[0], init_addr, BDADDR_SIZE);
	memcpy(&pdu_tx->connect_ind.adv_addr[0], adv_addr, BDADDR_SIZE);
	memcpy(&pdu_tx->connect_ind.access_addr[0],
	       &lll_conn->access_addr[0], 4);
	memcpy(&pdu_tx->connect_ind.crc_init[0], &lll_conn->crc_init[0], 3);
	pdu_tx->connect_ind.win_size = 1;

	conn_interval_us = (uint32_t)lll_conn->interval * CONN_INT_UNIT_US;
	conn_offset_us = radio_tmr_end_get() + EVENT_IFS_US +
			 PDU_AC_MAX_US(sizeof(struct pdu_adv_connect_ind),
				       (phy == PHY_LEGACY) ? PHY_1M : phy) -
			 radio_rx_chain_delay_get(phy, phy_flags_rx);

	/* Add transmitWindowDelay to default calculated connection offset:
	 * 1.25ms for a legacy PDU, 2.5ms for an LE Uncoded PHY and 3.75ms for
	 * an LE Coded PHY.
	 */
	if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (phy) {
		if (phy & PHY_CODED) {
			conn_offset_us += WIN_DELAY_CODED;
		} else {
			conn_offset_us += WIN_DELAY_UNCODED;
		}
#endif
	} else {
		conn_offset_us += WIN_DELAY_LEGACY;
	}

	if (!IS_ENABLED(CONFIG_BT_CTLR_SCHED_ADVANCED) ||
	    lll->conn_win_offset_us == 0U) {
		*conn_space_us = conn_offset_us;
		pdu_tx->connect_ind.win_offset = sys_cpu_to_le16(0);
	} else {
		uint32_t win_offset_us =
			lll->conn_win_offset_us +
			radio_rx_ready_delay_get(phy, PHY_FLAGS_S8);

		while ((win_offset_us & ((uint32_t)1 << 31)) ||
		       (win_offset_us < conn_offset_us)) {
			win_offset_us += conn_interval_us;
		}

		*conn_space_us = win_offset_us;
		pdu_tx->connect_ind.win_offset =
			sys_cpu_to_le16((win_offset_us - conn_offset_us) /
					CONN_INT_UNIT_US);
		pdu_tx->connect_ind.win_size++;
	}

	pdu_tx->connect_ind.interval = sys_cpu_to_le16(lll_conn->interval);
	pdu_tx->connect_ind.latency = sys_cpu_to_le16(lll_conn->latency);
	pdu_tx->connect_ind.timeout = sys_cpu_to_le16(lll->conn_timeout);
	memcpy(&pdu_tx->connect_ind.chan_map[0], &lll_conn->data_chan_map[0],
	       sizeof(pdu_tx->connect_ind.chan_map));
	pdu_tx->connect_ind.hop = lll_conn->data_chan_hop;
	pdu_tx->connect_ind.sca = lll_clock_sca_local_get();
}
#endif /* CONFIG_BT_CENTRAL */

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	return common_prepare_cb(p, false);
}

static int resume_prepare_cb(struct lll_prepare_param *p)
{
	struct ull_hdr *ull;

	ull = HDR_LLL2ULL(p->param);
	p->ticks_at_expire = ticker_ticks_now_get() - lll_event_offset_get(ull);
	p->remainder = 0;
	p->lazy = 0;

	return common_prepare_cb(p, true);
}

static int common_prepare_cb(struct lll_prepare_param *p, bool is_resume)
{
	uint32_t ticks_at_event, ticks_at_start;
	struct node_rx_pdu *node_rx;
	uint32_t remainder_us;
	struct lll_scan *lll;
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t ret;
	uint32_t aa;

	DEBUG_RADIO_START_O(1);

	lll = p->param;

#if defined(CONFIG_BT_CENTRAL)
	/* Check if stopped (on connection establishment race between LLL and
	 * ULL.
	 */
	if (unlikely(lll->is_stop ||
		     (lll->conn &&
		      (lll->conn->central.initiated ||
		       lll->conn->central.cancelled)))) {
		radio_isr_set(lll_isr_early_abort, lll);
		radio_disable();

		return 0;
	}
#endif /* CONFIG_BT_CENTRAL */

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
	radio_phy_set(lll->phy, PHY_FLAGS_S8);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, PDU_AC_LEG_PAYLOAD_SIZE_MAX,
			    RADIO_PKT_CONF_PHY(lll->phy));

	lll->is_adv_ind = 0U;
	lll->is_aux_sched = 0U;
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	radio_phy_set(0, 0);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, PDU_AC_LEG_PAYLOAD_SIZE_MAX,
			    RADIO_PKT_CONF_PHY(RADIO_PKT_CONF_PHY_LEGACY));
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	radio_pkt_rx_set(node_rx->pdu);

	aa = sys_cpu_to_le32(PDU_AC_ACCESS_ADDR);
	radio_aa_set((uint8_t *)&aa);
	radio_crc_configure(PDU_CRC_POLYNOMIAL,
					PDU_AC_CRC_IV);

	lll_chan_set(37 + lll->chan);

	radio_isr_set(isr_rx, lll);

	/* setup tIFS switching */
	if (0) {
	} else if (lll->type ||
#if defined(CONFIG_BT_CENTRAL)
		   lll->conn) {
#else /* !CONFIG_BT_CENTRAL */
		   0) {
#endif /* !CONFIG_BT_CENTRAL */
		radio_tmr_tifs_set(EVENT_IFS_US);
		radio_switch_complete_and_tx(0, 0, 0, 0);
	} else {
		radio_switch_complete_and_disable();
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_enabled()) {
		struct lll_filter *filter =
			ull_filter_lll_get((lll->filter_policy &
					   SCAN_FP_FILTER) != 0U);
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

	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST) && lll->filter_policy) {
		/* Setup Radio Filter */
		struct lll_filter *fal = ull_filter_lll_get(true);

		radio_filter_configure(fal->enable_bitmask,
				       fal->addr_type_bitmask,
				       (uint8_t *)fal->bdaddr);
	}

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0, ticks_at_start, remainder);

	/* capture end of Rx-ed PDU, for initiator to calculate first
	 * central event or extended scan to schedule auxiliary channel
	 * reception.
	 */
	radio_tmr_end_capture();

	/* scanner always measures RSSI */
	radio_rssi_measure();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(0, 0) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_LNA_PIN */
	ARG_UNUSED(remainder_us);
#endif /* !HAL_RADIO_GPIO_HAVE_LNA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	uint32_t overhead;

	overhead = lll_preempt_calc(ull, (TICKER_ID_SCAN_BASE + ull_scan_lll_handle_get(lll)),
				    ticks_at_event);
	/* check if preempt to start has changed */
	if (overhead) {
		LL_ASSERT_OVERHEAD(overhead);

		radio_isr_set(isr_abort, lll);
		radio_disable();

		return -ECANCELED;
	}
#endif /* !CONFIG_BT_CTLR_XTAL_ADVANCED */

	if (!is_resume && lll->ticks_window) {
		/* start window close timeout */
		ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_LLL, TICKER_ID_SCAN_STOP,
				   ticks_at_event, lll->ticks_window, TICKER_NULL_PERIOD,
				   TICKER_NULL_REMAINDER, TICKER_NULL_LAZY, TICKER_NULL_SLOT,
				   ticker_stop_cb, lll, ticker_op_start_cb, (void *)__LINE__);
		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (ret == TICKER_STATUS_BUSY));
	}

#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	/* calc next group in us for the anchor where first connection
	 * event to be placed.
	 */
	if (lll->conn) {
		static memq_link_t link;
		static struct mayfly mfy_after_cen_offset_get = {
			0U, 0U, &link, NULL, ull_sched_mfy_after_cen_offset_get};
		uint32_t retval;

		mfy_after_cen_offset_get.param = p;

		retval = mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_ULL_LOW, 1U,
					&mfy_after_cen_offset_get);
		LL_ASSERT(!retval);
	}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_CTLR_SCHED_ADVANCED */

	ret = lll_prepare_done(lll);
	LL_ASSERT(!ret);

	DEBUG_RADIO_START_O(1);

	return 0;
}

static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb)
{
	struct lll_scan *lll = curr;

#if defined(CONFIG_BT_CENTRAL)
	/* Irrespective of same state/role (initiator radio event) or different
	 * state/role (example, advertising radio event) that overlaps the
	 * initiator, if a CONNECT_REQ PDU has been enqueued for transmission
	 * then initiator shall not abort.
	 */
	if (lll->conn && lll->conn->central.initiated) {
		/* Connection Establishment initiated, do not abort */
		return 0;
	}
#endif /* CONFIG_BT_CENTRAL */

	/* Check if pre-emption by a different state/role radio event */
	if (next != curr) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		/* Resume not to be used if duration being used */
		if (unlikely(!lll->duration_reload || lll->duration_expire))
#endif /* CONFIG_BT_CTLR_ADV_EXT */
		{
			/* Put back to resume state for continuous scanning */
			if (!lll->ticks_window) {
				int err;

				/* Set the resume prepare function to use for
				 * resumption after the pre-emptor is done.
				 */
				*resume_cb = resume_prepare_cb;

				/* Retain HF clock */
				err = lll_hfclock_on();
				LL_ASSERT(err >= 0);

				/* Yield to the pre-emptor, but be
				 * resumed thereafter.
				 */
				return -EAGAIN;
			}

			/* Yield to the pre-emptor */
			return -ECANCELED;
		}
	}

	if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (unlikely(lll->duration_reload && !lll->duration_expire)) {
		/* Duration expired, do not continue, close and generate
		 * done event.
		 */
		radio_isr_set(isr_done_cleanup, lll);
	} else if (lll->state || lll->is_aux_sched) {
		/* Do not abort scan response reception or LLL scheduled
		 * auxiliary PDU scan.
		 */
		return 0;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	} else {
		/* Switch scan window to next radio channel */
		radio_isr_set(isr_window, lll);
	}

	radio_disable();

	return 0;
}

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
#if defined(CONFIG_BT_CENTRAL)
	struct lll_scan *lll = param;
#endif /* CONFIG_BT_CENTRAL */
	int err;

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		/* Perform event abort here.
		 * After event has been cleanly aborted, clean up resources
		 * and dispatch event done.
		 */
		if (0) {
#if defined(CONFIG_BT_CENTRAL)
		} else if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) &&
			   lll->conn && lll->conn->central.initiated) {
			while (!radio_has_disabled()) {
				cpu_sleep();
			}
#endif /* CONFIG_BT_CENTRAL */
		} else {
			radio_isr_set(isr_done_cleanup, param);
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

static void ticker_stop_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			   uint32_t remainder, uint16_t lazy, uint8_t force,
			   void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
	uint32_t ret;

	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL, 0,
			     &mfy);
	LL_ASSERT(!ret);
}

static void ticker_op_start_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

static void isr_rx(void *param)
{
	struct node_rx_pdu *node_rx;
	uint8_t phy_flags_rx;
	struct lll_scan *lll;
	struct pdu_adv *pdu;
	uint8_t devmatch_ok;
	uint8_t devmatch_id;
	uint8_t irkmatch_ok;
	uint8_t irkmatch_id;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t crc_ok;
	uint8_t rl_idx;
	bool has_adva;
	int err = 0U;

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
		phy_flags_rx = radio_phy_flags_rx_get();
	} else {
		crc_ok = devmatch_ok = irkmatch_ok = rssi_ready =
			phy_flags_rx = 0U;
		devmatch_id = irkmatch_id = FILTER_IDX_NONE;
	}

	/* Clear radio status and events */
	lll_isr_status_reset();

	lll = param;

	/* No Rx */
	if (!trx_done || !crc_ok) {
		goto isr_rx_do_close;
	}

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	pdu = (void *)node_rx->pdu;

	if (0) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (pdu->type == PDU_ADV_TYPE_EXT_IND) {
		has_adva = lll_scan_aux_addr_match_get(lll, pdu,
						       &devmatch_ok,
						       &devmatch_id,
						       &irkmatch_ok,
						       &irkmatch_id);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
	} else {
		has_adva = true;
	}

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
			goto isr_rx_do_close;
		}
	}

	err = isr_rx_pdu(lll, pdu, devmatch_ok, devmatch_id, irkmatch_ok,
			 irkmatch_id, rl_idx, rssi_ready, phy_flags_rx);
	if (!err) {
		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_send();
		}

		return;
	}

isr_rx_do_close:
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) && (err == -ECANCELED)) {
		radio_isr_set(isr_done_cleanup, lll);
	} else {
		radio_isr_set(isr_done, lll);
	}

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

	/* Complete currently setup Rx and disable radio */
	radio_switch_complete_and_disable();

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

	/* +/- 2us active clock jitter, +1 us PPI to timer start compensation */
	hcto = radio_tmr_tifs_base_get() + EVENT_IFS_US +
	       (EVENT_CLOCK_JITTER_US << 1) + RANGE_DELAY_US +
	       HAL_RADIO_TMR_START_DELAY_US;
	hcto += radio_rx_chain_delay_get(0, 0);
	hcto += addr_us_get(0);
	hcto -= radio_tx_chain_delay_get(0, 0);

	radio_tmr_hcto_configure(hcto);

	radio_rssi_measure();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() + EVENT_IFS_US - 4 -
				 radio_tx_chain_delay_get(0, 0) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

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
	lll->is_aux_sched = 0U;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	/* setup tIFS switching */
	if (0) {
		/* TODO: Add Rx-Rx switch usecase improvement in the future */
	} else if (lll->type ||
#if defined(CONFIG_BT_CENTRAL)
		   lll->conn) {
#else /* !CONFIG_BT_CENTRAL */
		   0) {
#endif /* !CONFIG_BT_CENTRAL */
		radio_tmr_tifs_set(EVENT_IFS_US);
		radio_switch_complete_and_tx(0, 0, 0, 0);
	} else {
		radio_switch_complete_and_disable();
	}

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

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	uint32_t start_us = radio_tmr_start_now(0);

	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(0, 0) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_LNA_PIN */
	radio_rx_enable();
#endif /* !HAL_RADIO_GPIO_HAVE_LNA_PIN */

	/* capture end of Rx-ed PDU, for initiator to calculate first
	 * central event.
	 */
	radio_tmr_end_capture();
}

static void isr_window(void *param)
{
	uint32_t remainder_us;
	struct lll_scan *lll;

	isr_common_done(param);

	lll = param;

	/* Next radio channel to scan, round-robin 37, 38, and 39. */
	if (++lll->chan == ADV_CHAN_MAX) {
		lll->chan = 0U;
	}
	lll_chan_set(37 + lll->chan);

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_CTLR_ADV_EXT)
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

	uint32_t ticks_at_start;

	ticks_at_start = ticker_ticks_now_get() +
			 HAL_TICKER_CNTR_CMP_OFFSET_MIN;
	remainder_us = radio_tmr_start_tick(0, ticks_at_start);
#else /* !CONFIG_BT_CENTRAL && !CONFIG_BT_CTLR_ADV_EXT */

	remainder_us = radio_tmr_start_now(0);
#endif /* !CONFIG_BT_CENTRAL && !CONFIG_BT_CTLR_ADV_EXT */

	/* capture end of Rx-ed PDU, for initiator to calculate first
	 * central event.
	 */
	radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();
	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(0, 0) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_LNA_PIN */
	ARG_UNUSED(remainder_us);
#endif /* !HAL_RADIO_GPIO_HAVE_LNA_PIN */

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

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
static void isr_abort(void *param)
{
	/* Clear radio status and events */
	lll_isr_status_reset();

	/* Disable Rx filters when aborting scan prepare */
	radio_filter_disable();

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	struct event_done_extra *extra;

	/* Generate Scan done events so that duration and max expiry is
	 * detected in ULL.
	 */
	extra = ull_done_extra_type_set(EVENT_DONE_EXTRA_TYPE_SCAN);
	LL_ASSERT(extra);
#endif  /* CONFIG_BT_CTLR_ADV_EXT */

	lll_isr_cleanup(param);
}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED &&
	* (EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	*/

static void isr_done_cleanup(void *param)
{
	struct lll_scan *lll;
	bool is_resume;

	/* Clear radio status and events */
	lll_isr_status_reset();

	/* Under race between duration expire, is_stop is set in this function,
	 * and event preemption, prevent generating duplicate scan done events.
	 */
	if (lll_is_done(param, &is_resume)) {
		return;
	}

	/* Disable Rx filters when yielding or stopping scan window */
	radio_filter_disable();

	/* Next window to use next advertising radio channel */
	lll = param;
	if (++lll->chan == ADV_CHAN_MAX) {
		lll->chan = 0U;
	}

	/* Scanner stop can expire while here in this ISR.
	 * Deferred attempt to stop can fail as it would have
	 * expired, hence ignore failure.
	 */
	(void)ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_LLL,
			  TICKER_ID_SCAN_STOP, NULL, NULL);

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
	struct node_rx_hdr *node_rx;

	/* Check if there are enough free node rx available:
	 * 1. For generating this scan indication
	 * 2. Keep one available free for reception of ACL connection Rx data
	 * 3. Keep one available free for reception on ACL connection to NACK
	 *    the PDU
	 */
	node_rx = ull_pdu_rx_alloc_peek(3);
	if (node_rx) {
		ull_pdu_rx_alloc();

		/* TODO: add other info by defining a payload struct */
		node_rx->type = NODE_RX_TYPE_SCAN_INDICATION;

		ull_rx_put_sched(node_rx->link, node_rx);
	}
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (_radio.advertiser.is_enabled && _radio.advertiser.is_mesh &&
	    !_radio.advertiser.retry) {
		mayfly_mesh_stop(NULL);
	}
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* If continuous scan then do not generate scan done when radio event
	 * has been placed into prepare pipeline as a resume radio event.
	 */
	if (!is_resume) {
		struct event_done_extra *extra;

		/* Generate Scan done events so that duration and max expiry is
		 * detected in ULL.
		 */
		extra = ull_done_extra_type_set(EVENT_DONE_EXTRA_TYPE_SCAN);
		LL_ASSERT(extra);
	}

	/* Prevent scan events in pipeline from being scheduled if duration has
	 * expired.
	 */
	if (unlikely(lll->duration_reload && !lll->duration_expire)) {
		lll->is_stop = 1U;
	}

	if (lll->is_aux_sched) {
		struct node_rx_pdu *node_rx2;

		lll->is_aux_sched = 0U;

		node_rx2 = ull_pdu_rx_alloc();
		LL_ASSERT(node_rx2);

		node_rx2->hdr.type = NODE_RX_TYPE_EXT_AUX_RELEASE;

		node_rx2->hdr.rx_ftr.param = lll;

		ull_rx_put_sched(node_rx2->hdr.link, node_rx2);
	}
#endif  /* CONFIG_BT_CTLR_ADV_EXT */

	lll_isr_cleanup(param);
}

static inline int isr_rx_pdu(struct lll_scan *lll, struct pdu_adv *pdu_adv_rx,
			     uint8_t devmatch_ok, uint8_t devmatch_id,
			     uint8_t irkmatch_ok, uint8_t irkmatch_id,
			     uint8_t rl_idx, uint8_t rssi_ready,
			     uint8_t phy_flags_rx)
{
	bool dir_report = false;

	if (0) {
#if defined(CONFIG_BT_CENTRAL)
	/* Initiator */
	/* Note: connectable ADV_EXT_IND is handled as any other ADV_EXT_IND
	 *       because we need to receive AUX_ADV_IND anyway.
	 */
	} else if (lll->conn && !lll->conn->central.cancelled &&
		   (pdu_adv_rx->type != PDU_ADV_TYPE_EXT_IND) &&
		   isr_scan_init_check(lll, pdu_adv_rx, rl_idx)) {
		struct lll_conn *lll_conn;
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
		ull = HDR_LLL2ULL(lll);
		if (pdu_end_us > (HAL_TICKER_TICKS_TO_US(ull->ticks_slot) -
				  EVENT_IFS_US - 352 - EVENT_OVERHEAD_START_US -
				  EVENT_TICKER_RES_MARGIN_US)) {
			return -ETIME;
		}

		radio_switch_complete_and_disable();

		/* Acquire the connection context */
		lll_conn = lll->conn;

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

		pdu_tx = (void *)radio_pkt_scratch_get();

		lll_scan_prepare_connect_req(lll, pdu_tx, PHY_LEGACY,
					     PHY_FLAGS_S8, pdu_adv_rx->tx_addr,
					     pdu_adv_rx->adv_ind.addr,
					     init_tx_addr, init_addr,
					     &conn_space_us);

		radio_pkt_tx_set(pdu_tx);

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();
		}

		radio_isr_set(isr_done_cleanup, lll);

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
					 radio_rx_chain_delay_get(0, 0) -
					 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

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

		/* Stop further connection initiation */
		/* FIXME: for extended connection initiation, handle reset on
		 *        event aborted before connect_rsp is received.
		 */
		lll->conn->central.initiated = 1U;

		/* Stop further initiating events */
		lll->is_stop = 1U;

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
		ftr->radio_end_us = conn_space_us;

#if defined(CONFIG_BT_CTLR_PRIVACY)
		ftr->rl_idx = irkmatch_ok ? rl_idx : FILTER_IDX_NONE;
		ftr->lrpa_used = lll->rpa_gen && lrpa;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			ftr->extra = ull_pdu_rx_alloc();
		}

		ull_rx_put_sched(rx->hdr.link, rx);

		return 0;
#endif /* CONFIG_BT_CENTRAL */

	/* Active scanner */
	} else if (((pdu_adv_rx->type == PDU_ADV_TYPE_ADV_IND) ||
		    (pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_IND)) &&
		   (pdu_adv_rx->len >= offsetof(struct pdu_adv_adv_ind, data)) &&
		   (pdu_adv_rx->len <= sizeof(struct pdu_adv_adv_ind)) &&
		   lll->type && !lll->state &&
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
		err = isr_rx_scan_report(lll, devmatch_ok, irkmatch_ok, rl_idx,
					 rssi_ready, phy_flags_rx, false);
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
					 radio_rx_chain_delay_get(0, 0) -
					 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

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
		   (pdu_adv_rx->len >= offsetof(struct pdu_adv_adv_ind, data)) &&
		   (pdu_adv_rx->len <= sizeof(struct pdu_adv_adv_ind))) ||
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_DIRECT_IND) &&
		   (pdu_adv_rx->len == sizeof(struct pdu_adv_direct_ind)) &&
		   (/* allow directed adv packets addressed to this device */
		    isr_scan_tgta_check(lll, false, pdu_adv_rx->rx_addr,
					pdu_adv_rx->direct_ind.tgt_addr,
					rl_idx, &dir_report))) ||
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_EXT_IND) &&
		   lll->phy && lll_scan_ext_tgta_check(lll, true, false,
						       pdu_adv_rx, rl_idx,
						       &dir_report)) ||
#endif /* CONFIG_BT_CTLR_ADV_EXT */
		  ((pdu_adv_rx->type == PDU_ADV_TYPE_SCAN_RSP) &&
		   (pdu_adv_rx->len >= offsetof(struct pdu_adv_scan_rsp, data)) &&
		   (pdu_adv_rx->len <= sizeof(struct pdu_adv_scan_rsp)) &&
		   (lll->state != 0U) &&
		   isr_scan_rsp_adva_matches(pdu_adv_rx))) &&
		 (pdu_adv_rx->len != 0) &&
#if defined(CONFIG_BT_CENTRAL)
		   /* Note: ADV_EXT_IND is allowed here even if initiating
		    *       because we still need to get AUX_ADV_IND as for any
		    *       other ADV_EXT_IND.
		    */
		   (!lll->conn || (pdu_adv_rx->type == PDU_ADV_TYPE_EXT_IND))) {
#else /* !CONFIG_BT_CENTRAL */
		   1) {
#endif /* !CONFIG_BT_CENTRAL */
		uint32_t err;

		/* save the scan response packet */
		err = isr_rx_scan_report(lll, devmatch_ok, irkmatch_ok, rl_idx,
					 rssi_ready, phy_flags_rx, dir_report);
		if (err) {
			/* Auxiliary PDU LLL scanning has been setup */
			if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
			    (err == -EBUSY)) {
				if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
					lll_prof_cputime_capture();
				}

				return 0;
			}

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

#if defined(CONFIG_BT_CENTRAL)
static inline bool isr_scan_init_check(const struct lll_scan *lll,
				       const struct pdu_adv *pdu,
				       uint8_t rl_idx)
{
	return ((((lll->filter_policy & SCAN_FP_FILTER) != 0U) ||
		lll_scan_adva_check(lll, pdu->tx_addr, pdu->adv_ind.addr,
				    rl_idx)) &&
		(((pdu->type == PDU_ADV_TYPE_ADV_IND) &&
		  (pdu->len >= offsetof(struct pdu_adv_adv_ind, data)) &&
		  (pdu->len <= sizeof(struct pdu_adv_adv_ind))) ||
		 ((pdu->type == PDU_ADV_TYPE_DIRECT_IND) &&
		  (pdu->len == sizeof(struct pdu_adv_direct_ind)) &&
		  (/* allow directed adv packets addressed to this device */
			  isr_scan_tgta_check(lll, true, pdu->rx_addr,
					      pdu->direct_ind.tgt_addr, rl_idx,
					      NULL)))));
}
#endif /* CONFIG_BT_CENTRAL */

static bool isr_scan_tgta_check(const struct lll_scan *lll, bool init,
				uint8_t addr_type, const uint8_t *addr,
				uint8_t rl_idx, bool *dir_report)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_addr_resolve(addr_type, addr, rl_idx)) {
		return true;
	} else if (init && lll->rpa_gen && ull_filter_lll_lrpa_get(rl_idx)) {
		/* Initiator generating RPAs, and could not resolve TargetA:
		 * discard
		 */
		return false;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	return (((lll->init_addr_type == addr_type) &&
		 !memcmp(lll->init_addr, addr, BDADDR_SIZE))) ||
	       /* allow directed adv packets where TargetA address
		* is resolvable private address (scanner only)
		*/
	       isr_scan_tgta_rpa_check(lll, addr_type, addr, dir_report);
}

static inline bool isr_scan_tgta_rpa_check(const struct lll_scan *lll,
					   uint8_t addr_type,
					   const uint8_t *addr,
					   bool *const dir_report)
{
	if (((lll->filter_policy & SCAN_FP_EXT) != 0U) && (addr_type != 0U) &&
	    ((addr[5] & 0xc0) == 0x40)) {

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

static int isr_rx_scan_report(struct lll_scan *lll, uint8_t devmatch_ok,
			      uint8_t irkmatch_ok, uint8_t rl_idx,
			      uint8_t rssi_ready, uint8_t phy_flags_rx,
			      bool dir_report)
{
	struct node_rx_pdu *node_rx;
	int err = 0;

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
					radio_rx_chain_delay_get(lll->phy,
								 phy_flags_rx);
				ftr->phy_flags = phy_flags_rx;
				ftr->aux_lll_sched =
					lll_scan_aux_setup(pdu_adv_rx, lll->phy,
							   phy_flags_rx,
							   lll_scan_aux_isr_aux_setup,
							   lll);
				if (ftr->aux_lll_sched) {
					lll->is_aux_sched = 1U;
					err = -EBUSY;
				}
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
	node_rx->hdr.rx_ftr.rl_idx = irkmatch_ok ? rl_idx : FILTER_IDX_NONE;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	node_rx->hdr.rx_ftr.direct_resolved = (rl_idx != FILTER_IDX_NONE);
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	/* save the directed adv report flag */
	node_rx->hdr.rx_ftr.direct = dir_report;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC) && \
	defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
	node_rx->hdr.rx_ftr.devmatch = devmatch_ok;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC && CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (node_rx->hdr.type == NODE_RX_TYPE_MESH_REPORT) {
		/* save channel and anchor point ticks. */
		node_rx->hdr.rx_ftr.chan = _radio.scanner.chan - 1;
		node_rx->hdr.rx_ftr.ticks_anchor = _radio.ticks_anchor;
	}
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

	ull_rx_put_sched(node_rx->hdr.link, node_rx);

	return err;
}
