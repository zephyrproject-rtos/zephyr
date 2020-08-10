/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr.h>
#include <soc.h>
#include <sys/byteorder.h>
#include <bluetooth/hci.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"

#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_adv.h"
#include "lll_adv_aux.h"
#include "lll_filter.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_adv_internal.h"
#include "lll_prof_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_adv_aux
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);
static int prepare_cb(struct lll_prepare_param *prepare_param);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_tx(void *param);
static void isr_rx(void *param);
static inline int isr_rx_pdu(struct lll_adv_aux *lll_aux,
			     uint8_t devmatch_ok, uint8_t devmatch_id,
			     uint8_t irkmatch_ok, uint8_t irkmatch_id,
			     uint8_t rssi_ready);

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
	struct lll_prepare_param *p = param;
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	err = lll_prepare(lll_is_abort_cb, abort_cb, prepare_cb, 0, p);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

void lll_adv_aux_pback_prepare(void *param)
{
}

static int init_reset(void)
{
	return 0;
}

static int prepare_cb(struct lll_prepare_param *prepare_param)
{
	struct lll_adv_aux *lll = prepare_param->param;
	uint32_t aa = sys_cpu_to_le32(PDU_AC_ACCESS_ADDR);
	struct pdu_adv_com_ext_adv *pri_com_hdr;
	uint32_t ticks_at_event, ticks_at_start;
	struct pdu_adv *pri_pdu, *sec_pdu;
	struct pdu_adv_aux_ptr *aux_ptr;
	struct pdu_adv_hdr *pri_hdr;
	struct lll_adv *lll_adv;
	struct evt_hdr *evt;
	uint32_t remainder;
	uint32_t start_us;
	uint8_t *pri_dptr;
	uint8_t phy_s;
	uint8_t upd;

	DEBUG_RADIO_START_A(1);

	/* FIXME: get latest only when primary PDU without Aux PDUs */
	sec_pdu = lll_adv_aux_data_latest_get(lll, &upd);

	/* Get reference to primary PDU */
	lll_adv = lll->adv;
	pri_pdu = lll_adv_data_curr_get(lll_adv);
	LL_ASSERT(pri_pdu->type == PDU_ADV_TYPE_EXT_IND);

	/* Get reference to extended header */
	pri_com_hdr = (void *)&pri_pdu->adv_ext_ind;
	pri_hdr = (void *)pri_com_hdr->ext_hdr_adi_adv_data;
	pri_dptr = (uint8_t *)pri_hdr + sizeof(*pri_hdr);

	/* traverse through adv_addr, if present */
	if (pri_hdr->adv_addr) {
		pri_dptr += BDADDR_SIZE;
	}

	/* traverse through adi, if present */
	if (pri_hdr->adi) {
		pri_dptr += sizeof(struct pdu_adv_adi);
	}

	aux_ptr = (void *)pri_dptr;

	/* Abort if no aux_ptr filled */
	if (!pri_hdr->aux_ptr || !aux_ptr->offs) {
		radio_isr_set(lll_isr_abort, lll);
		radio_disable();
	}

#if !defined(BT_CTLR_ADV_EXT_PBACK)
	/* Set up Radio H/W */
	radio_reset();
#endif  /* !BT_CTLR_ADV_EXT_PBACK */

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	phy_s = lll_adv->phy_s;

	/* TODO: if coded we use S8? */
	radio_phy_set(phy_s, 1);
	radio_pkt_configure(8, PDU_AC_PAYLOAD_SIZE_MAX, (phy_s << 1));

#if !defined(BT_CTLR_ADV_EXT_PBACK)
	/* Access address and CRC */
	radio_aa_set((uint8_t *)&aa);
	radio_crc_configure(((0x5bUL) | ((0x06UL) << 8) | ((0x00UL) << 16)),
			    0x555555);
#endif  /* !BT_CTLR_ADV_EXT_PBACK */

	/* Use channel idx in aux_ptr */
	lll_chan_set(aux_ptr->chan_idx);

	/* Set the Radio Tx Packet */
	radio_pkt_tx_set(sec_pdu);

	/* Switch to Rx if connectable or scannable */
	if (pri_com_hdr->adv_mode & (BT_HCI_LE_ADV_PROP_CONN |
				     BT_HCI_LE_ADV_PROP_SCAN)) {

		struct pdu_adv *scan_pdu;

		scan_pdu = lll_adv_scan_rsp_latest_get(lll_adv, &upd);

#if defined(CONFIG_BT_CTLR_PRIVACY)
		if (upd) {
			/* Copy the address from the adv packet we will send
			 * into the scan response.
			 */
			memcpy(&scan_pdu->adv_ext_ind.ext_hdr_adi_adv_data[1],
			       &sec_pdu->adv_ext_ind.ext_hdr_adi_adv_data[1],
			       BDADDR_SIZE);
		}
#else
		ARG_UNUSED(scan_pdu);
		ARG_UNUSED(upd);
#endif /* !CONFIG_BT_CTLR_PRIVACY */

		radio_isr_set(isr_tx, lll);
		radio_tmr_tifs_set(EVENT_IFS_US);
		radio_switch_complete_and_rx(phy_s);
	} else {
		radio_isr_set(lll_isr_done, lll);
		radio_switch_complete_and_disable();
	}

#if defined(BT_CTLR_ADV_EXT_PBACK)
	start_us = 1000;
	radio_tmr_start_us(1, start_us);
#else /* !BT_CTLR_ADV_EXT_PBACK */

	ticks_at_event = prepare_param->ticks_at_expire;
	evt = HDR_LLL2EVT(lll);
	ticks_at_event += lll_evt_offset_get(evt);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = prepare_param->remainder;
	start_us = radio_tmr_start(1, ticks_at_start, remainder);
#endif /* !BT_CTLR_ADV_EXT_PBACK */

	/* capture end of Tx-ed PDU, used to calculate HCTO. */
	radio_tmr_end_capture();

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
	if (lll_preempt_calc(evt, (TICKER_ID_ADV_AUX_BASE +
				   ull_adv_aux_lll_handle_get(lll)),
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
	int err;

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		/* Perform event abort here.
		 * After event has been cleanly aborted, clean up resources
		 * and dispatch event done.
		 */
		radio_isr_set(lll_isr_done, param);
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

static void isr_tx(void *param)
{
	struct lll_adv_aux *lll_aux = param;
	struct lll_adv *lll = lll_aux->adv;
	uint32_t hcto;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Clear radio tx status and events */
	lll_isr_tx_status_reset();

	/* setup tIFS switching */
	radio_tmr_tifs_set(EVENT_IFS_US);
	radio_switch_complete_and_tx(lll->phy_s, 0, lll->phy_s, 0);

	radio_pkt_rx_set(radio_pkt_scratch_get());
	/* assert if radio packet ptr is not set and radio started rx */
	LL_ASSERT(!radio_is_ready());

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

	radio_isr_set(isr_rx, param);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (ull_filter_lll_rl_enabled()) {
		uint8_t count, *irks = ull_filter_lll_irks_get(&count);

		radio_ar_configure(count, irks);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	/* +/- 2us active clock jitter, +1 us hcto compensation */
	hcto = radio_tmr_tifs_base_get() + EVENT_IFS_US + 4 + 1;
	hcto += radio_rx_chain_delay_get(lll->phy_s, 1);
	hcto += addr_us_get(0);
	hcto -= radio_tx_chain_delay_get(lll->phy_s, 0);
	radio_tmr_hcto_configure(hcto);

	/* capture end of CONNECT_IND PDU, used for calculating first
	 * slave event.
	 */
	radio_tmr_end_capture();

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
				 radio_tx_chain_delay_get(0, 0) -
				 CONFIG_BT_CTLR_GPIO_LNA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_LNA_PIN */

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* NOTE: as scratch packet is used to receive, it is safe to
		 * generate profile event using rx nodes.
		 */
		lll_prof_send();
	}
}

static void isr_rx(void *param)
{
	uint8_t trx_done;
	uint8_t crc_ok;
	uint8_t devmatch_ok;
	uint8_t devmatch_id;
	uint8_t irkmatch_ok;
	uint8_t irkmatch_id;
	uint8_t rssi_ready;

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

	/* No Rx */
	if (!trx_done) {
		goto isr_rx_do_close;
	}

	if (crc_ok) {
		int err;

		err = isr_rx_pdu(param, devmatch_ok, devmatch_id,
				 irkmatch_ok, irkmatch_id, rssi_ready);
		if (!err) {
			if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
				lll_prof_send();
			}

			return;
		}
	}

isr_rx_do_close:
	radio_isr_set(lll_isr_done, param);
	radio_disable();
}

static inline int isr_rx_pdu(struct lll_adv_aux *lll_aux,
			     uint8_t devmatch_ok, uint8_t devmatch_id,
			     uint8_t irkmatch_ok, uint8_t irkmatch_id,
			     uint8_t rssi_ready)
{
	struct pdu_adv *pdu_rx, *pdu_adv, *pdu_aux;
	struct lll_adv *lll = lll_aux->adv;
	uint8_t tx_addr;
	uint8_t *addr;
	uint8_t upd;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* An IRK match implies address resolution enabled */
	uint8_t rl_idx = irkmatch_ok ? ull_filter_lll_rl_irk_idx(irkmatch_id) :
				    FILTER_IDX_NONE;
#else
	uint8_t rl_idx = FILTER_IDX_NONE;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	pdu_rx = (void *)radio_pkt_scratch_get();
	pdu_adv = lll_adv_data_curr_get(lll);
	pdu_aux = lll_adv_aux_data_latest_get(lll_aux, &upd);

	if (pdu_adv->type == PDU_ADV_TYPE_EXT_IND) {
		/* AdvA is placed at 2nd byte of ext hdr data */
		addr = &pdu_aux->adv_ext_ind.ext_hdr_adi_adv_data[1];
		tx_addr = pdu_aux->tx_addr;
	} else {
		addr = pdu_adv->adv_ind.addr;
		tx_addr = pdu_adv->tx_addr;
	}

	if ((pdu_rx->type == PDU_ADV_TYPE_AUX_SCAN_REQ) &&
	    (pdu_rx->len == sizeof(struct pdu_adv_scan_req)) &&
	    lll_adv_scan_req_check(lll, pdu_rx, tx_addr, addr, devmatch_ok,
				   &rl_idx)) {
		radio_isr_set(lll_isr_done, lll);
		radio_switch_complete_and_disable();
		radio_pkt_tx_set(lll_adv_scan_rsp_curr_get(lll));

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

#if defined(CONFIG_BT_CTLR_GPIO_PA_PIN)
		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			/* PA/LNA enable is overwriting packet end used in ISR
			 * profiling, hence back it up for later use.
			 */
			lll_prof_radio_end_backup();
		}

		radio_gpio_pa_setup();
		radio_gpio_pa_lna_enable(radio_tmr_tifs_base_get() +
					 EVENT_IFS_US -
					 radio_rx_chain_delay_get(0, 0) -
					 CONFIG_BT_CTLR_GPIO_PA_OFFSET);
#endif /* CONFIG_BT_CTLR_GPIO_PA_PIN */
		return 0;
	}

	/* TODO: add handling for AUX_CONNECT_REQ */

	return -EINVAL;
}
