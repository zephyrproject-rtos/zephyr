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
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_sync_iso.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"

#include "ll_feat.h"

#include "hal/debug.h"

static int init_reset(void);
static void prepare(void *param);
static void create_prepare_bh(void *param);
static void prepare_bh(void *param);
static int create_prepare_cb(struct lll_prepare_param *p);
static int prepare_cb(struct lll_prepare_param *p);
static int prepare_cb_common(struct lll_prepare_param *p);
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_rx_estab(void *param);
static void isr_rx(void *param);
static void isr_rx_done(void *param);
static void isr_done(void *param);
static void next_chan_calc(struct lll_sync_iso *lll, uint16_t event_counter,
			   uint16_t data_chan_id);
static void isr_rx_iso_data_valid(const struct lll_sync_iso *const lll,
				  uint16_t handle, struct node_rx_pdu *node_rx);
static void isr_rx_iso_data_invalid(const struct lll_sync_iso *const lll,
				    uint8_t bn, uint16_t handle,
				    struct node_rx_pdu *node_rx);
static void isr_rx_ctrl_recv(struct lll_sync_iso *lll, struct pdu_bis *pdu);

/* FIXME: Optimize by moving to a common place, as similar variable is used for
 *        connections too.
 */
static uint8_t trx_cnt;
static uint8_t crc_ok_anchor;

int lll_sync_iso_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_sync_iso_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void lll_sync_iso_create_prepare(void *param)
{
	prepare(param);
	create_prepare_bh(param);
}

void lll_sync_iso_prepare(void *param)
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
	struct lll_sync_iso *lll;
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

	/* Accumulate window widening */
	lll->window_widening_prepare_us += lll->window_widening_periodic_us *
					   elapsed;
	if (lll->window_widening_prepare_us > lll->window_widening_max_us) {
		lll->window_widening_prepare_us = lll->window_widening_max_us;
	}
}

static void create_prepare_bh(void *param)
{
	int err;

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, abort_cb, create_prepare_cb, 0U,
			  param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static void prepare_bh(void *param)
{
	int err;

	/* Invoke common pipeline handling of prepare */
	err = lll_prepare(lll_is_abort_cb, abort_cb, prepare_cb, 0U, param);
	LL_ASSERT(!err || err == -EINPROGRESS);
}

static int create_prepare_cb(struct lll_prepare_param *p)
{
	int err;

	err = prepare_cb_common(p);
	if (err) {
		DEBUG_RADIO_START_O(1);
		return 0;
	}

	radio_isr_set(isr_rx_estab, p->param);

	DEBUG_RADIO_START_O(1);
	return 0;
}

static int prepare_cb(struct lll_prepare_param *p)
{
	int err;

	err = prepare_cb_common(p);
	if (err) {
		DEBUG_RADIO_START_O(1);
		return 0;
	}

	radio_isr_set(isr_rx, p->param);

	DEBUG_RADIO_START_O(1);
	return 0;
}

static int prepare_cb_common(struct lll_prepare_param *p)
{
	struct lll_sync_iso_stream *stream;
	struct node_rx_pdu *node_rx;
	struct lll_sync_iso *lll;
	uint32_t ticks_at_event;
	uint32_t ticks_at_start;
	uint16_t stream_handle;
	uint16_t event_counter;
	uint8_t access_addr[4];
	uint16_t data_chan_id;
	uint8_t data_chan_use;
	uint32_t remainder_us;
	uint8_t crc_init[3];
	struct ull_hdr *ull;
	uint32_t remainder;
	uint32_t hcto;
	uint32_t ret;
	uint8_t phy;

	DEBUG_RADIO_START_O(1);

	lll = p->param;

	/* Deduce the latency */
	lll->latency_event = lll->latency_prepare - 1U;

	/* Calculate the current event counter value */
	event_counter = (lll->payload_count / lll->bn) + lll->latency_event;

	/* Update BIS packet counter to next value */
	lll->payload_count += (lll->latency_prepare * lll->bn);

	/* Reset accumulated latencies */
	lll->latency_prepare = 0U;

	/* Current window widening */
	lll->window_widening_event_us += lll->window_widening_prepare_us;
	lll->window_widening_prepare_us = 0U;
	if (lll->window_widening_event_us > lll->window_widening_max_us) {
		lll->window_widening_event_us =	lll->window_widening_max_us;
	}

	/* Initialize trx chain count */
	trx_cnt = 0U;

	/* Initialize anchor point CRC ok flag */
	crc_ok_anchor = 0U;

	/* Initialize to mandatory parameter values */
	lll->bis_curr = 1U;
	lll->ptc_curr = 0U;
	lll->irc_curr = 1U;
	lll->bn_curr = 1U;

	/* Initialize control subevent flag */
	lll->ctrl = 0U;

	/* Calculate the Access Address for the BIS event */
	util_bis_aa_le32(lll->bis_curr, lll->seed_access_addr, access_addr);
	data_chan_id = lll_chan_id(access_addr);

	/* Calculate the radio channel to use for ISO event and hence store the
	 * channel to be used for control subevent.
	 */
	data_chan_use = lll_chan_iso_event(event_counter, data_chan_id,
					   lll->data_chan_map,
					   lll->data_chan_count,
					   &lll->data_chan_prn_s,
					   &lll->data_chan_remap_idx);

	/* Initialize stream current */
	lll->stream_curr = 0U;

	/* Skip subevents until first selected BIS */
	stream_handle = lll->stream_handle[lll->stream_curr];
	stream = ull_sync_iso_lll_stream_get(stream_handle);
	if ((stream->bis_index != lll->bis_curr) &&
	    (stream->bis_index <= lll->num_bis)) {
		/* First selected BIS */
		lll->bis_curr = stream->bis_index;

		/* Calculate the Access Address for the current BIS */
		util_bis_aa_le32(lll->bis_curr, lll->seed_access_addr,
				 access_addr);
		data_chan_id = lll_chan_id(access_addr);

		/* Calculate the channel id for the next BIS subevent */
		data_chan_use = lll_chan_iso_event(event_counter,
					data_chan_id,
					lll->data_chan_map,
					lll->data_chan_count,
					&lll->data_chan_prn_s,
					&lll->data_chan_remap_idx);
	}

	/* Calculate the CRC init value for the BIS event,
	 * preset with the BaseCRCInit value from the BIGInfo data the most
	 * significant 2 octets and the BIS_Number for the specific BIS in the
	 * least significant octet.
	 */
	crc_init[0] = lll->bis_curr;
	(void)memcpy(&crc_init[1], lll->base_crc_init, sizeof(uint16_t));

	/* Start setting up of Radio h/w */
	radio_reset();

	phy = lll->phy;
	radio_phy_set(phy, PHY_FLAGS_S8);
	radio_aa_set(access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL, sys_get_le24(crc_init));
	lll_chan_set(data_chan_use);

	/* By design, there shall always be one free node rx available for
	 * setting up radio for new PDU reception.
	 */
	node_rx = ull_iso_pdu_rx_alloc_peek(1U);
	LL_ASSERT(node_rx);

	/* Encryption */
	if (IS_ENABLED(CONFIG_BT_CTLR_BROADCAST_ISO_ENC) &&
	    lll->enc) {
		uint64_t payload_count;
		uint8_t pkt_flags;

		payload_count = lll->payload_count - lll->bn;
		lll->ccm_rx.counter = payload_count;

		(void)memcpy(lll->ccm_rx.iv, lll->giv, 4U);
		mem_xor_32(lll->ccm_rx.iv, lll->ccm_rx.iv, access_addr);

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_BIS,
						 phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT,
				    (lll->max_pdu + PDU_MIC_SIZE), pkt_flags);
		radio_pkt_rx_set(radio_ccm_iso_rx_pkt_set(&lll->ccm_rx, phy,
							  RADIO_PKT_CONF_PDU_TYPE_BIS,
							  node_rx->pdu));
	} else {
		uint8_t pkt_flags;

		pkt_flags = RADIO_PKT_CONF_FLAGS(RADIO_PKT_CONF_PDU_TYPE_BIS,
						 phy,
						 RADIO_PKT_CONF_CTE_DISABLED);
		radio_pkt_configure(RADIO_PKT_CONF_LENGTH_8BIT, lll->max_pdu,
				    pkt_flags);
		radio_pkt_rx_set(node_rx->pdu);
	}

	radio_switch_complete_and_disable();

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0U, ticks_at_start, remainder);

	radio_tmr_ready_save(remainder_us);
	radio_tmr_aa_save(0U);
	radio_tmr_aa_capture();

	hcto = remainder_us +
	       ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
		 lll->window_widening_event_us) << 1) +
	       lll->window_size_event_us;
	hcto += radio_rx_ready_delay_get(lll->phy, PHY_FLAGS_S8);
	hcto += addr_us_get(lll->phy);
	hcto += radio_rx_chain_delay_get(lll->phy, PHY_FLAGS_S8);
	radio_tmr_hcto_configure(hcto);

	radio_tmr_end_capture();
	radio_rssi_measure();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(remainder_us +
				 radio_rx_ready_delay_get(lll->phy, PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	uint32_t overhead;

	overhead = lll_preempt_calc(ull, (TICKER_ID_SCAN_SYNC_ISO_BASE +
					  ull_sync_iso_lll_index_get(lll)), ticks_at_event);
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

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	struct event_done_extra *e;
	int err;

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		radio_isr_set(isr_done, param);
		radio_disable();
		return;
	}

	/* NOTE: Else clean the top half preparations of the aborted event
	 * currently in preparation pipeline.
	 */
	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	/* Extra done event, to check sync lost */
	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC_ISO;
	e->trx_cnt = 0U;
	e->crc_valid = 0U;

	lll_done(param);
}

static void isr_rx_estab(void *param)
{
	struct event_done_extra *e;
	uint8_t trx_done;
	uint8_t crc_ok;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		trx_cnt++;
	} else {
		crc_ok = 0U;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

	/* Calculate and place the drift information in done event */
	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC_ISO_ESTAB;
	e->estab_failed = 0U;
	e->trx_cnt = trx_cnt;
	e->crc_valid = crc_ok;

	if (trx_cnt) {
		struct lll_sync_iso *lll;

		lll = param;
		e->drift.preamble_to_addr_us = addr_us_get(lll->phy);
		e->drift.start_to_address_actual_us =
			radio_tmr_aa_get() - radio_tmr_ready_get();
		e->drift.window_widening_event_us =
			lll->window_widening_event_us;

		/* Reset window widening, as anchor point sync-ed */
		lll->window_widening_event_us = 0U;
		lll->window_size_event_us = 0U;
	}

	lll_isr_cleanup(param);

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_send();
	}
}

static void isr_rx(void *param)
{
	struct lll_sync_iso_stream *stream;
	struct node_rx_pdu *node_rx;
	struct lll_sync_iso *lll;
	uint8_t access_addr[4];
	uint16_t data_chan_id;
	uint8_t data_chan_use;
	uint8_t crc_init[3];
	uint8_t rssi_ready;
	uint32_t start_us;
	uint8_t new_burst;
	uint8_t trx_done;
	uint8_t bis_idx;
	uint8_t skipped;
	uint8_t crc_ok;
	uint32_t hcto;
	uint8_t bis;
	uint8_t nse;

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (!trx_done) {
		/* Clear radio rx status and events */
		lll_isr_rx_status_reset();

		/* initialize LLL context reference */
		lll = param;

		/* BIS index */
		bis_idx = lll->bis_curr - 1U;

		goto isr_rx_done;
	}

	crc_ok = radio_crc_is_valid();
	rssi_ready = radio_rssi_is_ready();
	trx_cnt++;

	/* initialize LLL context reference */
	lll = param;

	/* Save the AA captured for the first anchor point sync */
	if (!radio_tmr_aa_restore()) {
		const struct lll_sync_iso_stream *sync_stream;
		uint32_t se_offset_us;
		uint8_t se;

		crc_ok_anchor = crc_ok;

		sync_stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
		se = ((lll->bis_curr - sync_stream->bis_index) *
		      ((lll->bn * lll->irc) + lll->ptc)) +
		     ((lll->irc_curr - 1U) * lll->bn) + (lll->bn_curr - 1U) +
		     lll->ptc_curr + lll->ctrl;
		se_offset_us = lll->sub_interval * se;
		radio_tmr_aa_save(radio_tmr_aa_get() - se_offset_us);
		radio_tmr_ready_save(radio_tmr_ready_get() - se_offset_us);
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* BIS index */
	bis_idx = lll->bis_curr - 1U;

	/* Check CRC and generate ISO Data PDU */
	if (crc_ok) {
		struct lll_sync_iso_stream *sync_stream;
		uint16_t stream_handle;
		uint8_t payload_offset;
		uint8_t payload_index;
		struct pdu_bis *pdu;

		/* Check if Control Subevent being received */
		if ((lll->bn_curr == lll->bn) &&
		    (lll->irc_curr == lll->irc) &&
		    (lll->ptc_curr == lll->ptc) &&
		    (lll->bis_curr == lll->num_bis) &&
		    lll->ctrl) {
			lll->cssn_curr = lll->cssn_next;

			/* Check the dedicated Control PDU buffer */
			pdu = radio_pkt_big_ctrl_get();
			if (pdu->ll_id == PDU_BIS_LLID_CTRL) {
				isr_rx_ctrl_recv(lll, pdu);
			}

			goto isr_rx_done;
		} else {
			/* Check if there are 2 free rx buffers, one will be
			 * consumed to receive the current PDU, and the other
			 * is to ensure a PDU can be setup for the radio DMA to
			 * receive in the next sub_interval/iso_interval.
			 */
			node_rx = ull_iso_pdu_rx_alloc_peek(2U);
			if (!node_rx) {
				goto isr_rx_done;
			}
		}

		pdu = (void *)node_rx->pdu;

		/* Check for new control PDU in control subevent */
		if (pdu->cstf && (pdu->cssn != lll->cssn_curr)) {
			lll->cssn_next = pdu->cssn;
			/* TODO: check same CSSN is used in every subevent */
		}

		/* Check payload buffer overflow */
		payload_offset = (lll->bn_curr - 1U) +
				 (lll->ptc_curr * lll->pto);
		if (payload_offset > lll->payload_count_max) {
			goto isr_rx_done;
		}

		/* Calculate the payload index in the sliding window */
		payload_index = lll->payload_tail + payload_offset;
		if (payload_index >= lll->payload_count_max) {
			payload_index -= lll->payload_count_max;
		}

		/* Get reference to stream context */
		stream_handle = lll->stream_handle[lll->stream_curr];
		sync_stream = ull_sync_iso_lll_stream_get(stream_handle);

		/* Store the received PDU if selected stream and not already
		 * received (say in previous event as pre-transmitted PDU.
		 */
		if ((lll->bis_curr == sync_stream->bis_index) && pdu->len &&
		    !lll->payload[bis_idx][payload_index]) {
			uint16_t handle;

			if (IS_ENABLED(CONFIG_BT_CTLR_BROADCAST_ISO_ENC) &&
			    lll->enc) {
				uint32_t mic_failure;
				uint32_t done;

				done = radio_ccm_is_done();
				LL_ASSERT(done);

				mic_failure = !radio_ccm_mic_is_valid();
				LL_ASSERT(!mic_failure);
			}

			ull_iso_pdu_rx_alloc();

			handle = LL_BIS_SYNC_HANDLE_FROM_IDX(stream_handle);
			isr_rx_iso_data_valid(lll, handle, node_rx);

			lll->payload[bis_idx][payload_index] = node_rx;
		}
	}

isr_rx_done:
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

	new_burst = 0U;
	skipped = 0U;

isr_rx_find_subevent:
	/* FIXME: Sequential or Interleaved BIS subevents decision */
	/* NOTE: below code is for Sequential Rx only */

	/* Find the next (bn_curr)th subevent to receive PDU */
	while (lll->bn_curr < lll->bn) {
		uint8_t payload_offset;
		uint8_t payload_index;

		/* Next burst number to check for reception required */
		lll->bn_curr++;

		/* Check payload buffer overflow */
		payload_offset = (lll->bn_curr - 1U);
		if (payload_offset > lll->payload_count_max) {
			/* (bn_curr)th Rx PDU skip subevent */
			skipped++;

			continue;
		}

		/* Find the index of the (bn_curr)th Rx PDU buffer */
		payload_index = lll->payload_tail + payload_offset;
		if (payload_index >= lll->payload_count_max) {
			payload_index -= lll->payload_count_max;
		}

		/* Check if (bn_curr)th Rx PDU has been received */
		if (!lll->payload[bis_idx][payload_index]) {
			/* Receive the (bn_curr)th Rx PDU of bis_curr */
			bis = lll->bis_curr;

			goto isr_rx_next_subevent;
		}

		/* (bn_curr)th Rx PDU already received, skip subevent */
		skipped++;
	}

	/* Find the next repetition (irc_curr)th subevent to receive PDU */
	if (lll->irc_curr < lll->irc) {
		if (!new_burst) {
			uint8_t payload_index;

			/* Increment to next repetition count and be at first
			 * burst count for it.
			 */
			lll->bn_curr = 1U;
			lll->irc_curr++;

			/* Find the index of the (irc_curr)th bn = 1 Rx PDU
			 * buffer.
			 */
			payload_index = lll->payload_tail;

			/* Check if (irc_curr)th bn = 1 Rx PDU has been
			 * received.
			 */
			if (!lll->payload[bis_idx][payload_index]) {
				/* Receive the (irc_curr)th bn = 1 Rx PDU of
				 * bis_curr.
				 */
				bis = lll->bis_curr;

				goto isr_rx_next_subevent;
			} else {
				/* bn = 1 Rx PDU already received, skip
				 * subevent.
				 */
				skipped++;

				/* flag to skip successive repetitions if all
				 * bn PDUs have been received. i.e. the bn
				 * loop above did not find a PDU to be received.
				 */
				new_burst = 1U;

				/* Find the missing (bn_curr)th Rx PDU of
				 * bis_curr
				 */
				goto isr_rx_find_subevent;
			}
		} else {
			/* Skip all successive repetition reception as all
			 * bn PDUs have been received.
			 */
			skipped += (lll->irc - lll->irc_curr) * lll->bn;
			lll->irc_curr = lll->irc;
		}
	}

	/* Next pre-transmission subevent */
	if (lll->ptc_curr < lll->ptc) {
		lll->ptc_curr++;

		/* TODO: optimize to skip pre-transmission subevent in case
		 * of insufficient buffers in sliding window.
		 */

		/* Receive the (ptc_curr)th Rx PDU of bis_curr */
		bis = lll->bis_curr;

		goto isr_rx_next_subevent;
	}

	/* Next BIS */
	if (lll->bis_curr < lll->num_bis) {
		const uint8_t stream_curr = lll->stream_curr + 1U;
		struct lll_sync_iso_stream *sync_stream;
		uint16_t stream_handle;

		/* Next selected stream */
		if (stream_curr < lll->stream_count) {
			lll->stream_curr = stream_curr;
			stream_handle = lll->stream_handle[lll->stream_curr];
			sync_stream = ull_sync_iso_lll_stream_get(stream_handle);
			if (sync_stream->bis_index <= lll->num_bis) {
				uint8_t payload_index;
				uint8_t bis_idx_new;

				lll->bis_curr = sync_stream->bis_index;
				lll->ptc_curr = 0U;
				lll->irc_curr = 1U;
				lll->bn_curr = 1U;

				/* new BIS index */
				bis_idx_new = lll->bis_curr - 1U;

				/* Find the index of the (irc_curr)th bn = 1 Rx
				 * PDU buffer.
				 */
				payload_index = lll->payload_tail;

				/* Check if (irc_curr)th bn = 1 Rx PDU has been
				 * received.
				 */
				if (!lll->payload[bis_idx_new][payload_index]) {
					/* bn = 1 Rx PDU not received */
					skipped = (bis_idx_new - bis_idx) *
						  ((lll->bn * lll->irc) +
						   lll->ptc);

					/* Receive the (irc_curr)th bn = 1 Rx
					 * PDU of bis_curr.
					 */
					bis = lll->bis_curr;

					goto isr_rx_next_subevent;
				} else {
					/* bn = 1 Rx PDU already received, skip
					 * subevent.
					 */
					skipped = ((bis_idx_new - bis_idx) *
						   ((lll->bn * lll->irc) +
						    lll->ptc)) + 1U;

					/* BIS index */
					bis_idx = lll->bis_curr - 1U;

					/* Find the missing (bn_curr)th Rx PDU
					 * of bis_curr
					 */
					goto isr_rx_find_subevent;
				}
			} else {
				lll->bis_curr = lll->num_bis;
			}
		} else {
			lll->bis_curr = lll->num_bis;
		}
	}

	/* Control subevent */
	if (!lll->ctrl && (lll->cssn_next != lll->cssn_curr)) {
		uint8_t pkt_flags;

		/* Receive the control PDU and close the BIG event
		 *  there after.
		 */
		lll->ctrl = 1U;

		/* control subevent to use bis = 0 and se_n = 1 */
		bis = 0U;

		/* Configure Radio to receive Control PDU that can have greater
		 * PDU length than max_pdu length.
		 */
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

		goto isr_rx_next_subevent;
	}

	isr_rx_done(param);

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_send();
	}

	return;

isr_rx_next_subevent:
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

	/* Set the channel to use */
	if (!bis) {
		const uint16_t event_counter =
				(lll->payload_count / lll->bn) - 1U;

		/* Calculate the radio channel to use for ISO event */
		data_chan_use = lll_chan_iso_event(event_counter, data_chan_id,
						   lll->data_chan_map,
						   lll->data_chan_count,
						   &lll->data_chan_prn_s,
						   &lll->data_chan_remap_idx);
	} else if (!skipped) {
		data_chan_use = lll->next_chan_use;
	} else {
		uint8_t bis_idx_new = lll->bis_curr - 1U;

		/* Initialise to avoid compile error */
		data_chan_use = 0U;

		if (bis_idx != bis_idx_new) {
			const uint16_t event_counter =
				(lll->payload_count / lll->bn) - 1U;

			/* Calculate the radio channel to use for next BIS */
			data_chan_use = lll_chan_iso_event(event_counter,
						data_chan_id,
						lll->data_chan_map,
						lll->data_chan_count,
						&lll->data_chan_prn_s,
						&lll->data_chan_remap_idx);

			skipped -= (bis_idx_new - bis_idx) *
				   ((lll->bn * lll->irc) + lll->ptc);
		}

		while (skipped--) {
			/* Calculate the radio channel to use for subevent */
			data_chan_use = lll_chan_iso_subevent(data_chan_id,
						lll->data_chan_map,
						lll->data_chan_count,
						&lll->data_chan_prn_s,
						&lll->data_chan_remap_idx);
		}
	}

	lll_chan_set(data_chan_use);

	/* Encryption */
	if (IS_ENABLED(CONFIG_BT_CTLR_BROADCAST_ISO_ENC) &&
	    lll->enc) {
		uint64_t payload_count;
		struct pdu_bis *pdu;

		payload_count = lll->payload_count - lll->bn;
		if (bis) {
			payload_count += (lll->bn_curr - 1U) +
					 (lll->ptc_curr * lll->pto);

			/* By design, there shall always be one free node rx
			 * available for setting up radio for new PDU reception.
			 */
			node_rx = ull_iso_pdu_rx_alloc_peek(1U);
			LL_ASSERT(node_rx);

			pdu = (void *)node_rx->pdu;
		} else {
			/* Use the dedicated Control PDU buffer */
			pdu = radio_pkt_big_ctrl_get();
		}

		lll->ccm_rx.counter = payload_count;

		(void)memcpy(lll->ccm_rx.iv, lll->giv, 4U);
		mem_xor_32(lll->ccm_rx.iv, lll->ccm_rx.iv, access_addr);

		radio_pkt_rx_set(radio_ccm_iso_rx_pkt_set(&lll->ccm_rx, lll->phy,
							  RADIO_PKT_CONF_PDU_TYPE_BIS,
							  pdu));

	} else {
		struct pdu_bis *pdu;

		if (bis) {
			/* By design, there shall always be one free node rx
			 * available for setting up radio for new PDU reception.
			 */
			node_rx = ull_iso_pdu_rx_alloc_peek(1U);
			LL_ASSERT(node_rx);

			pdu = (void *)node_rx->pdu;
		} else {
			/* Use the dedicated Control PDU buffer */
			pdu = radio_pkt_big_ctrl_get();
		}

		radio_pkt_rx_set(pdu);
	}

	radio_switch_complete_and_disable();

	/* PDU Header Complete TimeOut, calculate the absolute timeout in
	 * microseconds by when a PDU header is to be received for each
	 * subevent.
	 */
	stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
	nse = ((lll->bis_curr - stream->bis_index) *
	       ((lll->bn * lll->irc) + lll->ptc)) +
	      ((lll->irc_curr - 1U) * lll->bn) + (lll->bn_curr - 1U) +
	      lll->ptc_curr + lll->ctrl;
	hcto = lll->sub_interval * nse;

	if (trx_cnt) {
		/* Setup radio packet timer header complete timeout for
		 * subsequent subevent PDU.
		 */

		/* Calculate the radio start with consideration of the drift
		 * based on the access address capture timestamp.
		 * Listen early considering +/- 2 us active clock jitter, i.e.
		 * listen early by 4 us.
		 */
		hcto += radio_tmr_aa_restore();
		hcto -= radio_rx_chain_delay_get(lll->phy, PHY_FLAGS_S8);
		hcto -= addr_us_get(lll->phy);
		hcto -= radio_rx_ready_delay_get(lll->phy, PHY_FLAGS_S8);
		hcto -= (EVENT_CLOCK_JITTER_US << 1) * nse;

		start_us = hcto;
		hcto = radio_tmr_start_us(0U, start_us);
		/* FIXME: Assertion check disabled until investigation as to
		 *        why there is high ISR latency causing assertion here.
		 */
		/* LL_ASSERT(hcto == (start_us + 1U)); */

		/* Add 8 us * subevents so far, as radio was setup to listen
		 * 4 us early and subevents could have a 4 us drift each until
		 * the current subevent we are listening.
		 */
		hcto += (((EVENT_CLOCK_JITTER_US << 1) * nse) << 1) +
			RANGE_DELAY_US + HAL_RADIO_TMR_START_DELAY_US;
	} else {
		/* First subevent PDU was not received, hence setup radio packet
		 * timer header complete timeout from where the first subevent
		 * PDU which is the BIG event anchor point would have been
		 * received.
		 */
		hcto += radio_tmr_ready_restore();

		start_us = hcto;
		hcto = radio_tmr_start_us(0U, start_us);
		LL_ASSERT(hcto == (start_us + 1U));

		hcto += ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
			  lll->window_widening_event_us) << 1) +
			lll->window_size_event_us;
	}

	/* header complete timeout to consider the radio ready delay, chain
	 * delay and access address duration.
	 */
	hcto += radio_rx_ready_delay_get(lll->phy, PHY_FLAGS_S8);
	hcto += addr_us_get(lll->phy);
	hcto += radio_rx_chain_delay_get(lll->phy, PHY_FLAGS_S8);

	/* setup absolute PDU header reception timeout */
	radio_tmr_hcto_configure(hcto);

	/* setup capture of PDU end timestamp */
	radio_tmr_end_capture();

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(start_us +
				 radio_rx_ready_delay_get(lll->phy,
							  PHY_FLAGS_S8) -
				 HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

	/* Calculate ahead the next subevent channel index */
	const uint16_t event_counter = (lll->payload_count / lll->bn) - 1U;

	next_chan_calc(lll, event_counter, data_chan_id);

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_send();
	}
}

static void isr_rx_done(void *param)
{
	struct node_rx_pdu *node_rx;
	struct event_done_extra *e;
	struct lll_sync_iso *lll;
	uint8_t payload_index;
	uint8_t bis_idx;
	uint8_t bn;

	/* Enqueue PDUs to ULL */
	node_rx = NULL;
	lll = param;
	lll->stream_curr = 0U;
	payload_index = lll->payload_tail;
	for (bis_idx = 0U; bis_idx < lll->num_bis; bis_idx++) {
		struct lll_sync_iso_stream *stream;
		uint8_t payload_tail;
		uint8_t stream_curr;
		uint16_t stream_handle;

		stream_handle = lll->stream_handle[lll->stream_curr];
		stream = ull_sync_iso_lll_stream_get(stream_handle);
		/* Skip BIS indices not synchronized. bis_index is 0x01 to 0x1F,
		 * where as bis_idx is 0 indexed.
		 */
		if ((bis_idx + 1U) != stream->bis_index) {
			continue;
		}

		payload_tail = lll->payload_tail;
		bn = lll->bn;
		while (bn--) {
			if (lll->payload[bis_idx][payload_tail]) {
				node_rx = lll->payload[bis_idx][payload_tail];
				lll->payload[bis_idx][payload_tail] = NULL;

				iso_rx_put(node_rx->hdr.link, node_rx);
			} else {
				/* Check if there are 2 free rx buffers, one
				 * will be consumed to generate PDU with invalid
				 * status, and the other is to ensure a PDU can
				 * be setup for the radio DMA to receive in the
				 * next sub_interval/iso_interval.
				 */
				node_rx = ull_iso_pdu_rx_alloc_peek(2U);
				if (node_rx) {
					struct pdu_bis *pdu;
					uint16_t handle;

					ull_iso_pdu_rx_alloc();

					pdu = (void *)node_rx->pdu;
					pdu->ll_id = PDU_BIS_LLID_COMPLETE_END;
					pdu->len = 0U;

					handle = LL_BIS_SYNC_HANDLE_FROM_IDX(stream_handle);
					isr_rx_iso_data_invalid(lll, bn, handle,
								node_rx);

					iso_rx_put(node_rx->hdr.link, node_rx);
				}
			}

			payload_index = payload_tail + 1U;
			if (payload_index >= lll->payload_count_max) {
				payload_index = 0U;
			}
			payload_tail = payload_index;
		}

		stream_curr = lll->stream_curr + 1U;
		if (stream_curr < lll->stream_count) {
			lll->stream_curr = stream_curr;
		}
	}
	lll->payload_tail = payload_index;

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	if (node_rx) {
		iso_rx_sched();
	}
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

	e = ull_event_done_extra_get();
	LL_ASSERT(e);

	/* Check if BIG terminate procedure received */
	if (lll->term_reason) {
		e->type = EVENT_DONE_EXTRA_TYPE_SYNC_ISO_TERMINATE;

		goto isr_done_cleanup;

	/* Check if BIG Channel Map Update */
	} else if (lll->chm_chan_count) {
		const uint16_t event_counter = lll->payload_count / lll->bn;

		/* Bluetooth Core Specification v5.3 Vol 6, Part B,
		 * Section 5.5.2 BIG Control Procedures
		 *
		 * When a Synchronized Receiver receives such a PDU where
		 * (instant - bigEventCounter) mod 65536 is greater than or
		 * equal to 32767 (because the instant is in the past),
		 * the Link Layer may stop synchronization with the BIG.
		 */

		/* Note: We are not validating whether the control PDU was
		 * received after the instant but apply the new channel map.
		 * If the channel map was new at or after the instant and
		 * the channel at the event counter did not match then the
		 * control PDU would not have been received.
		 */
		if (((event_counter - lll->ctrl_instant) & 0xFFFF) <= 0x7FFF) {
			(void)memcpy(lll->data_chan_map, lll->chm_chan_map,
				     sizeof(lll->data_chan_map));
			lll->data_chan_count = lll->chm_chan_count;
			lll->chm_chan_count = 0U;
		}
	}

	/* Calculate and place the drift information in done event */
	e->type = EVENT_DONE_EXTRA_TYPE_SYNC_ISO;
	e->trx_cnt = trx_cnt;
	e->crc_valid = crc_ok_anchor;

	if (trx_cnt) {
		e->drift.preamble_to_addr_us = addr_us_get(lll->phy);
		e->drift.start_to_address_actual_us =
			radio_tmr_aa_restore() - radio_tmr_ready_restore();
		e->drift.window_widening_event_us =
			lll->window_widening_event_us;

		/* Reset window widening, as anchor point sync-ed */
		lll->window_widening_event_us = 0U;
		lll->window_size_event_us = 0U;
	}

isr_done_cleanup:
	lll_isr_cleanup(param);
}

static void isr_done(void *param)
{
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_latency_capture();
	}

	lll_isr_status_reset();

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_cputime_capture();
	}

	isr_rx_done(param);

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_send();
	}
}

static void next_chan_calc(struct lll_sync_iso *lll, uint16_t event_counter,
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

static void isr_rx_iso_data_valid(const struct lll_sync_iso *const lll,
				  uint16_t handle, struct node_rx_pdu *node_rx)
{
	struct lll_sync_iso_stream *stream;
	struct node_rx_iso_meta *iso_meta;

	node_rx->hdr.type = NODE_RX_TYPE_ISO_PDU;
	node_rx->hdr.handle = handle;

	iso_meta = &node_rx->rx_iso_meta;
	iso_meta->payload_number = lll->payload_count + (lll->bn_curr - 1U) +
				   (lll->ptc_curr * lll->pto) - lll->bn;

	stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
	iso_meta->timestamp = HAL_TICKER_TICKS_TO_US(radio_tmr_start_get()) +
			      radio_tmr_aa_restore() +
			      (DIV_ROUND_UP(lll->ptc_curr, lll->bn) *
			       lll->pto * lll->iso_interval *
			       PERIODIC_INT_UNIT_US) -
			      addr_us_get(lll->phy) -
			      ((stream->bis_index - 1U) *
			       lll->sub_interval * ((lll->irc * lll->bn) +
						    lll->ptc));
	iso_meta->timestamp %=
		HAL_TICKER_TICKS_TO_US(BIT(HAL_TICKER_CNTR_MSBIT + 1U));
	iso_meta->status = 0U;
}

static void isr_rx_iso_data_invalid(const struct lll_sync_iso *const lll,
				    uint8_t bn, uint16_t handle,
				    struct node_rx_pdu *node_rx)
{
	struct lll_sync_iso_stream *stream;
	struct node_rx_iso_meta *iso_meta;

	node_rx->hdr.type = NODE_RX_TYPE_ISO_PDU;
	node_rx->hdr.handle = handle;

	iso_meta = &node_rx->rx_iso_meta;
	iso_meta->payload_number = lll->payload_count - bn - 1U;

	stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
	iso_meta->timestamp = HAL_TICKER_TICKS_TO_US(radio_tmr_start_get()) +
			      radio_tmr_aa_restore() - addr_us_get(lll->phy) -
			      ((stream->bis_index - 1U) *
			       lll->sub_interval * ((lll->irc * lll->bn) +
						    lll->ptc));
	iso_meta->timestamp %=
		HAL_TICKER_TICKS_TO_US(BIT(HAL_TICKER_CNTR_MSBIT + 1U));
	iso_meta->status = 1U;
}

static void isr_rx_ctrl_recv(struct lll_sync_iso *lll, struct pdu_bis *pdu)
{
	const uint8_t opcode = pdu->ctrl.opcode;

	if (opcode == PDU_BIG_CTRL_TYPE_TERM_IND) {
		if (!lll->term_reason) {
			struct pdu_big_ctrl_term_ind *term;

			term = (void *)&pdu->ctrl.term_ind;
			lll->term_reason = term->reason;
			lll->ctrl_instant = term->instant;
		}
	} else if (opcode == PDU_BIG_CTRL_TYPE_CHAN_MAP_IND) {
		if (!lll->chm_chan_count) {
			struct pdu_big_ctrl_chan_map_ind *chm;
			uint8_t chan_count;

			chm = (void *)&pdu->ctrl.chan_map_ind;
			chan_count =
				util_ones_count_get(chm->chm, sizeof(chm->chm));
			if (chan_count >= CHM_USED_COUNT_MIN) {
				lll->chm_chan_count = chan_count;
				(void)memcpy(lll->chm_chan_map, chm->chm,
					     sizeof(lll->chm_chan_map));
				lll->ctrl_instant = chm->instant;
			}
		}
	} else {
		/* Unknown control PDU, ignore */
	}
}
