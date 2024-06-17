/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <soc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_vendor.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_adv_iso.h"
#include "lll_iso_tx.h"

#include "lll_internal.h"
#include "lll_adv_iso_internal.h"
#include "lll_prof_internal.h"

#include "ll_feat.h"

#include "hal/debug.h"

#define TEST_WITH_DUMMY_PDU 0

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
static void next_chan_calc(struct lll_adv_iso *lll, uint16_t event_counter,
			   uint16_t data_chan_id);
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
	uint64_t payload_count;
	uint16_t data_chan_id;
	uint8_t data_chan_use;
	uint8_t crc_init[3];
	struct pdu_bis *pdu;
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t start_us;
	uint8_t pkt_flags;
	uint32_t ret;
	uint8_t phy;

	DEBUG_RADIO_START_A(1);

	lll = p->param;

	/* Deduce the latency */
	lll->latency_event = lll->latency_prepare - 1U;

	/* Calculate the current event counter value */
	event_counter = (lll->payload_count / lll->bn) + lll->latency_event;

	/* Update BIS payload counter to next value */
	lll->payload_count += (lll->latency_prepare * lll->bn);
	payload_count = lll->payload_count - lll->bn;

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
	radio_tx_power_set(lll->adv->tx_pwr_lvl);
#else
	radio_tx_power_set(RADIO_TXP_DEFAULT);
#endif

	phy = lll->phy;
	radio_phy_set(phy, lll->phy_flags);
	radio_aa_set(access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL, sys_get_le24(crc_init));
	lll_chan_set(data_chan_use);

	/* Get ISO data PDU */
#if !TEST_WITH_DUMMY_PDU
	struct lll_adv_iso_stream *stream;
	memq_link_t *link = NULL;
	struct node_tx_iso *tx;
	uint16_t stream_handle;
	uint16_t handle;
	uint8_t bis_idx;

	bis_idx = lll->num_bis;
	while (bis_idx--) {
		stream_handle = lll->stream_handle[bis_idx];
		handle = LL_BIS_ADV_HANDLE_FROM_IDX(stream_handle);
		stream = ull_adv_iso_lll_stream_get(stream_handle);
		LL_ASSERT(stream);

		do {
			link = memq_peek(stream->memq_tx.head,
					 stream->memq_tx.tail, (void **)&tx);
			if (link) {
				if (tx->payload_count < payload_count) {
					memq_dequeue(stream->memq_tx.tail,
						     &stream->memq_tx.head,
						     NULL);

					tx->next = link;
					ull_iso_lll_ack_enqueue(handle, tx);
				} else if (tx->payload_count >=
					   lll->payload_count) {
					link = NULL;
				} else {
					if (tx->payload_count !=
					    payload_count) {
						link = NULL;
					}

					break;
				}
			}
		} while (link);
	}

	if (!link) {
		pdu = radio_pkt_empty_get();
		pdu->ll_id = lll->framing ? PDU_BIS_LLID_FRAMED : PDU_BIS_LLID_START_CONTINUE;
		pdu->len = 0U;
	} else {
		pdu = (void *)tx->pdu;
	}

#else /* TEST_WITH_DUMMY_PDU */
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

	pdu->payload[4] = payload_count;
	pdu->payload[5] = payload_count >> 8;
	pdu->payload[6] = payload_count >> 16;
	pdu->payload[7] = payload_count >> 24;
	pdu->payload[8] = payload_count >> 32;
#endif /* TEST_WITH_DUMMY_PDU */

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
		} else if (((lll->chm_req - lll->chm_ack) & CHM_STATE_MASK) ==
			   CHM_STATE_REQ) {
			lll->chm_ack--;
			lll->ctrl_expire = CONN_ESTAB_COUNTDOWN;
			lll->ctrl_instant = event_counter + lll->ctrl_expire;
			lll->cssn++;
		}

		pdu->cstf = 1U;
	} else {
		pdu->cstf = 0U;
	}
	pdu->cssn = lll->cssn;

	/* Radio packet configuration */
	pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_BIS, phy,
					 RADIO_PKT_CONF_CTE_DISABLED);
	if (IS_ENABLED(CONFIG_BT_CTLR_BROADCAST_ISO_ENC) &&
	    pdu->len && lll->enc) {
		/* Encryption */
		lll->ccm_tx.counter = payload_count;

		(void)memcpy(lll->ccm_tx.iv, lll->giv, 4U);
		mem_xor_32(lll->ccm_tx.iv, lll->ccm_tx.iv, access_addr);

		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
				    (lll->max_pdu + PDU_MIC_SIZE), pkt_flags);

		radio_pkt_tx_set(radio_ccm_iso_tx_pkt_set(&lll->ccm_tx,
						RADIO_PKT_CONF_PDU_TYPE_BIS,
						pdu));
	} else {
		if (lll->enc) {
			radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
					    (lll->max_pdu + PDU_MIC_SIZE),
					    pkt_flags);
		} else {
			radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
					    lll->max_pdu, pkt_flags);
		}

		radio_pkt_tx_set(pdu);
	}

	/* Setup radio IFS switching */
	if ((lll->bn_curr == lll->bn) &&
	    (lll->irc_curr == lll->irc) &&
	    (lll->ptc_curr == lll->ptc) &&
	    (lll->bis_curr == lll->num_bis) &&
	    !pdu->cstf) {
		radio_switch_complete_and_disable();
	} else {
		uint16_t iss_us;

		/* Calculate next subevent start based on previous PDU length */
		iss_us = lll->sub_interval -
			 PDU_BIS_US(pdu->len, ((pdu->len) ? lll->enc : 0U),
				    lll->phy, lll->phy_flags);

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

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR) || IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN)) {
		/* setup capture of PDU end timestamp */
		radio_tmr_end_capture();
	}

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	radio_gpio_pa_setup();

	radio_gpio_pa_lna_enable(start_us +
				 radio_tx_ready_delay_get(lll->phy,
							  lll->phy_flags) -
				 HAL_RADIO_GPIO_PA_OFFSET);
#else /* !HAL_RADIO_GPIO_HAVE_PA_PIN */
	ARG_UNUSED(start_us);
#endif /* !HAL_RADIO_GPIO_HAVE_PA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	uint32_t overhead;

	overhead = lll_preempt_calc(ull, (TICKER_ID_ADV_ISO_BASE + lll->handle), ticks_at_event);
	/* check if preempt to start has changed */
	if (overhead) {
		LL_ASSERT_OVERHEAD(overhead);

		radio_isr_set(lll_isr_abort, lll);
		radio_disable();

		return -ECANCELED;
	}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

	ret = lll_prepare_done(lll);
	LL_ASSERT(!ret);

	/* Calculate ahead the next subevent channel index */
	next_chan_calc(lll, event_counter, data_chan_id);

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
	uint8_t data_chan_use = 0;
	struct lll_adv_iso *lll;
	uint8_t access_addr[4];
	uint64_t payload_count;
	uint16_t data_chan_id;
	uint8_t crc_init[3];
	uint8_t bis;

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	uint16_t pa_iss_us = 0U;
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	lll = param;
	/* FIXME: Sequential or Interleaved BIS subevents decision */
	/* Sequential Tx complete flow pseudo code */
	if (lll->bn_curr < lll->bn) {
		/* transmit the (bn_curr)th Tx PDU of bis_curr */
		lll->bn_curr++; /* post increment */

		bis = lll->bis_curr;

	} else if (lll->irc_curr < lll->irc) {
		/* transmit the (bn_curr)th Tx PDU of bis_curr */
		lll->bn_curr = 1U;
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
		/* transmit the (bn_curr)th PDU of bis_curr */
		lll->bn_curr = 1U;

		bis = lll->bis_curr;

	} else if (lll->term_ack) {
		/* Transmit the control PDU and close the BIG event
		 *  there after.
		 */
		struct pdu_big_ctrl_term_ind *term;

		pdu = radio_pkt_big_ctrl_get();
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
		payload_count = lll->payload_count - lll->bn;

	} else if (((lll->chm_req - lll->chm_ack) & CHM_STATE_MASK) ==
		   CHM_STATE_SEND) {
		/* Transmit the control PDU and stop after 6 intervals
		 */
		struct pdu_big_ctrl_chan_map_ind *chm;

		pdu = radio_pkt_big_ctrl_get();
		pdu->ll_id = PDU_BIS_LLID_CTRL;
		pdu->cssn = lll->cssn;
		pdu->cstf = 0U;

		pdu->len = offsetof(struct pdu_big_ctrl, ctrl_data) +
			   sizeof(struct pdu_big_ctrl_chan_map_ind);
		pdu->ctrl.opcode = PDU_BIG_CTRL_TYPE_CHAN_MAP_IND;

		chm = (void *)&pdu->ctrl.chan_map_ind;
		(void)memcpy(chm->chm, lll->chm_chan_map, sizeof(chm->chm));
		chm->instant = lll->ctrl_instant;

		/* control subevent to use bis = 0 and se_n = 1 */
		bis = 0U;
		payload_count = lll->payload_count - lll->bn;

	} else {
		struct lll_adv_iso_stream *stream;
		uint16_t stream_handle;
		memq_link_t *link;
		uint16_t handle;

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();
		}

		for (uint8_t bis_idx = 0U; bis_idx < lll->num_bis; bis_idx++) {
			stream_handle = lll->stream_handle[bis_idx];
			handle = LL_BIS_ADV_HANDLE_FROM_IDX(stream_handle);
			stream = ull_adv_iso_lll_stream_get(stream_handle);
			LL_ASSERT(stream);

			do {
				struct node_tx_iso *tx;

				link = memq_peek(stream->memq_tx.head,
						 stream->memq_tx.tail,
						 (void **)&tx);
				if (link) {
					if (tx->payload_count >=
					    lll->payload_count) {
						break;
					}

					memq_dequeue(stream->memq_tx.tail,
						     &stream->memq_tx.head,
						     NULL);

					tx->next = link;
					ull_iso_lll_ack_enqueue(handle, tx);
				}
			} while (link);
		}

		/* Close the BIG event as no more subevents */
		radio_isr_set(isr_done, lll);
		radio_disable();

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_send();
		}

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

	/* Get ISO data PDU, not control subevent */
	if (!pdu) {
		uint8_t payload_index;

		payload_index = (lll->bn_curr - 1U) +
				(lll->ptc_curr * lll->pto);
		payload_count = lll->payload_count + payload_index - lll->bn;

#if !TEST_WITH_DUMMY_PDU
		struct lll_adv_iso_stream *stream;
		uint16_t stream_handle;
		struct node_tx_iso *tx;
		memq_link_t *link;

		stream_handle = lll->stream_handle[lll->bis_curr - 1U];
		stream = ull_adv_iso_lll_stream_get(stream_handle);
		LL_ASSERT(stream);

		link = memq_peek_n(stream->memq_tx.head, stream->memq_tx.tail,
				   payload_index, (void **)&tx);
		if (!link || (tx->payload_count != payload_count)) {
			payload_index = 0U;
			do {
				link = memq_peek_n(stream->memq_tx.head,
						   stream->memq_tx.tail,
						   payload_index, (void **)&tx);
				payload_index++;
			} while (link &&
				 (tx->payload_count < payload_count));
		}
		if (!link || (tx->payload_count != payload_count)) {
			pdu = radio_pkt_empty_get();
			pdu->ll_id = lll->framing ? PDU_BIS_LLID_FRAMED :
						    PDU_BIS_LLID_START_CONTINUE;
			pdu->len = 0U;
		} else {
			pdu = (void *)tx->pdu;
		}
		pdu->cssn = lll->cssn;
		pdu->cstf = (lll->term_req || !!(lll->chm_req - lll->chm_ack));

#else /* TEST_WITH_DUMMY_PDU */
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

		pdu->payload[4] = payload_count;
		pdu->payload[5] = payload_count >> 8;
		pdu->payload[6] = payload_count >> 16;
		pdu->payload[7] = payload_count >> 24;
		pdu->payload[8] = payload_count >> 32;
#endif /* TEST_WITH_DUMMY_PDU */

		data_chan_use = lll->next_chan_use;
	}
	pdu->rfu = 0U;

	if (!bis) {
		const uint16_t event_counter = payload_count / lll->bn;

		/* Calculate the radio channel to use for ISO event */
		data_chan_use = lll_chan_iso_event(event_counter, data_chan_id,
						   lll->data_chan_map,
						   lll->data_chan_count,
						   &lll->data_chan_prn_s,
						   &lll->data_chan_remap_idx);
	}

	lll_chan_set(data_chan_use);

	/* Encryption */
	if (IS_ENABLED(CONFIG_BT_CTLR_BROADCAST_ISO_ENC) &&
	    pdu->len && lll->enc) {
		lll->ccm_tx.counter = payload_count;

		(void)memcpy(lll->ccm_tx.iv, lll->giv, 4U);
		mem_xor_32(lll->ccm_tx.iv, lll->ccm_tx.iv, access_addr);

		radio_pkt_tx_set(radio_ccm_iso_tx_pkt_set(&lll->ccm_tx,
						RADIO_PKT_CONF_PDU_TYPE_BIS,
						pdu));
	} else {
		radio_pkt_tx_set(pdu);
	}

	/* Control subevent, then complete subevent and close radio use */
	if (!bis) {
		uint8_t pkt_flags;

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_BIS,
						 lll->phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		if (lll->enc) {
			radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
					    (sizeof(struct pdu_big_ctrl) + PDU_MIC_SIZE),
					    pkt_flags);
		} else {
			radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
					    sizeof(struct pdu_big_ctrl),
					    pkt_flags);
		}

		radio_switch_complete_and_b2b_tx_disable();

		radio_isr_set(isr_done_term, lll);
	} else {
		uint16_t iss_us;

		/* Calculate next subevent start based on previous PDU length */
		iss_us = lll->sub_interval -
			 PDU_BIS_US(pdu->len, ((pdu->len) ? lll->enc : 0U),
				    lll->phy, lll->phy_flags);

		radio_tmr_tifs_set(iss_us);
		radio_switch_complete_and_b2b_tx(lll->phy, lll->phy_flags,
						 lll->phy, lll->phy_flags);

		radio_isr_set(isr_tx, lll);

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
		/* local variable used later to store iss_us next subevent PA
		 * setup.
		 */
		pa_iss_us = iss_us;
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */
	}

	/* assert if radio packet ptr is not set and radio started tx */
	LL_ASSERT(!radio_is_ready());

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR) || IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN)) {
		/* setup capture of PDU end timestamp */
		radio_tmr_end_capture();
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
				 lll->pa_iss_us -
				 (EVENT_CLOCK_JITTER_US << 1U) -
				 radio_tx_chain_delay_get(lll->phy,
							  lll->phy_flags) -
				 HAL_RADIO_GPIO_PA_OFFSET);

	/* Remember to use it for the next subevent PA setup */
	lll->pa_iss_us = pa_iss_us;

#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */

	/* Calculate ahead the next subevent channel index */
	const uint16_t event_counter = (lll->payload_count / lll->bn) - 1U;

	next_chan_calc(lll, event_counter, data_chan_id);

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_send();
	}
}

static void next_chan_calc(struct lll_adv_iso *lll, uint16_t event_counter,
			   uint16_t data_chan_id)
{
	/* Calculate ahead the next subevent channel index */
	if ((lll->bn_curr < lll->bn) ||
	    (lll->irc_curr < lll->irc) ||
	    (lll->ptc_curr < lll->ptc)) {
		/* Calculate the radio channel to use for next subevent */
		lll->next_chan_use = lll_chan_iso_subevent(data_chan_id,
						lll->data_chan_map,
						lll->data_chan_count,
						&lll->data_chan_prn_s,
						&lll->data_chan_remap_idx);
	} else if (lll->bis_curr < lll->num_bis) {
		uint8_t access_addr[4];

		/* Calculate the Access Address for the next BIS subevent */
		util_bis_aa_le32((lll->bis_curr + 1U), lll->seed_access_addr,
				 access_addr);
		data_chan_id = lll_chan_id(access_addr);

		/* Calculate the radio channel to use for next BIS */
		lll->next_chan_use = lll_chan_iso_event(event_counter,
						data_chan_id,
						lll->data_chan_map,
						lll->data_chan_count,
						&lll->data_chan_prn_s,
						&lll->data_chan_remap_idx);
	}
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
	uint16_t elapsed_event;

	lll_isr_status_reset();

	lll = param;
	LL_ASSERT(lll->ctrl_expire);

	elapsed_event = lll->latency_event + 1U;
	if (lll->ctrl_expire > elapsed_event) {
		lll->ctrl_expire -= elapsed_event;
	} else {
		lll->ctrl_expire = 0U;

		if (lll->chm_req != lll->chm_ack) {
			struct lll_adv_sync *sync_lll;
			struct lll_adv *adv_lll;

			/* Reset channel map procedure requested */
			lll->chm_ack = lll->chm_req;

			/* Request periodic advertising to update channel map
			 * in the BIGInfo when filling BIG Offset until Thread
			 * context gets to update it using new PDU buffer.
			 */
			adv_lll = lll->adv;
			sync_lll = adv_lll->sync;
			if (sync_lll->iso_chm_done_req ==
			    sync_lll->iso_chm_done_ack) {
				struct node_rx_pdu *rx;

				/* Request ULL to update the channel map in the
				 * BIGInfo struct present in the current PDU of
				 * Periodic Advertising radio events. Channel
				 * Map is updated when filling the BIG offset.
				 */
				sync_lll->iso_chm_done_req++;

				/* Notify Thread context to update channel map
				 * in the BIGInfo struct present in the Periodic
				 * Advertising PDU.
				 */
				rx = ull_pdu_rx_alloc();
				LL_ASSERT(rx);

				rx->hdr.type = NODE_RX_TYPE_BIG_CHM_COMPLETE;
				rx->rx_ftr.param = lll;

				ull_rx_put_sched(rx->hdr.link, rx);
			}

			/* Use new channel map */
			lll->data_chan_count = lll->chm_chan_count;
			(void)memcpy(lll->data_chan_map, lll->chm_chan_map,
				     sizeof(lll->data_chan_map));
		} else {
			ull_done_extra_type_set(EVENT_DONE_EXTRA_TYPE_ADV_ISO_TERMINATE);
		}
	}

	lll_isr_cleanup(param);
}
