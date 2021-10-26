/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <soc.h>
#include <sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"

#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_vendor.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_adv_iso.h"

#include "lll_internal.h"
#include "lll_adv_internal.h"
#include "lll_tim_internal.h"

#include "ll_feat.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_adv_iso
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);
static void prepare(void *param);
static void create_prepare_bh(void *param);
static void prepare_bh(void *param);
static int create_prepare_cb(struct lll_prepare_param *p);
static int prepare_cb(struct lll_prepare_param *p);
static int prepare_cb_common(struct lll_prepare_param *p);
static void isr_tx_create(void *param);
static void isr_tx_normal(void *param);
static void isr_tx_common(void *param,
			  radio_isr_cb_t isr_tx,
			  radio_isr_cb_t isr_done);
static void isr_done_create(void *param);
static void isr_done_term(void *param);

int lll_adv_iso_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_adv_iso_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_adv_iso_create_prepare(void *param)
{
	prepare(param);
	create_prepare_bh(param);
}

void lll_adv_iso_prepare(void *param)
{
	prepare(param);
	prepare_bh(param);
}

static int init_reset(void)
{
	return 0;
}

static void prepare(void *param)
{
	struct lll_prepare_param *p;
	struct lll_adv_iso *lll;
	uint16_t elapsed;
	int err;

	err = lll_hfclock_on();
	LL_ASSERT(err >= 0);

	p = param;

	/* Instants elapsed */
	elapsed = p->lazy + 1U;

	lll = p->param;

	/* Save the (latency + 1) for use in event */
	lll->latency_prepare += elapsed;
}

static void create_prepare_bh(void *param)
{
	int err;

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, lll_abort_cb, create_prepare_cb, 0U,
			  param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static void prepare_bh(void *param)
{
	int err;

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, lll_abort_cb, prepare_cb, 0U, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int create_prepare_cb(struct lll_prepare_param *p)
{
	int err;

	err = prepare_cb_common(p);
	if (err) {
		DEBUG_RADIO_START_A(1);
		return 0;
	}

	radio_isr_set(isr_tx_create, p->param);

	DEBUG_RADIO_START_A(1);
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	int err;

	err = prepare_cb_common(p);
	if (err) {
		DEBUG_RADIO_START_A(1);
		return 0;
	}

	radio_isr_set(isr_tx_normal, p->param);

	DEBUG_RADIO_START_A(1);
	return 0;
}

static int prepare_cb_common(struct lll_prepare_param *p)
{
	struct lll_adv_iso *lll;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint16_t event_counter;
	uint8_t access_addr[4];
	uint16_t data_chan_id;
	uint8_t data_chan_use;
	uint8_t crc_init[3];
	struct pdu_bis *pdu;
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t start_us;
	uint8_t phy;

	DEBUG_RADIO_START_A(1);

	lll = p->param;

	/* Deduce the latency */
	lll->latency_event = lll->latency_prepare - 1U;

	/* Calculate the current event counter value */
	event_counter = (lll->payload_count / lll->bn) +
			(lll->latency_event * lll->bn);

	/* Update BIS packet counter to next value */
	lll->payload_count += (lll->latency_prepare * lll->bn);

	/* Reset accumulated latencies */
	lll->latency_prepare = 0U;

	/* Initialize to mandatory parameter values */
	lll->bis_curr = 1U;
	lll->ptc_curr = 0U;
	lll->irc_curr = 1U;
	lll->bn_curr = 1U;

	/* Calculate the Access Address for the BIS event */
	util_bis_aa_le32(lll->bis_curr, lll->seed_access_addr, access_addr);
	data_chan_id = lll_chan_id(access_addr);

	/* Calculate the CRC init value for the BIS event,
	 * preset with the BaseCRCInit value from the BIGInfo data the most
	 * significant 2 octets and the BIS_Number for the specific BIS in the
	 * least significant octet.
	 */
	crc_init[0] = lll->bis_curr;
	(void)memcpy(&crc_init[1], lll->base_crc_init, sizeof(uint16_t));

	/* Calculate the radio channel to use for ISO event */
	data_chan_use = lll_chan_iso_event(event_counter, data_chan_id,
					   lll->data_chan_map,
					   lll->data_chan_count,
					   &lll->data_chan_prn_s,
					   &lll->data_chan_remap_idx);

	/* Start setting up of Radio h/w */
	radio_reset();

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	radio_tx_power_set(lll->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	phy = lll->phy;
	radio_phy_set(phy, lll->phy_flags);
	radio_pkt_configure(8U, lll->max_pdu, (phy << 1));
	radio_aa_set(access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL, sys_get_le24(crc_init));
	lll_chan_set(data_chan_use);

	/* FIXME: get ISO data PDU */
	pdu = radio_pkt_scratch_get();
	if (lll->bn_curr >= lll->bn) {
		pdu->ll_id = PDU_BIS_LLID_COMPLETE_END;
	} else {
		pdu->ll_id = PDU_BIS_LLID_START_CONTINUE;
	}
	pdu->len = LL_BIS_OCTETS_TX_MAX;

	pdu->payload[0] = lll->bn_curr;
	pdu->payload[1] = lll->irc_curr;
	pdu->payload[2] = lll->ptc_curr;
	pdu->payload[3] = lll->bis_curr;

	pdu->payload[4] = lll->payload_count;
	pdu->payload[5] = lll->payload_count >> 8;
	pdu->payload[6] = lll->payload_count >> 16;
	pdu->payload[7] = lll->payload_count >> 24;
	pdu->payload[8] = lll->payload_count >> 32;

	/* Initialize reserve bit */
	pdu->rfu = 0U;

	/* Handle control procedure */
	if (unlikely(lll->term_req || !!(lll->chm_req - lll->chm_ack))) {
		if (lll->term_req) {
			if (!lll->term_ack) {
				lll->term_ack = 1U;
				lll->ctrl_expire = CONN_ESTAB_COUNTDOWN;
				lll->ctrl_instant = event_counter +
						    lll->ctrl_expire - 1U;
				lll->cssn++;
			}

			lll->ctrl_chan_use = data_chan_use;

		} else if ((lll->chm_req - lll->chm_ack) == 1U) {
			lll->chm_ack--;
			lll->cssn++;
		}
		pdu->cstf = 1U;
	} else {
		pdu->cstf = 0U;
	}
	pdu->cssn = lll->cssn;

	radio_pkt_tx_set(pdu);

	if ((lll->bn_curr == lll->bn) &&
	    (lll->irc_curr == lll->irc) &&
	    (lll->ptc_curr == lll->ptc) &&
	    (lll->bis_curr == lll->num_bis) &&
	    !pdu->cstf) {
		radio_switch_complete_and_disable();
	} else {
		uint16_t iss_us;

		iss_us = lll->sub_interval -
			 PDU_BIS_US(pdu->len, lll->enc, lll->phy,
				    lll->phy_flags);
		radio_tmr_tifs_set(iss_us);
		radio_switch_complete_and_b2b_tx(lll->phy, lll->phy_flags,
						 lll->phy, lll->phy_flags);
	}

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	start_us = radio_tmr_start(1U, ticks_at_start, remainder);

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_setup();

	radio_gpio_pa_lna_enable(start_us +
				 radio_tx_ready_delay_get(phy, PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
	ARG_UNUSED(start_us);
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

	if (0) {
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	/* check if preempt to start has changed */
	} else if (lll_preempt_calc(ull, (TICKER_ID_ADV_ISO_BASE + lll->handle),
				    ticks_at_event)) {
		radio_isr_set(lll_isr_abort, lll);
		radio_disable();

		return -ECANCELED;
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	} else {
		uint32_t ret;

		ret = lll_prepare_done(lll);
		LL_ASSERT(!ret);
	}

	return 0;
}

static void isr_tx_create(void *param)
{
	isr_tx_common(param, isr_tx_create, isr_done_create);
}

static void isr_tx_normal(void *param)
{
	isr_tx_common(param, isr_tx_normal, lll_isr_done);
}

static void isr_tx_common(void *param,
			  radio_isr_cb_t isr_tx,
			  radio_isr_cb_t isr_done)
{
	struct pdu_bis *pdu = NULL;
	struct lll_adv_iso *lll;
	uint8_t access_addr[4];
	uint16_t data_chan_id;
	uint8_t data_chan_use;
	uint8_t crc_init[3];
	uint8_t bis;

	lll = param;
	/* FIXME: Sequential or Interleaved BIS subevents decision */
	/* Sequential Tx complete flow pseudo code */
	if (lll->bn_curr < lll->bn) {
		/* transmit the (bn_curr)th Tx PDU of bis_curr */
		lll->bn_curr++; /* post increment */

		bis = lll->bis_curr;

	} else if (lll->irc_curr < lll->irc) {
		lll->bn_curr = 0U;
		/* transmit the (bn_curr)th Tx PDU of bis_curr */
		lll->bn_curr++;  /* post increment */
		lll->irc_curr++; /* post increment */

		bis = lll->bis_curr;

	} else if (lll->ptc_curr < lll->ptc) {
		lll->ptc_curr++; /* pre increment */
		/* transmit the (ptc_curr * bn)th Tx PDU */

		bis = lll->bis_curr;

	} else if (lll->bis_curr < lll->num_bis) {
		lll->bis_curr++;
		lll->ptc_curr = 0U;
		lll->irc_curr = 1U;
		lll->bn_curr = 0U;
		/* transmit the (bn_curr)th PDU of bis_curr */
		lll->bn_curr++;  /* post increment */

		bis = lll->bis_curr;

	} else if (lll->term_ack) {
		/* Transmit the control PDU and close the BIG event
		 *  there after.
		 */
		struct pdu_big_ctrl_term_ind *term;

		pdu = radio_pkt_scratch_get();
		pdu->ll_id = PDU_BIS_LLID_CTRL;
		pdu->cssn = lll->cssn;
		pdu->cstf = 0U;

		pdu->len = offsetof(struct pdu_big_ctrl, ctrl_data) +
			   sizeof(struct pdu_big_ctrl_term_ind);
		pdu->ctrl.opcode = PDU_BIG_CTRL_TYPE_TERM_IND;

		term = (void *)&pdu->ctrl.term_ind;
		term->reason = lll->term_reason;
		term->instant = lll->ctrl_instant;

		/* control subevent to use bis = 0 and se_n = 1 */
		bis = 0U;
		data_chan_use = lll->ctrl_chan_use;
	} else {
		/* Close the BIG event as no more subevents */
		radio_isr_set(isr_done, lll);
		radio_disable();

		return;
	}

	lll_isr_tx_status_reset();

	/* Calculate the Access Address for the BIS event */
	util_bis_aa_le32(bis, lll->seed_access_addr, access_addr);
	data_chan_id = lll_chan_id(access_addr);

	/* Calculate the CRC init value for the BIS event,
	 * preset with the BaseCRCInit value from the BIGInfo data the most
	 * significant 2 octets and the BIS_Number for the specific BIS in the
	 * least significant octet.
	 */
	crc_init[0] = bis;
	(void)memcpy(&crc_init[1], lll->base_crc_init, sizeof(uint16_t));

	radio_aa_set(access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL, sys_get_le24(crc_init));

	/* FIXME: get ISO data PDU */
	if (!pdu) {
		pdu = radio_pkt_scratch_get();
		if (lll->bn_curr >= lll->bn && !(lll->ptc_curr % lll->bn)) {
			pdu->ll_id = PDU_BIS_LLID_COMPLETE_END;
		} else {
			pdu->ll_id = PDU_BIS_LLID_START_CONTINUE;
		}
		pdu->len = LL_BIS_OCTETS_TX_MAX;
		pdu->cssn = lll->cssn;
		pdu->cstf = 0U;

		pdu->payload[0] = lll->bn_curr;
		pdu->payload[1] = lll->irc_curr;
		pdu->payload[2] = lll->ptc_curr;
		pdu->payload[3] = lll->bis_curr;

		/* Calculate the radio channel to use for ISO event */
		data_chan_use =
			lll_chan_iso_subevent(data_chan_id,
					      lll->data_chan_map,
					      lll->data_chan_count,
					      &lll->data_chan_prn_s,
					      &lll->data_chan_remap_idx);

	}
	pdu->rfu = 0U;

	lll_chan_set(data_chan_use);

	radio_pkt_tx_set(pdu);

	if (!bis) {
		radio_switch_complete_and_disable();

		radio_isr_set(isr_done_term, lll);
	} else {
		uint16_t iss_us;

		iss_us = lll->sub_interval -
			 PDU_BIS_US(pdu->len, lll->enc, lll->phy,
				    lll->phy_flags);
		radio_tmr_tifs_set(iss_us);
		radio_switch_complete_and_b2b_tx(lll->phy, lll->phy_flags,
						 lll->phy, lll->phy_flags);

		radio_isr_set(isr_tx, lll);
	}

	/* assert if radio packet ptr is not set and radio started rx */
	LL_ASSERT(!radio_is_ready());
}

static void isr_done_create(void *param)
{
	lll_isr_status_reset();

	ull_done_extra_type_set(EVENT_DONE_EXTRA_TYPE_ADV_ISO_COMPLETE);

	lll_isr_cleanup(param);
}

static void isr_done_term(void *param)
{
	struct lll_adv_iso *lll;

	lll_isr_status_reset();

	lll = param;

	LL_ASSERT(lll->ctrl_expire);

	lll->ctrl_expire--;
	if (!lll->ctrl_expire) {
		ull_done_extra_type_set(EVENT_DONE_EXTRA_TYPE_ADV_ISO_TERMINATE);
	}

	lll_isr_cleanup(param);
}
