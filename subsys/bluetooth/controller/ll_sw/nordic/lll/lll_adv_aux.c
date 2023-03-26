/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci.h>
#include <soc.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

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
#include "lll_df_types.h"
#include "lll_conn.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_adv_aux.h"
#include "lll_adv_sync.h"
#include "lll_filter.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_adv_internal.h"
#include "lll_prof_internal.h"

#include "ull_adv_types.h"

#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *p);
static void isr_done(void *param);
#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_BACK2BACK)
static void isr_tx_chain(void *param);
#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_BACK2BACK */
static void isr_tx_rx(void *param);
static void isr_rx(void *param);
static inline int isr_rx_pdu(struct lll_adv_aux *lll_aux, uint8_t phy_flags_rx,
			     uint8_t devmatch_ok, uint8_t devmatch_id,
			     uint8_t irkmatch_ok, uint8_t irkmatch_id,
			     uint8_t rssi_ready);
#if defined(CONFIG_BT_PERIPHERAL)
static struct pdu_adv *init_connect_rsp_pdu(struct pdu_adv *pdu_ci);
static void isr_tx_connect_rsp(void *param);
#endif /* CONFIG_BT_PERIPHERAL */

int lll_adv_aux_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_adv_aux_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_adv_aux_prepare(void *param)
{
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	err = lll_prepare(lll_is_abort_cb, lll_abort_cb, prepare_cb, 0, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

void lll_adv_aux_pback_prepare(void *param)
{
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	uint32_t ticks_at_event, ticks_at_start;
	struct pdu_adv_com_ext_adv *com_hdr;
	struct pdu_adv *sec_pdu;
	struct lll_adv_aux *lll;
	struct lll_adv *lll_adv;
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t start_us;
	uint8_t chan_idx;
	uint8_t phy_s;
	uint8_t upd;
	uint32_t aa;

	DEBUG_RADIO_START_A(1);

	lll = p->param;
	lll_adv = lll->adv;

	/* FIXME: get latest only when primary PDU without Aux PDUs */
	upd = 0U;
	sec_pdu = lll_adv_aux_data_latest_get(lll, &upd);
	LL_ASSERT(sec_pdu);

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	struct ll_adv_aux_set *aux;

	/* Get reference to extended header */
	com_hdr = (void *)&sec_pdu->adv_ext_ind;

	aux = HDR_LLL2ULL(lll);
	chan_idx = lll_chan_sel_2(lll->data_chan_counter, aux->data_chan_id,
				  aux->chm[aux->chm_first].data_chan_map,
				  aux->chm[aux->chm_first].data_chan_count);

#else /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
	struct pdu_adv_aux_ptr *aux_ptr;
	struct pdu_adv_ext_hdr *pri_hdr;
	struct pdu_adv *pri_pdu;
	uint8_t *pri_dptr;

	/* Get reference to primary PDU */
	pri_pdu = lll_adv_data_curr_get(lll_adv);
	LL_ASSERT(pri_pdu->type == PDU_ADV_TYPE_EXT_IND);

	/* Get reference to extended header */
	com_hdr = (void *)&pri_pdu->adv_ext_ind;
	pri_hdr = (void *)com_hdr->ext_hdr_adv_data;
	pri_dptr = pri_hdr->data;

	/* NOTE: We shall be here in auxiliary PDU prepare due to
	 * aux_ptr flag being set in the extended common header
	 * flags. Hence, ext_hdr_len is non-zero, an explicit check
	 * is not needed.
	 */
	LL_ASSERT(com_hdr->ext_hdr_len);

	/* traverse through adv_addr, if present */
	if (pri_hdr->adv_addr) {
		pri_dptr += BDADDR_SIZE;
	}

	/* traverse through tgt_addr, if present */
	if (pri_hdr->tgt_addr) {
		pri_dptr += BDADDR_SIZE;
	}

	/* No CTEInfo flag in primary and secondary channel PDU */

	/* traverse through adi, if present */
	if (pri_hdr->adi) {
		pri_dptr += sizeof(struct pdu_adv_adi);
	}

	aux_ptr = (void *)pri_dptr;

	/* Abort if no aux_ptr filled */
	if (unlikely(!pri_hdr->aux_ptr || !PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr))) {
		radio_isr_set(lll_isr_early_abort, lll);
		radio_disable();

		return 0;
	}

	chan_idx = aux_ptr->chan_idx;
#endif /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

	/* Increment counter used in ULL for channel index calculation */
	lll->data_chan_counter++;

	/* Set up Radio H/W */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll_adv->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	phy_s = lll_adv->phy_s;

	/* TODO: if coded we use S8? */
	radio_phy_set(phy_s, lll_adv->phy_flags);
	radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, PDU_AC_PAYLOAD_SIZE_MAX,
			    RADIO_PKT_CONF_PHY(phy_s));

	/* Access address and CRC */
	aa = sys_cpu_to_le32(PDU_AC_ACCESS_ADDR);
	radio_aa_set((uint8_t *)&aa);
	radio_crc_configure(PDU_CRC_POLYNOMIAL,
					PDU_AC_CRC_IV);

	/* Use channel idx calculated or that was in aux_ptr */
	lll_chan_set(chan_idx);

	/* Set the Radio Tx Packet */
	radio_pkt_tx_set(sec_pdu);

	/* Switch to Rx if connectable or scannable */
	if (com_hdr->adv_mode & (BT_HCI_LE_ADV_PROP_CONN |
				     BT_HCI_LE_ADV_PROP_SCAN)) {

		struct pdu_adv *scan_pdu;

		scan_pdu = lll_adv_scan_rsp_latest_get(lll_adv, &upd);
		LL_ASSERT(scan_pdu);

		radio_isr_set(isr_tx_rx, lll);
		radio_tmr_tifs_set(EVENT_IFS_US);
		radio_switch_complete_and_rx(phy_s);

		if (false) {

#if defined(CONFIG_BT_CTLR_PRIVACY)
		} else if (upd) {
			/* Copy the address from the adv packet we will send
			 * into the scan response.
			 */
			memcpy(&scan_pdu->adv_ext_ind.ext_hdr.data[ADVA_OFFSET],
			       &sec_pdu->adv_ext_ind.ext_hdr.data[ADVA_OFFSET],
			       BDADDR_SIZE);
		}

		if (ull_filter_lll_rl_enabled()) {
			struct lll_filter *filter =
				ull_filter_lll_get(!!(lll_adv->filter_policy));

			radio_filter_configure(filter->enable_bitmask,
					       filter->addr_type_bitmask,
					       (uint8_t *)filter->bdaddr);
#endif /* CONFIG_BT_CTLR_PRIVACY */

		} else if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST) &&
			   lll_adv->filter_policy) {
			struct lll_filter *fal = ull_filter_lll_get(true);

			radio_filter_configure(fal->enable_bitmask,
					       fal->addr_type_bitmask,
					       (uint8_t *)fal->bdaddr);
			ARG_UNUSED(scan_pdu);
			ARG_UNUSED(upd);
		} else {
			ARG_UNUSED(scan_pdu);
			ARG_UNUSED(upd);
		}

#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_BACK2BACK)
	} else if (sec_pdu->adv_ext_ind.ext_hdr_len &&
		   sec_pdu->adv_ext_ind.ext_hdr.aux_ptr) {
		lll->last_pdu = sec_pdu;

		radio_isr_set(isr_tx_chain, lll);
		radio_tmr_tifs_set(EVENT_B2B_MAFS_US);
		radio_switch_complete_and_b2b_tx(phy_s, lll_adv->phy_flags,
						 phy_s, lll_adv->phy_flags);
#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_BACK2BACK */

	} else {
		radio_isr_set(isr_done, lll);
		radio_switch_complete_and_disable();
	}

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	start_us = radio_tmr_start(1, ticks_at_start, remainder);

	/* capture end of Tx-ed PDU, used to calculate HCTO. */
	radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_setup();
	radio_gpio_pa_lna_enable(start_us +
				 radio_tx_ready_delay_get(phy_s,
							  lll_adv->phy_flags) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
	ARG_UNUSED(start_us);
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	if (lll_preempt_calc(ull, (TICKER_ID_ADV_AUX_BASE +
				   ull_adv_aux_lll_handle_get(lll)),
			     ticks_at_event)) {
		radio_isr_set(lll_isr_abort, lll);
		radio_disable();
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		uint32_t ret;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC) && \
	defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
		if (sec_pdu->adv_ext_ind.ext_hdr_len &&
		    sec_pdu->adv_ext_ind.ext_hdr.sync_info) {
			ull_adv_sync_lll_syncinfo_fill(sec_pdu, lll);
		}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC &&
	* CONFIG_BT_TICKER_EXT_EXPIRE_INFO
	*/

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	DEBUG_RADIO_START_A(1);

	return 0;
}

static void isr_done(void *param)
{
	struct event_done_extra *extra;

	/* Clear radio status and events */
	lll_isr_status_reset();

	/* Generate auxiliary radio event done */
	extra = ull_done_extra_type_set(EVENT_DONE_EXTRA_TYPE_ADV_AUX);
	LL_ASSERT(extra);

	/* Cleanup radio event and dispatch the done event */
	lll_isr_cleanup(param);
}

#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_BACK2BACK)
static void isr_tx_chain(void *param)
{
	struct lll_adv_aux *lll_aux;
	struct lll_adv *lll;
	struct pdu_adv *pdu;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Clear radio tx status and events */
	lll_isr_tx_status_reset();

	lll_aux = param;
	lll = lll_aux->adv;

	/* FIXME: Use implementation defined channel index */
	lll_chan_set(0);

	pdu = lll_adv_pdu_linked_next_get(lll_aux->last_pdu);
	LL_ASSERT(pdu);
	lll_aux->last_pdu = pdu;

	/* setup tIFS switching */
	if (pdu->adv_ext_ind.ext_hdr_len && pdu->adv_ext_ind.ext_hdr.aux_ptr) {
		radio_isr_set(isr_tx_chain, lll_aux);
		radio_tmr_tifs_set(EVENT_B2B_MAFS_US);
		radio_switch_complete_and_b2b_tx(lll->phy_s, lll->phy_flags,
						 lll->phy_s, lll->phy_flags);
	} else {
		radio_isr_set(isr_done, lll_aux);
		radio_switch_complete_and_disable();
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

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* PA/LNA enable is overwriting packet end used in ISR
		 * profiling, hence back it up for later use.
		 */
		lll_prof_radio_end_backup();
	}

	radio_gpio_pa_setup();
	radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
				 EVENT_B2B_MAFS_US -
				 (EVENT_CLOCK_JITTER_US << 1U) -
				 radio_tx_chain_delay_get(lll->phy_s,
							  lll->phy_flags) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_send();
	}
}
#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_BACK2BACK */

static void isr_tx_rx(void *param)
{
	struct node_rx_pdu *node_rx_prof;
	struct node_rx_pdu *node_rx;
	struct lll_adv_aux *lll_aux;
	struct lll_adv *lll;
	uint32_t hcto;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
		node_rx_prof = lll_prof_reserve();
	}

	/* Clear radio tx status and events */
	lll_isr_tx_status_reset();

	lll_aux = param;
	lll = lll_aux->adv;

	/* setup tIFS switching */
	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_tx(lll->phy_s, 0, lll->phy_s, lll->phy_flags);

	/* setup Rx buffer */
	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);
	radio_pkt_rx_set(node_rx->pdu);

	/* assert if radio packet ptr is not set and radio started rx */
	LL_ASSERT(!radio_is_ready());

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

	radio_isr_set(isr_rx, param);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_enabled()) {
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

		radio_ar_configure(count, irks, (lll->phy_s << 2) | BIT(0));
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	/* +/- 2us active clock jitter, +1 us hcto compensation */
	hcto = radio_tmr_tifs_base_get() + EVENT_IFS_US +
	       (EVENT_CLOCK_JITTER_US << 1U) + 1U;
	hcto += radio_rx_chain_delay_get(lll->phy_s, PHY_FLAGS_S8);
	hcto += addr_us_get(lll->phy_s);
	hcto -= radio_tx_chain_delay_get(lll->phy_s, PHY_FLAGS_S8);
	radio_tmr_hcto_configure(hcto);

	/* capture end of CONNECT_IND PDU, used for calculating first
	 * peripheral event.
	 */
	radio_tmr_end_capture();

	if (IS_ENABLED(CONFIG_BT_CTLR_SCAN_REQ_RSSI) ||
	    IS_ENABLED(CONFIG_BT_CTLR_CONN_RSSI)) {
		radio_rssi_measure();
	}

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
				 radio_tx_chain_delay_get(lll->phy_s,
							  PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_reserve_send(node_rx_prof);
	}
}

static void isr_rx(void *param)
{
	uint8_t phy_flags_rx;
	uint8_t devmatch_ok;
	uint8_t devmatch_id;
	uint8_t irkmatch_ok;
	uint8_t irkmatch_id;
	uint8_t rssi_ready;
	uint8_t trx_done;
	uint8_t crc_ok;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		phy_flags_rx = radio_phy_flags_rx_get();
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
	} else {
		crc_ok = devmatch_ok = irkmatch_ok = rssi_ready =
			phy_flags_rx = 0U;
		devmatch_id = irkmatch_id = FILTER_IDX_NONE;
	}

	/* Clear radio status and events */
	lll_isr_status_reset();

	/* No Rx */
	if (!trx_done) {
		goto isr_rx_do_close;
	}

	if (crc_ok) {
		int err;

		err = isr_rx_pdu(param, phy_flags_rx, devmatch_ok, devmatch_id,
				 irkmatch_ok, irkmatch_id, rssi_ready);
		if (!err) {
			if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
				lll_prof_send();
			}

			return;
		}
	}

isr_rx_do_close:
	radio_isr_set(isr_done, param);
	radio_disable();
}

static inline int isr_rx_pdu(struct lll_adv_aux *lll_aux, uint8_t phy_flags_rx,
			     uint8_t devmatch_ok, uint8_t devmatch_id,
			     uint8_t irkmatch_ok, uint8_t irkmatch_id,
			     uint8_t rssi_ready)
{
	struct node_rx_pdu *node_rx;
	struct pdu_adv_ext_hdr *hdr;
	struct pdu_adv *pdu_adv;
	struct pdu_adv *pdu_aux;
	struct pdu_adv *pdu_rx;
	struct lll_adv *lll;
	uint8_t *tgt_addr;
	uint8_t tx_addr;
	uint8_t rx_addr;
	uint8_t *addr;
	uint8_t upd;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* An IRK match implies address resolution enabled */
	uint8_t rl_idx = irkmatch_ok ? ull_filter_lll_rl_irk_idx(irkmatch_id) :
				    FILTER_IDX_NONE;
#else
	uint8_t rl_idx = FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	lll = lll_aux->adv;

	node_rx = ull_pdu_rx_alloc_peek(1);
	LL_ASSERT(node_rx);

	pdu_rx = (void *)node_rx->pdu;
	pdu_adv = lll_adv_data_curr_get(lll);
	pdu_aux = lll_adv_aux_data_latest_get(lll_aux, &upd);
	LL_ASSERT(pdu_aux);

	hdr = &pdu_aux->adv_ext_ind.ext_hdr;

	addr = &pdu_aux->adv_ext_ind.ext_hdr.data[ADVA_OFFSET];
	tx_addr = pdu_aux->tx_addr;

	if (hdr->tgt_addr) {
		tgt_addr = &pdu_aux->adv_ext_ind.ext_hdr.data[TGTA_OFFSET];
	} else {
		tgt_addr = NULL;
	}
	rx_addr = pdu_aux->rx_addr;

	if ((pdu_rx->type == PDU_ADV_TYPE_AUX_SCAN_REQ) &&
	    (pdu_rx->len == sizeof(struct pdu_adv_scan_req)) &&
	    lll_adv_scan_req_check(lll, pdu_rx, tx_addr, addr, devmatch_ok,
				   &rl_idx)) {
		struct pdu_adv *sr_pdu;

		sr_pdu = lll_adv_scan_rsp_curr_get(lll);

		if (0) {

#if defined(CONFIG_BT_CTLR_ADV_AUX_PDU_BACK2BACK)
		} else if (sr_pdu->adv_ext_ind.ext_hdr_len &&
			   sr_pdu->adv_ext_ind.ext_hdr.aux_ptr) {
			lll_aux->last_pdu = sr_pdu;

			radio_isr_set(isr_tx_chain, lll_aux);
			radio_tmr_tifs_set(EVENT_B2B_MAFS_US);
			radio_switch_complete_and_b2b_tx(lll->phy_s,
							 lll->phy_flags,
							 lll->phy_s,
							 lll->phy_flags);
#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
			radio_tmr_end_capture();
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

#endif /* CONFIG_BT_CTLR_ADV_AUX_PDU_BACK2BACK */

		} else {
			radio_isr_set(isr_done, lll_aux);
			radio_switch_complete_and_disable();
		}

		radio_pkt_tx_set(sr_pdu);

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();
		}

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		if (lll->scan_req_notify) {
			uint32_t err;

			/* Generate the scan request event */
			err = lll_adv_scan_req_report(lll, pdu_rx, rl_idx,
						      rssi_ready);
			if (err) {
				/* Scan Response will not be transmitted */
				return err;
			}
		}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			/* PA/LNA enable is overwriting packet end used in ISR
			 * profiling, hence back it up for later use.
			 */
			lll_prof_radio_end_backup();
		}

		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 EVENT_IFS_US -
					 radio_rx_chain_delay_get(lll->phy_s,
								  phy_flags_rx) -
					 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */
		return 0;

#if defined(CONFIG_BT_PERIPHERAL)
	} else if ((pdu_rx->type == PDU_ADV_TYPE_AUX_CONNECT_REQ) &&
		   (pdu_rx->len == sizeof(struct pdu_adv_connect_ind)) &&
		   lll->conn &&
		   lll_adv_connect_ind_check(lll, pdu_rx, tx_addr, addr,
					     rx_addr, tgt_addr,
					     devmatch_ok, &rl_idx)) {
		struct node_rx_ftr *ftr;
		struct node_rx_pdu *rx;
		struct pdu_adv *pdu_tx;

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			rx = ull_pdu_rx_alloc_peek(4);
		} else {
			rx = ull_pdu_rx_alloc_peek(3);
		}

		if (!rx) {
			return -ENOBUFS;
		}

		/* rx is effectively allocated later, after critical isr steps
		 * are done */
		radio_isr_set(isr_tx_connect_rsp, rx);
		radio_switch_complete_and_disable();
		pdu_tx = init_connect_rsp_pdu(pdu_rx);
		radio_pkt_tx_set(pdu_tx);

		/* assert if radio packet ptr is not set and radio started tx */
		LL_ASSERT(!radio_is_ready());

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();
		}

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			/* PA/LNA enable is overwriting packet end used in ISR
			 * profiling, hence back it up for later use.
			 */
			lll_prof_radio_end_backup();
		}

		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 EVENT_IFS_US -
					 radio_rx_chain_delay_get(lll->phy_s,
								  phy_flags_rx) -
					 HAL_RADIO_GPIO_PA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

		/* Note: this is the same as previous result from alloc_peek */
		rx = ull_pdu_rx_alloc();

		rx->hdr.type = NODE_RX_TYPE_CONNECTION;
		rx->hdr.handle = 0xffff;

		ftr = &(rx->hdr.rx_ftr);
		ftr->param = lll;
		ftr->ticks_anchor = radio_tmr_start_get();
		ftr->radio_end_us = radio_tmr_end_get() -
				    radio_rx_chain_delay_get(lll->phy_s,
							     phy_flags_rx);

#if defined(CONFIG_BT_CTLR_PRIVACY)
		ftr->rl_idx = irkmatch_ok ? rl_idx : FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			ftr->extra = ull_pdu_rx_alloc();
		}

		return 0;
#endif /* CONFIG_BT_PERIPHERAL */
	}

	return -EINVAL;
}

#if defined(CONFIG_BT_PERIPHERAL)
static struct pdu_adv *init_connect_rsp_pdu(struct pdu_adv *pdu_ci)
{
	struct pdu_adv_com_ext_adv *cr_com_hdr;
	struct pdu_adv_ext_hdr *cr_hdr;
	struct pdu_adv *pdu_cr;
	uint8_t *cr_dptr;

	pdu_cr = radio_pkt_scratch_get();
	pdu_cr->type = PDU_ADV_TYPE_AUX_CONNECT_RSP;
	pdu_cr->rfu = 0;
	pdu_cr->chan_sel = 0;
	pdu_cr->tx_addr = pdu_ci->rx_addr;
	pdu_cr->rx_addr = pdu_ci->tx_addr;

	/* Common Extended Header Format Advertising Mode */
	cr_com_hdr = &pdu_cr->adv_ext_ind;
	cr_com_hdr->adv_mode = 0;

	/* Clear Flags */
	cr_hdr = &cr_com_hdr->ext_hdr;
	cr_dptr = (void *)cr_hdr;
	*cr_dptr = 0;
	cr_dptr = cr_hdr->data;

	/* AdvA */
	cr_hdr->adv_addr = 1;
	memcpy(cr_dptr, &pdu_ci->connect_ind.adv_addr, BDADDR_SIZE);
	cr_dptr += BDADDR_SIZE;

	/* InitA */
	cr_hdr->tgt_addr = 1;
	memcpy(cr_dptr, &pdu_ci->connect_ind.init_addr, BDADDR_SIZE);
	cr_dptr += BDADDR_SIZE;

	/* Common Extended Header Length */
	cr_com_hdr->ext_hdr_len = cr_dptr - (uint8_t *)&cr_com_hdr->ext_hdr;

	/* PDU length */
	pdu_cr->len = cr_dptr - &pdu_cr->payload[0];

	return pdu_cr;
}

static void isr_tx_connect_rsp(void *param)
{
	struct node_rx_ftr *ftr;
	struct node_rx_pdu *rx;
	struct lll_adv *lll;
	bool is_done;

	rx = param;
	ftr = &(rx->hdr.rx_ftr);
	lll = ftr->param;

	is_done = radio_is_done();

	if (!is_done) {
		/* AUX_CONNECT_RSP was not sent properly, need to release
		 * allocated resources and keep advertising.
		 */

		rx->hdr.type = NODE_RX_TYPE_RELEASE;

		if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
			ull_rx_put(rx->hdr.link, rx);

			rx = ftr->extra;
			rx->hdr.type = NODE_RX_TYPE_RELEASE;
		}
	}

	ull_rx_put_sched(rx->hdr.link, rx);

	if (is_done) {
		/* Stop further LLL radio events */
		lll->conn->periph.initiated = 1;
	}

	/* Clear radio status and events */
	lll_isr_status_reset();
	lll_isr_cleanup(lll);
}
#endif /* CONFIG_BT_PERIPHERAL */
