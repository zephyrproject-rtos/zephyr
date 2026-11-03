/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci_types.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_chan.h"
#include "lll_scan.h"
#include "lll_df_types.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"

#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_prof_internal.h"

#include "hal/debug.h"

#define CTRL_CHAN_NONE  0xFFU
#define CTRL_CHAN_DEFER 0xFEU

static int init_reset(void);
static int create_prepare_cb(struct lll_prepare_param *p);
static int prepare_cb(struct lll_prepare_param *p);
static int prepare_cb_common(struct lll_prepare_param *p, bool is_resort);
static int create_is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb);
static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb);
#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
static int resume_prepare_cb(struct lll_prepare_param *p);
static bool is_yield_to_ready(struct lll_sync_iso *lll, void *next);
static struct lll_event *is_yield_to_pending(struct lll_sync_iso *lll);
static bool is_gap_available(struct lll_sync_iso *lll, uint32_t ticks_duration);
static bool is_yield_to_other(struct lll_sync_iso *lll);
static void isr_rx_yield(struct lll_sync_iso *lll, struct lll_event *e, uint8_t is_yield);
static struct lll_event *ready_before_get(const struct lll_sync_iso *lll,
					  uint32_t ticks_at_expire, uint32_t ticks_end);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */
static void abort_cb(struct lll_prepare_param *prepare_param, void *param);
static void isr_rx_estab(void *param);
static void isr_rx(void *param);
static void isr_rx_lll_done(void *param);
static void isr_done(void *param);
static uint16_t payload_index_get(const struct lll_sync_iso *lll);
static void ctrl_chan_calc(struct lll_sync_iso *lll, uint16_t event_counter);
#if defined(CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL)
static void next_chan_calc_seq(struct lll_sync_iso *lll, uint16_t event_counter,
			       uint16_t data_chan_id);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL */
#if defined(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED)
static void next_chan_calc_int(struct lll_sync_iso *lll,
			       uint16_t event_counter);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED */
static void isr_rx_iso_data_valid(const struct lll_sync_iso *const lll,
				  uint16_t handle, struct node_rx_pdu *node_rx);
static void isr_rx_iso_data_invalid(const struct lll_sync_iso *const lll,
				    uint16_t latency, uint8_t bn,
				    uint16_t handle,
				    struct node_rx_pdu *node_rx);
static void isr_rx_ctrl_recv(struct lll_sync_iso *lll, struct pdu_bis *pdu);
#if defined(CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX)
static uint8_t se_index_get(const struct lll_sync_iso *lll,
			    bool is_sequential_packing);
static void se_b2b_rx_arm(struct lll_sync_iso *lll, uint8_t se,
			  bool is_sequential_packing, bool is_chained);
static uint32_t se_b2b_rx_start_us(struct lll_sync_iso *lll, uint8_t se,
				   uint32_t start_us,
				   bool is_sequential_packing);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */

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
	int err;

	err = lll_hfclock_on();
	LL_ASSERT_ERR(err >= 0);

	err = lll_prepare(create_is_abort_cb, abort_cb, create_prepare_cb, 0U,
			  param);
	LL_ASSERT_ERR(err == 0 || err == -EINPROGRESS);
}

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
/* The ticker delayed the event to after the slot of an overlapping event. If the HF clock
 * is already running, start at the earliest subevent that can be received now instead of
 * after the HF clock startup time from the delayed expiry. If the prepare is late, move
 * the expiry to the earliest subevent that can still be received.
 */
static void prepare_early_start(struct lll_prepare_param *p, struct lll_sync_iso *lll)
{
	uint32_t ticks_at_expire_anchor;
	uint32_t ticks_at_expire;
	uint32_t ticks_diff;

	ticks_at_expire_anchor = ticker_ticks_diff_get(p->ticks_at_expire, p->ticks_drift);
	ticks_at_expire = ticker_ticks_diff_get(ticker_ticks_now_get(),
						lll_event_offset_get(HDR_LLL2ULL(lll)));

	ticks_diff = ticker_ticks_diff_get(ticks_at_expire, ticks_at_expire_anchor);
	if ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) != 0U) {
		ticks_at_expire = ticks_at_expire_anchor;
		ticks_diff = 0U;
	}

	if ((ticks_diff > p->ticks_drift) ||
	    ((ticks_diff < p->ticks_drift) && (lll->is_hfclock_ready != 0U))) {
		p->ticks_at_expire = ticks_at_expire;
		p->ticks_drift = ticks_diff;
	}
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

void lll_sync_iso_prepare(void *param)
{
	int err;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	struct lll_prepare_param *p = param;
	struct lll_sync_iso *lll = p->param;
	bool is_resume;

	/* A current or active radio event uses the HF clock, which then stays on for this
	 * event.
	 */
	lll->is_hfclock_ready = ((!lll_is_done(NULL, &is_resume)) ||
				 (lll_radio_is_idle() == 0U)) ? 1U : 0U;

	/* Start the preempt timeout for the earliest subevent that can be received */
	if (p->ticks_drift != 0U) {
		prepare_early_start(p, lll);
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	err = lll_hfclock_on();
	LL_ASSERT_ERR(err >= 0);

	err = lll_prepare(is_abort_cb, abort_cb, prepare_cb, 0U, param);
	LL_ASSERT_ERR(err == 0 || err == -EINPROGRESS);
}

void lll_sync_iso_flush(uint8_t handle, struct lll_sync_iso *lll)
{
	uint8_t stream_count;
	bool release = false;

	ARG_UNUSED(handle);

	stream_count = BT_CTLR_SYNC_ISO_STREAM_MAX;
	while (stream_count != 0U) {
		uint8_t payload_count;

		stream_count--;
		payload_count = lll->payload_count_max;
		while (payload_count != 0U) {
			struct node_rx_pdu *node_rx;

			payload_count--;
			node_rx = lll->payload[stream_count][payload_count];
			if (node_rx == NULL) {
				continue;
			}

			lll->payload[stream_count][payload_count] = NULL;

			node_rx->hdr.type = NODE_RX_TYPE_RELEASE;
			iso_rx_put(node_rx->hdr.link, node_rx);

			release = true;
		}
	}

	if (release) {
		iso_rx_sched();
	}
}

static int init_reset(void)
{
	return 0;
}

static int create_prepare_cb(struct lll_prepare_param *p)
{
	struct lll_sync_iso *lll;
	int err;

	lll = p->param;

	/* Calculate the current event latency */
	lll->lazy_prepare = p->lazy;
	lll->latency_event = lll->latency_prepare + lll->lazy_prepare;

	/* Update BIS packet counter to next value */
	lll->payload_count += (lll->latency_event + 1U) * lll->bn;

	/* Reset accumulated latencies */
	lll->latency_prepare = 0U;

	/* Accumulate window widening */
	lll->window_widening_prepare_us += lll->window_widening_periodic_us * lll->lazy_prepare;
	if (lll->window_widening_prepare_us > lll->window_widening_max_us) {
		lll->window_widening_prepare_us = lll->window_widening_max_us;
	}

	/* Current window widening */
	lll->window_widening_event_us += lll->window_widening_prepare_us;
	if (lll->window_widening_event_us > lll->window_widening_max_us) {
		lll->window_widening_event_us =	lll->window_widening_max_us;
	}

	/* Pre-increment window widening */
	lll->window_widening_prepare_us = lll->window_widening_periodic_us;

	/* Initialize trx chain count, retained across resume. */
	lll->trx_cnt = 0U;

	/* Initialize anchor point CRC ok flag, retained across resume */
	lll->crc_ok_anchor = 0U;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	/* Initialise resume subevent flag */
	lll->is_lll_resume = 0U;
	lll->is_yielded = 0U;

	/* Initialise the subevent to resume at */
	lll->se_next = 0U;
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	err = prepare_cb_common(p, false);
	if (err) {
		if (err == -EOVERFLOW) {
			err = 0;
		}

		DEBUG_RADIO_START_O(1);
		return err;
	}

	radio_isr_set(isr_rx_estab, p->param);

	DEBUG_RADIO_START_O(1);

	return 0;
}

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
/* Check if a resume of the previous event is pending in the prepare pipeline. The previous
 * event shall complete, generating its done event using the event state, before the current
 * event is prepared. The resume is placed first in the pipeline, as a normal prepare, and
 * the current event is deferred after it.
 */
static bool is_prepare_resume_pending(struct lll_prepare_param *p, lll_prepare_cb_t defer_cb)
{
	struct lll_sync_iso *lll = p->param;
	struct lll_event *iter;
	struct lll_event *e;
	void *idx;
	int err;

	idx = NULL;
	while ((e = ull_prepare_dequeue_iter(&idx)) != NULL) {
		if ((e->is_aborted == 0U) && (e->prepare_param.param == lll) &&
		    (e->prepare_cb == resume_prepare_cb)) {
			break;
		}

		if (idx == NULL) {
			e = NULL;
			break;
		}
	}

	if (e == NULL) {
		return false;
	}

	lll_defer(defer_cb, 0U, 0U, p->ticks_drift);

	/* Dequeue the prepare deferred, which was dequeued now, right after the resume of the
	 * previous event completes, and not after other prepares placed before it.
	 */
	idx = NULL;
	while ((iter = ull_prepare_dequeue_iter(&idx)) != NULL) {
		if ((iter->is_aborted == 0U) && (iter->is_resume == 0U) &&
		    (iter->prepare_param.param == lll) && (iter->prepare_cb == defer_cb)) {
			ull_prepare_dequeue_first_set(iter);

			break;
		}

		if (idx == NULL) {
			break;
		}
	}

	e->is_resume = 0U;
	e->is_yield = 0U;
	ull_prepare_dequeue_first_set(e);

	/* Retain HF clock, lll_isr_early_abort() turns it off */
	err = lll_hfclock_on();
	LL_ASSERT_ERR(err >= 0);

	radio_isr_set(lll_isr_early_abort, lll);
	radio_disable();

	return true;
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

static int prepare_cb(struct lll_prepare_param *p)
{
	struct lll_sync_iso *lll;
	int err;

	lll = p->param;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	if (is_prepare_resume_pending(p, prepare_cb)) {
		return 0;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	/* Calculate the current event latency */
	lll->lazy_prepare = p->lazy;
	lll->latency_event = lll->latency_prepare + lll->lazy_prepare;

	/* Update BIS packet counter to next value */
	lll->payload_count += (lll->latency_event + 1U) * lll->bn;

	/* Reset accumulated latencies */
	lll->latency_prepare = 0U;

	/* Accumulate window widening */
	lll->window_widening_prepare_us += lll->window_widening_periodic_us * lll->lazy_prepare;
	if (lll->window_widening_prepare_us > lll->window_widening_max_us) {
		lll->window_widening_prepare_us = lll->window_widening_max_us;
	}

	/* Current window widening */
	lll->window_widening_event_us += lll->window_widening_prepare_us;
	if (lll->window_widening_event_us > lll->window_widening_max_us) {
		lll->window_widening_event_us =	lll->window_widening_max_us;
	}

	/* Pre-increment window widening */
	lll->window_widening_prepare_us = lll->window_widening_periodic_us;

	/* Initialize trx chain count, retained across resume. */
	lll->trx_cnt = 0U;

	/* Initialize anchor point CRC ok flag, retained across resume */
	lll->crc_ok_anchor = 0U;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	/* Initialise resume subevent flag */
	lll->is_lll_resume = 0U;
	lll->is_yielded = 0U;

	/* Initialise the subevent to resume at */
	lll->se_next = 0U;

	/* Start at the earliest subevent that can be received now */
	prepare_early_start(p, lll);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	/* Initialize the last access address sync subevent */
	lll->se = 0U;
	lll->aa_se = 0U;

	err = prepare_cb_common(p, true);
	if (err) {
		if (err == -EOVERFLOW) {
			err = 0;
		}

		DEBUG_RADIO_START_O(1);
		return err;
	}

	radio_isr_set(isr_rx, p->param);

	DEBUG_RADIO_START_O(1);

	return 0;
}

static int prepare_cb_common(struct lll_prepare_param *p, bool is_resort)
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

	/* Check if stopped (on disconnection between prepare and preempt)
	 */
	if (unlikely(lll->is_lll_stop != 0U)) {
		radio_isr_set(lll_isr_early_abort, lll);
		radio_disable();

		return 0;
	}

	/* Calculate the current event counter value */
	event_counter = (lll->payload_count / lll->bn) - 1U;

	/* Initialize to mandatory parameter values */
	lll->bis_curr = 1U;
	lll->ptc_curr = 0U;
	lll->irc_curr = 1U;
	lll->bn_curr = 1U;

	/* Initialize control subevent flag */
	lll->ctrl = 0U;
	lll->ctrl_chan_use = CTRL_CHAN_NONE;

	/* Calculate the Access Address for the BIS event */
	util_bis_aa_le32(lll->bis_curr, lll->seed_access_addr, access_addr);
	data_chan_id = lll_chan_id(access_addr);

	/* Calculate the radio channel to use for ISO event and hence store the
	 * channel to be used for control subevent.
	 */
	data_chan_use = lll_chan_iso_event(event_counter, data_chan_id,
					   lll->data_chan_map,
					   lll->data_chan_count,
					   &lll->data_chan.prn_s,
					   &lll->data_chan.remap_idx);

	/* Initialize stream current */
	lll->stream_curr = 0U;

	/* Skip subevents until first selected BIS */
	stream_handle = lll->stream_handle[lll->stream_curr];
	stream = ull_sync_iso_lll_stream_get(stream_handle);
	if ((stream->bis_index != lll->bis_curr) &&
	    (stream->bis_index <= lll->num_bis)) {
		/* First selected BIS */
		lll->bis_curr = stream->bis_index;
	}

	const bool is_sequential_packing = (lll->bis_spacing >= (lll->sub_interval * lll->nse));

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	uint8_t skipped_bis;
	uint8_t skipped;

	if (p->ticks_drift != 0U) {
		uint32_t drift_us;

		/* FIXME: Add implementation to support interleaved packing BIG event drift and
		 *        resume.
		 */
		if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED) && !is_sequential_packing) {
			radio_isr_set(lll_isr_early_abort, lll);
			radio_disable();

			return -EOVERFLOW;
		}

		drift_us = HAL_TICKER_TICKS_TO_US(p->ticks_drift);

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
		uint32_t ticks_at_expire;

		ticks_at_expire = p->ticks_at_expire;
		remainder_us = p->remainder;
		hal_ticker_remove_jitter(&ticks_at_expire, &remainder_us);
		drift_us += HAL_TICKER_TICKS_TO_US(p->ticks_at_expire - ticks_at_expire) -
			    remainder_us;
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

		/* Skipped subevents in sequential packing since anchor point */
		skipped = DIV_ROUND_UP(drift_us, lll->sub_interval);
		skipped_bis = skipped / lll->nse;
		if (skipped_bis > 0U) {
			lll->stream_curr += skipped_bis;
			if (lll->stream_curr >= lll->stream_count) {
				if (lll->trx_cnt == 0U) {
					/* Increment abort count */
					if (lll->abort_count < UINT8_MAX) {
						lll->abort_count++;
					}
				} else {
					radio_tmr_start_save(lll->ticks_start);
					radio_tmr_ready_save(lll->ready_us);
					radio_tmr_aa_save(lll->aa_us);
				}

				/* Generate lost ISO data for the payloads of the event */
				radio_isr_set(isr_done, lll);

				radio_disable();

				return -EOVERFLOW;
			}

			lll->bis_curr += skipped_bis;
			LL_ASSERT_DBG(lll->bis_curr <= lll->num_bis);

			lll->bn_curr = 1U;
			lll->irc_curr = 1U;
			lll->ptc_curr = 0U;
		}

		lll->se = skipped;
		skipped %= lll->nse;

		/* Calculate the remainder drift for the current BIS subevent */
		drift_us %= lll->sub_interval;

		/* Calculate the offset to next BIS subevent for reception */
		if (drift_us != 0U) {
			drift_us = lll->sub_interval - drift_us;
		}

		/* A deferred or resumed event drifted to its next subevent can be later than a
		 * ready prepare in the pipeline, or be preempted by it before the subevent is
		 * received. Enqueue it back in the order of the drifted time, to resume at the
		 * subevent, so that the ready prepare uses the radio first.
		 */
		struct lll_event *ready = NULL;
		uint32_t ticks_resume = 0U;

		if (is_resort && (lll->se != 0U)) {
			uint32_t ticks_end;

			ticks_resume = HAL_TICKER_US_TO_TICKS((lll->se * lll->sub_interval) -
							      (lll->sub_interval >> 1));
			ticks_end = (p->ticks_at_expire +
				     HAL_TICKER_US_TO_TICKS(drift_us + EVENT_OVERHEAD_START_US +
							    EVENT_OVERHEAD_PREEMPT_MIN_US +
							    lll->sub_interval)) &
				    HAL_TICKER_CNTR_MASK;
			/* Compare against the pipeline sort key used by lll_defer() */
			ready = ready_before_get(lll, (p->ticks_at_expire - p->ticks_drift +
						       ticks_resume) & HAL_TICKER_CNTR_MASK,
						 ticks_end);
		}

		if (ready != NULL) {
			int err;

			lll->is_lll_resume = 1U;
			lll->se_next = lll->se;

			lll_defer(resume_prepare_cb, 0U, 0U, ticks_resume);

			/* Dequeue the ready prepare before the deferred current event */
			ull_prepare_dequeue_first_set(ready);

			/* Retain HF clock, lll_isr_early_abort() turns it off */
			err = lll_hfclock_on();
			LL_ASSERT_ERR(err >= 0);

			radio_isr_set(lll_isr_early_abort, lll);
			radio_disable();

			return -EOVERFLOW;
		}

		/* Drift to next BIS subevent */
		p->ticks_at_expire += HAL_TICKER_US_TO_TICKS(drift_us);

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
		p->remainder = HAL_TICKER_REMAINDER(drift_us);
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */
	} else {
		skipped_bis = 0U;
		skipped = 0U;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	if (lll->bis_curr != 1U) {
		/* Calculate the Access Address for the current BIS */
		util_bis_aa_le32(lll->bis_curr, lll->seed_access_addr,
				 access_addr);
		data_chan_id = lll_chan_id(access_addr);

		/* Calculate the channel id for the next BIS subevent */
		if (false) {

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL)
		} else if (is_sequential_packing) {
			data_chan_use = lll_chan_iso_event(event_counter,
							   data_chan_id,
							   lll->data_chan_map,
							   lll->data_chan_count,
							   &lll->data_chan.prn_s,
							   &lll->data_chan.remap_idx);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL */

#if defined(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED)
		} else if (!is_sequential_packing) {
			struct lll_sync_iso_data_chan_interleaved *interleaved_data_chan;

			interleaved_data_chan =
				&lll->interleaved_data_chan[lll->bis_curr - 1U];

			data_chan_use = lll_chan_iso_event(event_counter,
							   data_chan_id,
							   lll->data_chan_map,
							   lll->data_chan_count,
							   &interleaved_data_chan->prn_s,
							   &interleaved_data_chan->remap_idx);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED */

		} else {
			LL_ASSERT_DBG(false);
		}
	}

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	/* Advance the channel calculation past skipped subevents */
	if (skipped != 0U) {
		uint8_t bn_irc;

		/* Calculate the correct bn_curr, irc_curr, ptc_curr for the
		 * skipped subevent position within the current BIS.
		 * Subevent layout per BIS: BN*IRC data subevents then PTC
		 * pre-transmission subevents.
		 */
		bn_irc = lll->bn * lll->irc;
		if (skipped < bn_irc) {
			lll->bn_curr = (skipped % lll->bn) + 1U;
			lll->irc_curr = (skipped / lll->bn) + 1U;
			lll->ptc_curr = 0U;
		} else {
			lll->bn_curr = lll->bn;
			lll->irc_curr = lll->irc;
			lll->ptc_curr = skipped - bn_irc + 1U;
		}

		/* Calculate the radio channel to use for subevent */
		uint8_t skip = skipped;

		while (skip != 0U) {
			skip--;

			data_chan_use = lll_chan_iso_subevent(data_chan_id,
						lll->data_chan_map,
						lll->data_chan_count,
						&lll->data_chan.prn_s,
						&lll->data_chan.remap_idx);
		}
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

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
	LL_ASSERT_DBG(node_rx);

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

#if !defined(CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX)
	radio_switch_complete_and_disable();
#endif /* !CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */

	ticks_at_event = p->ticks_at_expire;
	ull = HDR_LLL2ULL(lll);
	ticks_at_event += lll_event_offset_get(ull);

	ticks_at_start = ticks_at_event;
	ticks_at_start += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	lll->ticks_se_first = ticker_ticks_diff_get(ticks_at_start, p->ticks_drift);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	remainder = p->remainder;
	remainder_us = radio_tmr_start(0U, ticks_at_start, remainder);

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX)
	/* Pre-arm the subevent after the first subevent being received, after
	 * radio_tmr_start() as it restores the END based SW switch timer clear.
	 */
	se_b2b_rx_arm(lll, se_index_get(lll, is_sequential_packing),
		      is_sequential_packing, false);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */

	if (false) {

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	} else if (lll->trx_cnt != 0U) {
		radio_tmr_start_save(lll->ticks_start);
		radio_tmr_ready_save(lll->ready_us);
		radio_tmr_aa_save(lll->aa_us);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	} else {
		uint32_t ticks_start;

		ticks_start = radio_tmr_start_get();
		radio_tmr_start_save(ticks_start);
		radio_tmr_ready_save(remainder_us);
		radio_tmr_aa_save(0U);
	}

	hcto = remainder_us +
	       ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
		 lll->window_widening_event_us) << 1) +
	       lll->window_size_event_us;

	const uint32_t rx_ready_delay = radio_rx_ready_delay_get(lll->phy, PHY_FLAGS_S8);

	hcto += rx_ready_delay;
	hcto += addr_us_get(lll->phy);
	hcto += radio_rx_address_delay_get(lll->phy, PHY_FLAGS_S8);
	radio_tmr_hcto_configure(hcto);

	/* Setup Access Address capture for subsequent subevents */
	radio_tmr_aa_capture();

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* setup capture of PDU end timestamp */
		radio_tmr_end_capture();
	}

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(remainder_us + rx_ready_delay - HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED) && \
	(EVENT_OVERHEAD_PREEMPT_US <= EVENT_OVERHEAD_PREEMPT_MIN_US)
	uint32_t overhead;

	overhead = lll_preempt_calc(ull, (TICKER_ID_SCAN_SYNC_ISO_BASE +
					  ull_sync_iso_lll_index_get(lll)), ticks_at_event);
	/* check if preempt to start has changed */
	if (unlikely(overhead)) {
		int err;

		/* We accept the overlap if previous event elected to continue */
		if (p->defer == 1U) {
			err = 0;
		} else {
			LL_ASSERT_OVERHEAD(overhead);

			err = -ECANCELED;
		}

		radio_isr_set(lll_isr_abort, lll);
		radio_disable();

		return err;
	}
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */

	ret = lll_prepare_done(lll);
	LL_ASSERT_ERR(!ret);

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	/* Reset resume subevent flag */
	lll->is_lll_resume = 0U;
	lll->is_yielded = 0U;
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	/* Calculate ahead the next subevent channel index */
	if (false) {

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL)
	} else if (is_sequential_packing) {
		next_chan_calc_seq(lll, event_counter, data_chan_id);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL */

#if defined(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED)
	} else if (!is_sequential_packing) {
		struct lll_sync_iso_data_chan_interleaved *interleaved_data_chan;

		interleaved_data_chan =
			&lll->interleaved_data_chan[lll->bis_curr - 1U];
		interleaved_data_chan->id = data_chan_id;

		next_chan_calc_int(lll, event_counter);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED */

	} else {
		lll->ctrl_chan_use = CTRL_CHAN_DEFER;

		LL_ASSERT_DBG(false);
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
static int drift_prepare_cb(struct lll_prepare_param *p)
{
	struct lll_sync_iso *lll;
	uint32_t ticks_at_expire;
	uint32_t ticks_offset;
	uint32_t ticks_diff;
	struct ull_hdr *ull;
	uint32_t ticks_now;
	int err;

	LL_ASSERT_DBG(p->defer == 1U);

	lll = p->param;

	if (is_prepare_resume_pending(p, drift_prepare_cb)) {
		return 0;
	}

	ull = HDR_LLL2ULL(lll);
	ticks_offset = lll_event_offset_get(ull);
	ticks_offset += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US - EVENT_OVERHEAD_RESUME_US);
	ticks_now = ticker_ticks_now_get();

	ticks_at_expire = ticker_ticks_diff_get(ticks_now, ticks_offset);
	ticks_diff = ticker_ticks_diff_get(ticks_at_expire, p->ticks_at_expire);
	if ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) {
		uint32_t ticks_at_expire_prev;

		ticks_at_expire_prev = p->ticks_at_expire;
		p->ticks_at_expire = ticks_at_expire;
		p->ticks_drift += ticker_ticks_diff_get(p->ticks_at_expire, ticks_at_expire_prev);
	}

	/* Calculate the current event latency */
	lll->lazy_prepare = p->lazy;
	lll->latency_event = lll->latency_prepare + lll->lazy_prepare;

	/* Update BIS packet counter to next value */
	lll->payload_count += (lll->latency_event + 1U) * lll->bn;

	/* Reset accumulated latencies */
	lll->latency_prepare = 0U;

	/* Accumulate window widening */
	lll->window_widening_prepare_us += lll->window_widening_periodic_us * lll->lazy_prepare;
	if (lll->window_widening_prepare_us > lll->window_widening_max_us) {
		lll->window_widening_prepare_us = lll->window_widening_max_us;
	}

	/* Current window widening */
	lll->window_widening_event_us += lll->window_widening_prepare_us;
	if (lll->window_widening_event_us > lll->window_widening_max_us) {
		lll->window_widening_event_us =	lll->window_widening_max_us;
	}

	/* Pre-increment window widening */
	lll->window_widening_prepare_us = lll->window_widening_periodic_us;

	/* Initialize trx chain count, retained across resume. */
	lll->trx_cnt = 0U;

	/* Initialize anchor point CRC ok flag, retained across resume */
	lll->crc_ok_anchor = 0U;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	/* Initialise resume subevent flag */
	lll->is_lll_resume = 0U;
	lll->is_yielded = 0U;

	/* Initialise the subevent to resume at */
	lll->se_next = 0U;
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	/* Initialize the last access address sync subevent */
	lll->se = 0U;
	lll->aa_se = 0U;

	err = prepare_cb_common(p, true);
	if (err) {
		if (err == -EOVERFLOW) {
			err = 0;
		}

		DEBUG_RADIO_START_O(1);
		return err;
	}

	radio_isr_set(isr_rx, lll);

	return err;
}

static int resume_prepare_cb(struct lll_prepare_param *p)
{
	struct lll_sync_iso *lll;
	uint32_t ticks_at_expire;
	uint32_t ticks_offset;
	uint32_t ticks_diff;
	struct ull_hdr *ull;
	uint32_t ticks_now;
	int err;

	lll = p->param;

	ull = HDR_LLL2ULL(lll);
	ticks_offset = lll_event_offset_get(ull);
	ticks_offset += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US - EVENT_OVERHEAD_RESUME_US);
	ticks_now = ticker_ticks_now_get();

	ticks_at_expire = ticker_ticks_diff_get(ticks_now, ticks_offset);
	ticks_diff = ticker_ticks_diff_get(ticks_at_expire, p->ticks_at_expire);
	if ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) {
		uint32_t ticks_at_expire_prev;

		ticks_at_expire_prev = p->ticks_at_expire;
		p->ticks_at_expire = ticks_at_expire;
		p->ticks_drift += ticker_ticks_diff_get(p->ticks_at_expire, ticks_at_expire_prev);
	}

	/* Resume at the subevent that was to be received when the event yielded, if it is
	 * later than the next subevent that can be received now. Half a sub_interval before
	 * the subevent is used so that the drift calculation rounds up to this subevent.
	 */
	if (lll->se_next != 0U) {
		uint32_t ticks_at_expire_prev;

		ticks_at_expire = ticker_ticks_diff_get(p->ticks_at_expire, p->ticks_drift);
		ticks_at_expire += HAL_TICKER_US_TO_TICKS((lll->se_next * lll->sub_interval) -
							  (lll->sub_interval >> 1));
		ticks_at_expire &= HAL_TICKER_CNTR_MASK;

		ticks_diff = ticker_ticks_diff_get(ticks_at_expire, p->ticks_at_expire);
		if ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) {
			ticks_at_expire_prev = p->ticks_at_expire;
			p->ticks_at_expire = ticks_at_expire;
			p->ticks_drift += ticker_ticks_diff_get(p->ticks_at_expire,
								ticks_at_expire_prev);
		}
	}

	err = prepare_cb_common(p, true);
	if (!err) {
		radio_isr_set(isr_rx, lll);
	} else if (err == -EOVERFLOW) {
		err = 0;
	}

	return err;
}

/* Get the first ready prepare in the pipeline, that is not a resume, a deferred nor a yielded
 * prepare, if it is due before the given ticks_at_expire. Scan and scan aux prepares are not
 * considered, as the current event does not yield to them, refer to is_abort_cb(). ISO sync
 * prepares are not considered, these are ordered by the subevent yield, refer to is_yield().
 *
 * A ready prepare, other than a periodic sync prepare, is also returned if it is due before
 * ticks_end, i.e. its preempt timeout is before the end of the subevent to be received; the
 * current event would be preempted before it receives the subevent. A periodic sync prepare
 * does not preempt the current event while it has payload to receive, refer to is_abort_cb().
 */
static struct lll_event *ready_before_get(const struct lll_sync_iso *lll,
					  uint32_t ticks_at_expire, uint32_t ticks_end)
{
	struct lll_event *e;
	uint32_t ticks_diff;
	void *idx;

	idx = NULL;
	while ((e = ull_prepare_dequeue_iter(&idx)) != NULL) {
		if ((e->is_aborted == 0U) && (e->is_resume == 0U) &&
		    (e->prepare_param.defer == 0U)) {
			break;
		}

		if (idx == NULL) {
			e = NULL;
			break;
		}
	}

	if ((e == NULL) || (e->is_yield != 0U) ||
	    (ull_sync_iso_lll_is_valid_get(e->prepare_param.param) != NULL) ||
	    (ull_scan_lll_is_valid_get(e->prepare_param.param) != NULL) ||
	    (ull_scan_aux_lll_is_valid_get(e->prepare_param.param) != NULL)) {
		return NULL;
	}

	ticks_diff = ticker_ticks_diff_get(ticks_at_expire, e->prepare_param.ticks_at_expire);
	if ((ticks_diff != 0U) && ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U)) {
		return e;
	}

	if (ull_sync_lll_is_valid_get(e->prepare_param.param) != NULL) {
		return NULL;
	}

	ticks_diff = ticker_ticks_diff_get(ticks_end, e->prepare_param.ticks_at_expire);
	if ((ticks_diff != 0U) && ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U)) {
		return e;
	}

	return NULL;
}

/* Radio start ticks of the next subevent to be received by an event in the prepare pipeline */
static uint32_t pending_ticks_se_get(const struct lll_event *e, uint8_t *se_pending)
{
	const struct lll_sync_iso *lll = e->prepare_param.param;
	uint32_t ticks;
	uint32_t us;
	uint8_t se;

	/* Ticks at expire of the event before any drift */
	ticks = ticker_ticks_diff_get(e->prepare_param.ticks_at_expire,
				      e->prepare_param.ticks_drift);
	ticks += lll_event_offset_get(HDR_LLL2ULL(lll));

	/* First subevent at or after the drift */
	se = DIV_ROUND_UP(HAL_TICKER_TICKS_TO_US(e->prepare_param.ticks_drift),
			  lll->sub_interval);

	/* Yielded event resumes at its next subevent */
	if ((e->prepare_cb == resume_prepare_cb) && (lll->se_next > se)) {
		se = lll->se_next;
	}

	us = EVENT_OVERHEAD_START_US + (se * lll->sub_interval);
	ticks += HAL_TICKER_US_TO_TICKS(us);

	*se_pending = se;

	return ticks & HAL_TICKER_CNTR_MASK;
}

/* Radio start ticks of the next subevent, that can be started after the given duration from
 * now, to be received by an event in the prepare pipeline. Returns false if the event has no
 * such subevent remaining.
 */
static bool pending_ticks_se_after_get(const struct lll_event *e, uint32_t ticks_after,
				       uint32_t *ticks_se, uint8_t *se_pending)
{
	const struct lll_sync_iso *lll = e->prepare_param.param;
	uint32_t ticks_resume;
	uint32_t ticks_diff;
	uint32_t ticks_now;
	uint32_t ticks_si;
	uint32_t ticks;
	uint8_t se_max;
	uint8_t se;

	ticks = pending_ticks_se_get(e, &se);
	ticks_si = HAL_TICKER_US_TO_TICKS(lll->sub_interval);
	ticks_resume = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_RESUME_US);
	ticks_now = ticker_ticks_now_get() + ticks_after;
	se_max = lll->nse * lll->num_bis;

	ticks_diff = ticker_ticks_diff_get(ticks, ticks_now + ticks_resume);
	while (((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) != 0U) && (se < se_max)) {
		ticks += ticks_si;
		se++;

		ticks_diff = ticker_ticks_diff_get(ticks, ticks_now + ticks_resume);
	}

	*ticks_se = ticks & HAL_TICKER_CNTR_MASK;
	*se_pending = se;

	return se < se_max;
}

/* Radio start ticks of the next subevent, that can be started from now, to be received by an
 * event in the prepare pipeline. Returns false if the event has no such subevent remaining.
 */
static bool pending_ticks_se_now_get(const struct lll_event *e, uint32_t *ticks_se,
				     uint8_t *se_pending)
{
	return pending_ticks_se_after_get(e, 0U, ticks_se, se_pending);
}

/* Check if an event in the prepare pipeline has no subevent remaining to be received */
static bool is_pending_done(const struct lll_event *e)
{
	uint32_t ticks_se;
	uint8_t se;

	if (ull_sync_iso_lll_is_valid_get(e->prepare_param.param) == NULL) {
		return false;
	}

	return !pending_ticks_se_now_get(e, &ticks_se, &se);
}

/* Check if the next subevent that can be received from now by an event in the prepare
 * pipeline is before that of another event in the prepare pipeline.
 */
static bool is_pending_se_before(const struct lll_event *e, const struct lll_event *e_other)
{
	uint32_t ticks_se_other;
	uint32_t ticks_diff;
	uint32_t ticks_se;
	uint8_t se;

	if (!pending_ticks_se_now_get(e, &ticks_se, &se)) {
		return false;
	}

	if (!pending_ticks_se_now_get(e_other, &ticks_se_other, &se)) {
		return true;
	}

	ticks_diff = ticker_ticks_diff_get(ticks_se_other, ticks_se);

	return (ticks_diff != 0U) && ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U);
}

/* Window widening that an event in the prepare pipeline uses to receive its next subevent.
 * A resume of an event that has received a subevent is anchored to its access address
 * capture, and has no window widening. Otherwise, it is the window widening accumulated
 * so far or, if not yet prepared, the window widening its prepare will accumulate, refer
 * to prepare_cb() and drift_prepare_cb().
 */
static uint32_t pending_ww_us_get(const struct lll_event *e)
{
	const struct lll_sync_iso *lll = e->prepare_param.param;
	uint32_t ww_prepare_us;
	uint32_t ww_us;

	if (e->prepare_cb == resume_prepare_cb) {
		if (lll->trx_cnt != 0U) {
			return 0U;
		}

		return lll->window_widening_event_us;
	}

	ww_prepare_us = lll->window_widening_prepare_us +
			(lll->window_widening_periodic_us * e->prepare_param.lazy);
	ww_prepare_us = MIN(ww_prepare_us, lll->window_widening_max_us);

	ww_us = lll->window_widening_event_us + ww_prepare_us;

	return MIN(ww_us, lll->window_widening_max_us);
}

/* Number of subevents remaining, after the given subevent, to receive the payload that the
 * given subevent carries. Pre-transmission subevents carry payloads that are repeated in
 * the next event, they are given the maximum count.
 */
static uint8_t se_remaining_get(const struct lll_sync_iso *lll, uint8_t se)
{
	uint8_t se_bis;

	se_bis = se % lll->nse;
	if (se_bis >= (lll->bn * lll->irc)) {
		return lll->irc;
	}

	return lll->irc - 1U - (se_bis / lll->bn);
}

/* Event latency of an event in the prepare pipeline. A resumed event has its latency
 * calculated when it was prepared, other events calculate it when prepared. When the
 * previous event is resumed later in the prepare pipeline, its payloads are not yet
 * dequeued, refer to isr_done(), and offset the payloads of the event.
 */
static uint16_t pending_latency_get(const struct lll_event *e)
{
	const struct lll_sync_iso *lll = e->prepare_param.param;
	struct lll_event *iter;
	uint16_t latency;
	void *idx;

	if (e->prepare_cb == resume_prepare_cb) {
		return lll->latency_event;
	}

	latency = lll->latency_prepare + e->prepare_param.lazy;

	idx = NULL;
	while ((iter = ull_prepare_dequeue_iter(&idx)) != NULL) {
		if ((iter != e) && (iter->is_aborted == 0U) &&
		    (iter->prepare_param.param == lll) &&
		    (iter->prepare_cb == resume_prepare_cb)) {
			return latency + lll->latency_event + 1U;
		}

		if (idx == NULL) {
			break;
		}
	}

	return latency;
}

/* Check if the payload of the given stream and burst number, in the event at the given
 * latency, has been received, say from a pre-transmission subevent of a previous event.
 */
static bool is_payload_received(const struct lll_sync_iso *lll, uint16_t latency,
				uint8_t stream_curr, uint8_t bn)
{
	uint32_t payload_offset;
	uint32_t payload_index;

	payload_offset = ((uint32_t)latency * lll->bn) + bn;
	if (payload_offset >= lll->payload_count_max) {
		return false;
	}

	payload_index = lll->payload_tail + payload_offset;
	if (payload_index >= lll->payload_count_max) {
		payload_index -= lll->payload_count_max;
	}

	return lll->payload[stream_curr][payload_index] != NULL;
}

/* Check if receiving from the subevent se_after instead of the subevent se, in the event at
 * the given latency, loses no payload. A payload is lost when its last repetition in the
 * selected BIS is skipped and it has not already been received. Pre-transmission subevents
 * carry payloads that are repeated in their own event, skipping them loses no payload.
 */
static bool is_se_skip_lossless(const struct lll_sync_iso *lll, uint16_t latency, uint8_t se,
				uint8_t se_after)
{
	const struct lll_sync_iso_stream *stream;
	uint8_t bis_first;

	stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
	bis_first = stream->bis_index;

	for (uint8_t stream_curr = 0U; stream_curr < lll->stream_count; stream_curr++) {
		uint16_t se_last;

		stream = ull_sync_iso_lll_stream_get(lll->stream_handle[stream_curr]);
		se_last = ((stream->bis_index - bis_first) * lll->nse) +
			  ((lll->irc - 1U) * lll->bn);

		for (uint8_t bn = 0U; bn < lll->bn; bn++) {
			if (((se_last + bn) >= se) && ((se_last + bn) < se_after) &&
			    !is_payload_received(lll, latency, stream_curr, bn)) {
				return false;
			}
		}
	}

	return true;
}

/* Check if the given subevent and the subsequent subevents of the event, at the given
 * latency, can be skipped without losing a payload.
 */
static bool is_se_optional(const struct lll_sync_iso *lll, uint16_t latency, uint8_t se)
{
	return is_se_skip_lossless(lll, latency, se, UINT8_MAX);
}

bool lll_sync_iso_is_defer_lossless(void *param, uint32_t ticks_duration)
{
	struct lll_sync_iso *lll;
	struct lll_event *e;
	uint16_t latency;
	uint32_t ticks_se;
	uint8_t se_after;
	void *idx;
	uint8_t se;

	lll = ull_sync_iso_lll_is_valid_get(param);
	if ((lll == NULL) || (lll->abort_count != 0U) || (ticks_duration == 0U)) {
		return false;
	}

	idx = NULL;
	while ((e = ull_prepare_dequeue_iter(&idx)) != NULL) {
		if ((e->is_aborted == 0U) && (e->prepare_param.param == lll)) {
			break;
		}

		if (idx == NULL) {
			return false;
		}
	}

	if ((e == NULL) || !pending_ticks_se_now_get(e, &ticks_se, &se)) {
		return false;
	}

	latency = pending_latency_get(e);
	if (is_se_optional(lll, latency, se)) {
		return true;
	}

	ticks_duration += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_RESUME_US);
	if (!pending_ticks_se_after_get(e, ticks_duration, &ticks_se, &se_after)) {
		return false;
	}

	return is_se_skip_lossless(lll, latency, se, se_after);
}

/* Time of the subevent, from now, of an event in the prepare pipeline after which a
 * payload that has not been received is lost, i.e. the deadline of the event.
 */
static bool pending_ticks_deadline_get(const struct lll_event *e, uint32_t *ticks_deadline)
{
	const struct lll_sync_iso *lll = e->prepare_param.param;
	uint16_t se_count;
	uint16_t latency;
	uint32_t ticks_se;
	uint8_t se;

	if (!pending_ticks_se_now_get(e, &ticks_se, &se)) {
		return false;
	}

	latency = pending_latency_get(e);
	se_count = (uint16_t)lll->num_bis * lll->nse;
	for (uint16_t se_last = se; se_last < se_count; se_last++) {
		if (!is_se_skip_lossless(lll, latency, se, se_last + 1U)) {
			*ticks_deadline = (ticks_se +
					   HAL_TICKER_US_TO_TICKS((se_last - se) *
								  lll->sub_interval)) &
					  HAL_TICKER_CNTR_MASK;

			return true;
		}
	}

	return false;
}

/* Check if an event in the prepare pipeline has its deadline before that of another event
 * in the prepare pipeline, an event with no deadline has no payload to be received.
 */
static bool is_pending_deadline_before(const struct lll_event *e,
				       const struct lll_event *e_other)
{
	uint32_t ticks_deadline_other;
	uint32_t ticks_deadline;
	uint32_t ticks_diff;

	if (!pending_ticks_deadline_get(e, &ticks_deadline)) {
		return false;
	}

	if (!pending_ticks_deadline_get(e_other, &ticks_deadline_other)) {
		return true;
	}

	ticks_diff = ticker_ticks_diff_get(ticks_deadline_other, ticks_deadline);

	return (ticks_diff != 0U) && ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U);
}

/* Check if an event in the prepare pipeline has its deadline before that of the current
 * event. The current event has no deadline when all its payloads have been received, say
 * from pre-transmission subevents of a previous event.
 */
static bool is_pending_deadline_before_curr(const struct lll_event *e,
					    const struct lll_sync_iso *lll)
{
	uint32_t ticks_deadline;
	uint32_t ticks_diff;
	uint16_t se_count;
	uint8_t se;

	if (!pending_ticks_deadline_get(e, &ticks_deadline)) {
		return false;
	}

	se = MAX(lll->se, lll->se_next);
	se_count = (uint16_t)lll->num_bis * lll->nse;
	for (uint16_t se_last = se; se_last < se_count; se_last++) {
		if (!is_se_skip_lossless(lll, lll->latency_event, se, se_last + 1U)) {
			uint32_t ticks_deadline_curr;

			ticks_deadline_curr = (lll->ticks_se_first +
					       HAL_TICKER_US_TO_TICKS(se_last *
								      lll->sub_interval)) &
					      HAL_TICKER_CNTR_MASK;
			ticks_diff = ticker_ticks_diff_get(ticks_deadline_curr, ticks_deadline);

			return (ticks_diff != 0U) &&
			       ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U);
		}
	}

	return true;
}

/* Check if the current event, waiting for its next subevent, shall yield to the other event
 * in the prepare pipeline. Yield when a subevent of the other event can be received before
 * the next subevent of the current event, considering the overhead to resume the current
 * event thereafter. When the other event's subevent conflicts with the next subevent of the
 * current event, including the overhead to start the other event thereafter, yield only if
 * the other event would otherwise lose a payload and the current event, resuming after the
 * other event's subevent, does not. If both would lose a payload, yield when the other event
 * has fewer remaining subevents to receive its payload.
 */
static bool is_yield(const struct lll_sync_iso *lll, const struct lll_event *e)
{
	const struct lll_sync_iso *lll_other = e->prepare_param.param;
	uint32_t ticks_end_other;
	uint32_t ticks_se_after;
	uint32_t ticks_se_other;
	uint32_t ticks_si_other;
	uint32_t ticks_resume;
	uint32_t ticks_after;
	uint32_t ticks_diff;
	uint32_t ticks_end;
	uint8_t se_other_after;
	uint8_t se_resume;
	uint8_t se_other;

	if (!pending_ticks_se_now_get(e, &ticks_se_other, &se_other)) {
		return false;
	}

	/* Other event's subevent including its receive window widening. Its radio start is
	 * already early by the window widening, as the ticker interval is reduced by the
	 * periodic window widening, refer to ull_sync_iso_setup().
	 */
	ticks_si_other = HAL_TICKER_US_TO_TICKS(lll_other->sub_interval +
						(pending_ww_us_get(e) << 1));
	ticks_resume = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_RESUME_US);

	/* Other event's subevent, the overhead to resume this event and a margin of a tick
	 * on either side for the conversions to ticks.
	 */
	ticks_end_other = ticks_se_other + ticks_si_other + ticks_resume + 2U;
	ticks_end_other &= HAL_TICKER_CNTR_MASK;

	ticks_diff = ticker_ticks_diff_get(lll->ticks_se_next, ticks_end_other);
	if ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) {
		return true;
	}

	/* Other event's subevent starts after the next subevent of the current event and the
	 * overhead to start the other event thereafter.
	 */
	ticks_end = lll->ticks_se_next + HAL_TICKER_US_TO_TICKS(lll->sub_interval);
	ticks_end &= HAL_TICKER_CNTR_MASK;

	ticks_diff = ticker_ticks_diff_get(ticks_se_other, ticks_end + ticks_resume);
	if ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) {
		return false;
	}

	/* Other event's subevent that can be received after the next subevent of the current
	 * event, no payload lost by the other event when the current event does not yield.
	 */
	ticks_diff = ticker_ticks_diff_get(ticks_end, ticker_ticks_now_get());
	ticks_after = ((ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) ? ticks_diff : 0U;
	(void)pending_ticks_se_after_get(e, ticks_after, &ticks_se_after, &se_other_after);
	if (is_se_skip_lossless(lll_other, pending_latency_get(e), se_other, se_other_after)) {
		return false;
	}

	/* Current event's subevent to resume at after the other event's subevent, no payload
	 * lost by the current event when it yields.
	 */
	ticks_diff = ticker_ticks_diff_get(ticks_end_other, lll->ticks_se_next);
	se_resume = lll->se_next + DIV_ROUND_UP(HAL_TICKER_TICKS_TO_US(ticks_diff),
						lll->sub_interval);
	if (is_se_skip_lossless(lll, lll->latency_event, lll->se_next, se_resume)) {
		return true;
	}

	return se_remaining_get(lll_other, se_other) < se_remaining_get(lll, lll->se_next);
}

/* Ready event's prepare in the prepare pipeline that has a subevent remaining to be received */
static struct lll_event *ready_event_get(const struct lll_sync_iso *lll_next)
{
	struct lll_event *e;
	uint32_t ticks_se;
	void *idx;
	uint8_t se;

	idx = NULL;
	while ((e = ull_prepare_dequeue_iter(&idx)) != NULL) {
		if ((e->is_aborted == 0U) && (e->prepare_param.param == lll_next) &&
		    pending_ticks_se_now_get(e, &ticks_se, &se)) {
			return e;
		}

		if (idx == NULL) {
			break;
		}
	}

	return NULL;
}

/* Number of subevents remaining, after the next subevent that can be received from now, for
 * the ready event to receive the payload of that subevent. Returns UINT8_MAX if the ready
 * event is not in the prepare pipeline or has no subevent remaining.
 */
static uint8_t ready_se_remaining_get(const struct lll_sync_iso *lll_next, bool *is_yielded)
{
	struct lll_event *e;
	uint32_t ticks_se;
	void *idx;
	uint8_t se;

	idx = NULL;
	while ((e = ull_prepare_dequeue_iter(&idx)) != NULL) {
		if ((e->is_aborted == 0U) && (e->prepare_param.param == lll_next) &&
		    pending_ticks_se_now_get(e, &ticks_se, &se)) {
			/* A yielded event is enqueued as a normal, not deferred, resume */
			*is_yielded = (e->is_resume == 0U) && (e->prepare_param.defer == 0U) &&
				      (e->prepare_cb == resume_prepare_cb);

			return se_remaining_get(lll_next, se);
		}

		if (idx == NULL) {
			break;
		}
	}

	return UINT8_MAX;
}

/* Check if the current event, waiting for its next subevent, can yield to the ready event */
static bool is_yield_to_ready(struct lll_sync_iso *lll, void *next)
{
	struct lll_event *e;
	void *idx;

	if ((lll->trx_cnt == 0U) || (lll->se_next == 0U)) {
		return false;
	}

	idx = NULL;
	while ((e = ull_prepare_dequeue_iter(&idx)) != NULL) {
		/* Skip a resume of a previous event that has no subevent remaining */
		if ((e->is_aborted == 0U) && (e->prepare_param.param == next) &&
		    !is_pending_done(e)) {
			return is_yield(lll, e);
		}

		if (idx == NULL) {
			break;
		}
	}

	return false;
}

/* Check if the current event, waiting for its next subevent, can yield to the event that
 * will be dequeued next from the prepare pipeline, i.e. the first deferred or normal
 * prepare, else the first resume prepare. Returns the event yielded to.
 */
static struct lll_event *is_yield_to_pending(struct lll_sync_iso *lll)
{
	const struct lll_sync_iso *lll_pending;
	struct lll_event *resume_iso;
	struct lll_event *resume;
	struct lll_event *iter;
	struct lll_event *e;
	void *idx_next;
	void *idx;

	resume_iso = NULL;
	resume = NULL;
	idx_next = NULL;
	e = NULL;
	idx = NULL;
	while ((iter = ull_prepare_dequeue_iter(&idx)) != NULL) {
		/* Skip own prepares and prepares with no subevent remaining, a resume of a
		 * previous event completes without radio use.
		 */
		if ((iter->is_aborted == 0U) && (iter->prepare_param.param != lll) &&
		    !is_pending_done(iter)) {
			if (iter->is_resume == 0U) {
				if (e == NULL) {
					e = iter;
					idx_next = idx;
				}
			} else {
				if (resume == NULL) {
					resume = iter;
				}

				/* Prefer a resume of another ISO sync event that can receive its
				 * subevent before the next subevent of the current event, over
				 * resumes of other roles.
				 */
				if ((resume_iso == NULL) &&
				    (ull_sync_iso_lll_is_valid_get(iter->prepare_param.param) !=
				     NULL) &&
				    is_yield(lll, iter)) {
					resume_iso = iter;
				}
			}
		}

		if (idx == NULL) {
			break;
		}
	}

	/* Yield to the resume of another ISO sync event when no event is pending, or when the
	 * pending event is a prepare of another role.
	 */
	if ((resume_iso != NULL) &&
	    ((e == NULL) || (ull_sync_iso_lll_is_valid_get(e->prepare_param.param) == NULL))) {
		return resume_iso;
	}

	if (e == NULL) {
		e = resume;
	}

	if (e == NULL) {
		return NULL;
	}

	lll_pending = ull_sync_iso_lll_is_valid_get(e->prepare_param.param);
	if ((lll_pending == NULL) || (lll_pending == lll)) {
		return NULL;
	}

	if (is_yield(lll, e)) {
		struct lll_event *yield;

		/* Pipeline order is not the order of the deadlines, a subsequent deferred
		 * prepare of another ISO sync event can lose a payload earlier. Yield to the
		 * one with the earliest deadline.
		 */
		yield = e;
		idx = idx_next;
		while ((idx != NULL) && ((iter = ull_prepare_dequeue_iter(&idx)) != NULL)) {
			if ((iter->is_aborted == 0U) && (iter->is_resume == 0U) &&
			    (iter->prepare_param.defer != 0U) &&
			    (iter->prepare_param.param != lll) &&
			    (ull_sync_iso_lll_is_valid_get(iter->prepare_param.param) != NULL) &&
			    !is_pending_done(iter) && is_pending_deadline_before(iter, yield) &&
			    is_yield(lll, iter)) {
				yield = iter;
			}
		}

		/* A resume of another ISO sync event, preempted by another role, is dequeued
		 * after the pending prepares. Yield to the one whose subevent is earlier, so
		 * that ISO sync events yielding to each other do not starve the resume.
		 */
		if ((resume_iso != NULL) && is_pending_se_before(resume_iso, yield)) {
			return resume_iso;
		}

		return yield;
	}

	if (resume_iso != NULL) {
		return resume_iso;
	}

	/* Yield to a subsequent deferred prepare of another ISO sync event */
	if (e->is_resume != 0U) {
		return NULL;
	}

	idx = idx_next;
	while ((idx != NULL) && ((e = ull_prepare_dequeue_iter(&idx)) != NULL)) {
		if ((e->is_aborted == 0U) && (e->is_resume == 0U) &&
		    (e->prepare_param.defer != 0U) && (e->prepare_param.param != lll) &&
		    (ull_sync_iso_lll_is_valid_get(e->prepare_param.param) != NULL) &&
		    !is_pending_done(e) && is_yield(lll, e)) {
			return e;
		}
	}

	return NULL;
}

/* Place the prepare yielded to first in the prepare pipeline, preceded by the prepares of
 * events that have no subevent remaining, these complete without radio use and generate
 * their done events.
 */
static void yield_dequeue_first_set(struct lll_event *e)
{
	struct lll_event *iter;
	void *idx;

	ull_prepare_dequeue_first_set(e);

	/* Iterator is at the next prepare, hence the prepare iterated can be moved */
	idx = NULL;
	while ((iter = ull_prepare_dequeue_iter(&idx)) != NULL) {
		if ((iter != e) && (iter->is_aborted == 0U) && (iter->is_resume == 0U) &&
		    is_pending_done(iter)) {
			ull_prepare_dequeue_first_set(iter);
		}

		if (idx == NULL) {
			break;
		}
	}
}

/* Check if the current event, waiting for its next subevent, has a radio idle gap of the
 * given duration before its next subevent.
 */
static bool is_gap_available(struct lll_sync_iso *lll, uint32_t ticks_duration)
{
	uint32_t ticks_diff;
	uint32_t ticks_end;

	if ((lll->trx_cnt == 0U) || (lll->se_next == 0U)) {
		return false;
	}

	/* Duration, the overhead to resume and a margin of a tick on either side for the
	 * conversions to ticks.
	 */
	ticks_end = ticker_ticks_now_get() + ticks_duration +
		    HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_RESUME_US) + 2U;
	ticks_end &= HAL_TICKER_CNTR_MASK;

	ticks_diff = ticker_ticks_diff_get(lll->ticks_se_next, ticks_end);

	return (ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U;
}

/* Check if the current event, waiting for its next subevent, can yield to a resume prepare or
 * a deferred scan prepare in the prepare pipeline, that is not of an ISO sync event. The
 * prepare uses the radio until the preempt timeout of the yielded current event, i.e.
 * EVENT_OVERHEAD_START_US and half a sub_interval before its next subevent, and is to use
 * the radio for at least a sub_interval.
 */
static bool is_yield_to_other(struct lll_sync_iso *lll)
{
	uint32_t ticks_duration;
	struct lll_event *e;
	void *idx;

	ticks_duration = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_RESUME_US +
						EVENT_OVERHEAD_START_US +
						lll->sub_interval + (lll->sub_interval >> 1));
	if (!is_gap_available(lll, ticks_duration)) {
		return false;
	}

	idx = NULL;
	while ((e = ull_prepare_dequeue_iter(&idx)) != NULL) {
		if ((e->is_aborted == 0U) &&
		    (ull_sync_iso_lll_is_valid_get(e->prepare_param.param) == NULL) &&
		    ((e->is_resume != 0U) ||
		     ((e->prepare_param.defer != 0U) &&
		      (ull_scan_lll_is_valid_get(e->prepare_param.param) != NULL)))) {
			return true;
		}

		if (idx == NULL) {
			break;
		}
	}

	return false;
}

/* Close the radio use of the current event and place it back in the prepare pipeline to be
 * resumed at its next subevent, after the event yielded to which is a deferred or a resume
 * prepare.
 */
static void isr_rx_yield(struct lll_sync_iso *lll, struct lll_event *e, uint8_t is_yield)
{
	uint8_t is_resume;
	int err;

	is_resume = (e != NULL) ? e->is_resume : 0U;

	lll->ticks_start = radio_tmr_start_restore();
	lll->ready_us = radio_tmr_ready_restore();
	lll->aa_us = radio_tmr_aa_restore();

	lll->is_lll_resume = 1U;

	/* Yielding to a resume of another ISO sync event, the current event is placed as a
	 * yielded event that preempts at its next subevent, so that it is not queued behind
	 * resumes of other roles.
	 */
	if ((e != NULL) && (is_resume != 0U)) {
		is_resume = 0U;
		is_yield = 1U;
	}

	/* Place in the pipeline at the time to resume, half a sub_interval before the next
	 * subevent, as used by resume_prepare_cb().
	 */
	lll->is_yielded = is_yield;
	lll_defer(resume_prepare_cb, is_resume, is_yield,
		  HAL_TICKER_US_TO_TICKS((lll->se_next * lll->sub_interval) -
					 (lll->sub_interval >> 1)));

	/* Dequeue the prepare yielded to before the current event. A resume is dequeued after
	 * normal prepares, hence the resume yielded to is placed as normal prepare.
	 */
	if (e != NULL) {
		e->is_resume = 0U;
		e->is_yield = 0U;

		yield_dequeue_first_set(e);
	}

	/* Retain HF clock, lll_isr_cleanup() turns it off */
	err = lll_hfclock_on();
	LL_ASSERT_ERR(err >= 0);

	lll_isr_cleanup(lll);
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

static int create_is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb)
{
	if (next == NULL) {
		return -EBUSY;
	}

	if (next != curr) {
		struct lll_sync_iso *lll = curr;
		struct lll_sync_iso *lll_next;

		lll_next = ull_sync_iso_lll_is_valid_get(next);
		if ((lll_next == NULL) || (lll_next->abort_count <= lll->abort_count)) {
			return -EBUSY;
		}
	}

	return -ECANCELED;
}

static int is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb)
{
#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	if (next == NULL) {
		struct lll_sync_iso *lll = curr;

		/* Current event yields to this ready event */
		if (lll->is_yield_to != 0U) {
			lll->is_yield_to = 0U;
			lll->is_yielded = 0U;

			return 0;
		}

		/* A yielded event preempts at its next subevent, unless its remaining
		 * subevents carry no payload to be received, then it is enqueued back as
		 * deferred event to not cancel the current event.
		 */
		if ((lll->is_yielded != 0U) && (*resume_cb == resume_prepare_cb)) {
			lll->is_yielded = 0U;

			if (!is_se_optional(lll, lll->latency_event, lll->se_next)) {
				return 0;
			}

			lll->is_lll_resume = 1U;

			return -EAGAIN;
		}

		if (lll->abort_count != 0U) {
			return -EBUSY;
		}

		/* This role was is the ready (next) event, it did not preempt current event; this
		 * event can return 0 to preempt the current event as final decision. Or return
		 * -EAGAIN so that this ready event is placed as resume event. Or return -EBUSY so
		 *  that this ready event is placed as deferred event.
		 */

		/* A resume of a yielded event is enqueued back as is, otherwise the prepare is
		 * enqueued back with drift compensation.
		 */
		if (*resume_cb != resume_prepare_cb) {
			*resume_cb = drift_prepare_cb;
		}

		/* Drift prepare is not a resume, but we set the flag so that abort_cb will
		 * correctly retain the ULL prepare.
		 */
		lll->is_lll_resume = 1U;

		return -EAGAIN;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	if (next != curr) {
		struct lll_sync_iso *lll = curr;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
		struct lll_sync_iso *lll_next;
		struct lll_scan_aux *aux_lll;
		struct lll_scan *scan_lll;
		struct lll_sync *sync_lll;

		scan_lll = ull_scan_lll_is_valid_get(next);
		if (scan_lll) {
			/* Do not abort current ISO sync event as next event is a scan event, a
			 * continuous scan event is deferred and the current event yields to it
			 * when waiting for a subsequent subevent.
			 */
			return -EBUSY;
		}

		aux_lll = ull_scan_aux_lll_is_valid_get(next);
		if ((aux_lll != NULL) || (ull_sync_lll_is_valid_get(next) != NULL)) {
			/* Do not abort current ISO sync event as next event is a scan aux or a
			 * periodic sync event, unless the current event is waiting for its next
			 * subevent and the next event's radio use ends before it.
			 */
			struct ull_hdr *ull_next = HDR_LLL2ULL(next);

			/* No payload remains to be received, resume after the next event */
			if (is_se_optional(lll, lll->latency_event, MAX(lll->se, lll->se_next))) {
				*resume_cb = resume_prepare_cb;
				lll->is_lll_resume = 1U;

				return -EAGAIN;
			}

			if (ull_next->ticks_slot == 0U) {
				return -EBUSY;
			}

			if (is_gap_available(lll, ull_next->ticks_slot)) {
				*resume_cb = resume_prepare_cb;
				lll->is_lll_resume = 1U;

				return -EINPROGRESS;
			}

			/* Periodic sync, resume after it when the periodic sync is near its
			 * supervision timeout, otherwise it is received only in a gap or when
			 * the remaining subevents carry no payload to be received.
			 */
			sync_lll = ull_sync_lll_is_valid_get(next);
			if ((sync_lll != NULL) && (sync_lll->forced != 0U)) {
				*resume_cb = resume_prepare_cb;
				lll->is_lll_resume = 1U;

				return -EAGAIN;
			}

			return -EBUSY;
		}

		/* Setup the resume callback so that we yield now and be scheduled after the
		 * overlap finishes its radio use.
		 */
		*resume_cb = resume_prepare_cb;

		/* Resume prepare setup hence abort_cb() use is_lll_resume flag to retain the
		 * hfclock on.
		 */
		lll->is_lll_resume = 1U;

		/* Yield to the ready event if the current event is waiting for its next
		 * subevent and a subevent of the ready event can be received before it.
		 */
		lll_next = ull_sync_iso_lll_is_valid_get(next);
		if ((lll_next != NULL) && is_yield_to_ready(lll, next)) {
			lll_next->is_yield_to = 1U;

			return -EINPROGRESS;
		}

		/* Yield to the ready event when it loses a payload before the current event
		 * does, payloads already received, say from pre-transmission subevents of a
		 * previous event, have no deadline.
		 */
		if (lll_next != NULL) {
			struct lll_event *e_next;

			e_next = ready_event_get(lll_next);
			if ((e_next != NULL) && is_pending_deadline_before_curr(e_next, lll)) {
				return -EAGAIN;
			}
		}

		if (lll_next != NULL) {
			uint8_t se_rem_next;
			bool is_yielded;
			uint8_t se_rem;

			/* Continue the current event if it has fewer remaining subevents to
			 * receive its payload than the ready event, a pre-transmission subevent
			 * has the most. A yielded ready event is then enqueued back as deferred.
			 */
			is_yielded = false;
			se_rem_next = ready_se_remaining_get(lll_next, &is_yielded);
			se_rem = se_remaining_get(lll, MAX(lll->se, lll->se_next));
			if ((se_rem_next != UINT8_MAX) && (se_rem < se_rem_next)) {
				return is_yielded ? -EBUSY : -EINPROGRESS;
			}
		}

		if (lll->bn_curr <= lll->bn) {
			if ((lll_next == NULL) || (lll_next->abort_count <= lll->abort_count)) {
				return -EINPROGRESS;
			}
		}

		return -EAGAIN;

#else /* !CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */
		if (lll->bn_curr <= lll->bn) {
			return 0;
		}
#endif /* !CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */
	}

	return -ECANCELED;
}

static void abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	struct event_done_extra *e;
	struct lll_sync_iso *lll;
	int err;

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		lll = param;

		if (false) {

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
		} else if (lll->is_lll_resume != 0U) {
			lll->ticks_start = radio_tmr_start_restore();
			lll->ready_us = radio_tmr_ready_restore();
			lll->aa_us = radio_tmr_aa_restore();

			/* Retain HF clock */
			err = lll_hfclock_on();
			LL_ASSERT_ERR(err >= 0);

			radio_isr_set(lll_isr_abort, param);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

		} else {
			/* Increment abort count */
			if (lll->abort_count < UINT8_MAX) {
				lll->abort_count++;
			}

			radio_isr_set(isr_done, param);
		}

		radio_disable();

		if (IS_ENABLED(CONFIG_BT_CTLR_BROADCAST_ISO_ENC) && (lll->enc != 0U)) {
			radio_ccm_disable();
		}

		return;
	}

	/* Get reference to LLL connection context */
	lll = prepare_param->param;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	/* Being put back into the pipeline as drift prepare, keep the preparations */
	if (param && (lll->is_lll_resume != 0U)) {
		return;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

	/* NOTE: Else clean the top half preparations of the aborted event
	 * currently in preparation pipeline.
	 */
	err = lll_hfclock_off();
	LL_ASSERT_ERR(err >= 0);

	/* Accumulate the latency as event is aborted while being in pipeline */
	if (false) {

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
	} else if (lll->is_lll_resume != 0U) {
		/* latency prepare already incremented as this is a resume being aborted. */

#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */
	} else {
		lll->lazy_prepare = prepare_param->lazy;
		lll->latency_prepare += (lll->lazy_prepare + 1U);

		/* Accumulate window widening */
		lll->window_widening_prepare_us += lll->window_widening_periodic_us *
						   (prepare_param->lazy + 1);
		if (lll->window_widening_prepare_us > lll->window_widening_max_us) {
			lll->window_widening_prepare_us = lll->window_widening_max_us;
		}
	}

	/* Increment abort count */
	if (lll->abort_count < UINT8_MAX) {
		lll->abort_count++;
	}

	/* Extra done event, to check sync lost */
	e = ull_event_done_extra_get();
	LL_ASSERT_ERR(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC_ISO;
	e->estab_failed = 0U;
	e->trx_cnt = 0U;
	e->crc_valid = 0U;

	lll_done(prepare_param->param);
}

static void isr_rx_estab(void *param)
{
	struct event_done_extra *e;
	struct lll_sync_iso *lll;
	uint8_t trx_done;
	uint8_t crc_ok;

	/* Get reference to LLL context */
	lll = param;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX)
	/* Cancel the subevent pre-armed in prepare */
	radio_switch_complete_and_b2b_se_rx_disable();
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */

	/* Read radio status and events */
	trx_done = radio_is_done();
	if (trx_done) {
		crc_ok = radio_crc_is_valid();
		lll->trx_cnt++;
	} else {
		crc_ok = 0U;
	}

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* Check for MIC failures for encrypted Broadcast ISO streams */
	if (IS_ENABLED(CONFIG_BT_CTLR_BROADCAST_ISO_ENC) && crc_ok && lll->enc) {
		struct node_rx_pdu *node_rx;
		struct pdu_bis *pdu;

		/* By design, there shall always be one free node rx available when setting up radio
		 * for new PDU reception.
		 */
		node_rx = ull_iso_pdu_rx_alloc_peek(1U);
		LL_ASSERT_DBG(node_rx);

		/* Get reference to received PDU and validate MIC for non-empty PDU */
		pdu = (void *)node_rx->pdu;
		if (pdu->len) {
			bool mic_failure;
			uint32_t done;

			done = radio_ccm_is_done();
			LL_ASSERT_ERR(done);

			mic_failure = !radio_ccm_mic_is_valid();
			if (mic_failure) {
				lll->term_reason = BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;
			}
		}
	}

	/* Calculate and place the drift information in done event */
	e = ull_event_done_extra_get();
	LL_ASSERT_ERR(e);

	e->type = EVENT_DONE_EXTRA_TYPE_SYNC_ISO_ESTAB;
	e->estab_failed = lll->term_reason ? 1U : 0U;
	e->trx_cnt = lll->trx_cnt;
	e->crc_valid = crc_ok;

	if (lll->trx_cnt) {
		e->drift.preamble_to_addr_us = addr_us_get(lll->phy);
		e->drift.start_to_address_actual_us =
			radio_tmr_aa_get() - radio_tmr_ready_get();
		e->drift.window_widening_event_us =
			lll->window_widening_event_us;

		/* Reset window widening, as anchor point sync-ed */
		lll->window_widening_event_us = 0U;
		lll->window_size_event_us = 0U;

		/* Reset LLL abort count as LLL event is gracefully done and
		 * was not aborted by any other event when current event could
		 * have been using unreserved time space.
		 */
		lll->abort_count = 0U;
	}

	lll_isr_cleanup(param);
}

static void isr_rx(void *param)
{
	struct lll_sync_iso_stream *stream;
	struct node_rx_pdu *node_rx_prof;
	struct lll_sync_iso *lll;
	uint8_t access_addr[4];
	uint16_t data_chan_id;
	uint8_t data_chan_use;
	uint32_t se_offset_us;
	uint8_t phy_flags_rx;
	uint8_t crc_init[3];
	uint8_t stream_curr;
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
		node_rx_prof = lll_prof_reserve();
	}

	/* Read radio status and events */
	trx_done = radio_is_done();

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR) && !trx_done) {
		/* ISR latency is measured from the on-air packet END
		 * timestamp, no END captured when the ISR is due to Rx timeout
		 * or explicit radio disable, hence do not measure latency.
		 */
		lll_prof_latency_invalidate();
	}

	/* initialize LLL context reference */
	lll = param;

	const bool is_sequential_packing = (lll->bis_spacing >= (lll->sub_interval * lll->nse));

	if (!trx_done) {
		/* Clear radio rx status and events */
		lll_isr_rx_status_reset();

		/* BIS index */
		bis_idx = lll->bis_curr - 1U;

		/* Current stream */
		stream_curr = lll->stream_curr;

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			crc_ok = 0U;
		}

		goto isr_rx_done;
	}

	crc_ok = radio_crc_is_valid();
	phy_flags_rx = radio_phy_flags_rx_get();
	lll->trx_cnt++;

	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL) &&
	    is_sequential_packing) {
		const struct lll_sync_iso_stream *sync_stream;
		uint8_t se;

		sync_stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
		se = ((lll->bis_curr - sync_stream->bis_index) *
		      ((lll->bn * lll->irc) + lll->ptc)) +
		     ((lll->irc_curr - 1U) * lll->bn) +
		     (lll->bn_curr - 1U) + lll->ptc_curr + lll->ctrl;
		se_offset_us = lll->sub_interval * se;
		lll->se = se;
	} else if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED) &&
		   !is_sequential_packing) {
		const struct lll_sync_iso_stream *sync_stream;
		uint8_t se;

		sync_stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
		se = (lll->bis_curr - sync_stream->bis_index) +
		     ((((lll->irc_curr - 1U) * lll->bn) +
		       (lll->bn_curr - 1U) + lll->ptc_curr) *
		      lll->num_bis) + lll->ctrl;
		se_offset_us = lll->bis_spacing * se;
		lll->se = se;
	} else {
		se_offset_us = 0U;

		LL_ASSERT_DBG(false);
	}

	/* Store the access address timestamp used for subsequent subevent scheduling */
	lll->aa_se = radio_tmr_aa_get();
	if (!radio_tmr_aa_restore()) {
		uint32_t ticks_offset;
		uint32_t ticks_start;
		uint32_t ready_us;

		/* CRC valid for at least one of the subevent in the BIG, used in supervision check.
		 */
		lll->crc_ok_anchor = crc_ok;

		/* Save the START, READY and AA captured for the first anchor point sync */
		ticks_start = radio_tmr_start_get();
		ticks_offset = HAL_TICKER_US_TO_TICKS(se_offset_us);
		radio_tmr_start_save(ticks_start - ticks_offset);
		ready_us = radio_tmr_ready_get();
		radio_tmr_ready_save(ready_us - se_offset_us);
		radio_tmr_aa_save(lll->aa_se - se_offset_us);
	}

	/* Compensate for the Rx chain delay */
	lll->aa_se -= radio_rx_address_delay_get(lll->phy, phy_flags_rx);

	/* Clear radio rx status and events */
	lll_isr_rx_status_reset();

	/* BIS index */
	bis_idx = lll->bis_curr - 1U;

	/* Current stream */
	stream_curr = lll->stream_curr;

	/* Check CRC and generate ISO Data PDU */
	if (crc_ok) {
		struct lll_sync_iso_stream *sync_stream;
		struct node_rx_pdu *node_rx;
		uint32_t payload_offset;
		uint16_t payload_index;
		uint16_t stream_handle;
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
		}

		node_rx = ull_iso_pdu_rx_alloc_peek(1U);
		LL_ASSERT_DBG(node_rx);

		pdu = (void *)node_rx->pdu;

		/* Check for new control PDU in control subevent */
		if (pdu->cstf && (pdu->cssn != lll->cssn_curr)) {
			lll->cssn_next = pdu->cssn;
			/* TODO: check same CSSN is used in every subevent */
		}

		/* Check if there are 2 free rx buffers, one will be
		 * consumed to receive the current PDU, and the other
		 * is to ensure a PDU can be setup for the radio DMA to
		 * receive in the next sub_interval/iso_interval.
		 */
		if (ull_iso_pdu_rx_alloc_peek(2U) == NULL) {
			goto isr_rx_done;
		}

		/* Check payload buffer overflow.
		 * Ensure we are not having offset values over 255 in payload_count_max, used to
		 * allocate the buffers.
		 */
		payload_offset = (lll->latency_event * lll->bn) + payload_index_get(lll);
		if (payload_offset >= lll->payload_count_max) {
			goto isr_rx_done;
		}

		/* Calculate the payload index in the sliding window */
		payload_index = lll->payload_tail + payload_offset;
		if (payload_index >= lll->payload_count_max) {
			payload_index -= lll->payload_count_max;
		}

		/* Get reference to stream context */
		stream_handle = lll->stream_handle[stream_curr];
		sync_stream = ull_sync_iso_lll_stream_get(stream_handle);

		/* Store the received PDU if selected stream and not already
		 * received (say in previous event as pre-transmitted PDU.
		 */
		if ((lll->bis_curr == sync_stream->bis_index) && pdu->len &&
		    !lll->payload[stream_curr][payload_index]) {
			uint16_t handle;

			if (IS_ENABLED(CONFIG_BT_CTLR_BROADCAST_ISO_ENC) &&
			    lll->enc) {
				bool mic_failure;
				uint32_t done;

				done = radio_ccm_is_done();
				LL_ASSERT_ERR(done);

				mic_failure = !radio_ccm_mic_is_valid();
				if (mic_failure) {
					lll->term_reason = BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL;

					goto isr_rx_mic_failure;
				}
			}

			/* Discard the received PDU, whether unencrypted or the
			 * decrypted, with length that exceeds the configured
			 * maximum receive data length used to setup the radio
			 * packet reception.
			 */
			if (pdu->len > lll->max_pdu) {
				goto isr_rx_done;
			}

			ull_iso_pdu_rx_alloc();

			handle = LL_BIS_SYNC_HANDLE_FROM_IDX(stream_handle);
			isr_rx_iso_data_valid(lll, handle, node_rx);

			lll->payload[stream_curr][payload_index] = node_rx;
		}
	}

isr_rx_done:
	uint8_t bis_idx_old = bis_idx;
	uint8_t bis_idx_new = bis_idx;

	new_burst = 0U;
	skipped = 0U;

isr_rx_find_subevent:
	/* Sequential or Interleaved BIS subevents decision */
#if defined(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED)
	if (!is_sequential_packing) {
		goto isr_rx_interleaved;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED */

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL)
	/* NOTE: below code is for Sequential Rx only */

	/* Find the next (bn_curr)th subevent to receive PDU */
	while (lll->bn_curr < lll->bn) {
		uint32_t payload_offset;
		uint16_t payload_index;

		/* Next burst number to check for reception required */
		lll->bn_curr++;

		/* Check payload buffer overflow.
		 * Ensure we are not having offset values over 255 in payload_count_max, used to
		 * allocate the buffers.
		 */
		payload_offset = (lll->latency_event * lll->bn) + (lll->bn_curr - 1U);
		if (payload_offset >= lll->payload_count_max) {
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
		if (!lll->payload[stream_curr][payload_index]) {
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
			uint32_t payload_offset;
			uint16_t payload_index;

			/* Increment to next repetition count and be at first
			 * burst count for it.
			 */
			lll->bn_curr = 1U;
			lll->irc_curr++;

			/* Check payload buffer overflow */
			/* FIXME: Typically we should not have high latency, but do have an
			 *        assertion check to ensure we do not rollover in the payload_index
			 *        variable use. Alternatively, add implementation to correctly
			 *        skip subevents as buffers at these high offset are unavailable.
			 */
			payload_offset = (lll->latency_event * lll->bn);
			LL_ASSERT_ERR(payload_offset <= UINT8_MAX);

			/* Find the index of the (irc_curr)th bn = 1 Rx PDU
			 * buffer.
			 */
			payload_index = lll->payload_tail + payload_offset;
			if (payload_index >= lll->payload_count_max) {
				payload_index -= lll->payload_count_max;
			}

			/* Check if (irc_curr)th bn = 1 Rx PDU has been
			 * received.
			 */
			if (!lll->payload[stream_curr][payload_index]) {
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
		/* Next selected stream */
		if ((lll->stream_curr + 1U) < lll->stream_count) {
			struct lll_sync_iso_stream *sync_stream;
			uint16_t stream_handle;

			stream_curr = ++lll->stream_curr;
			stream_handle = lll->stream_handle[stream_curr];
			sync_stream = ull_sync_iso_lll_stream_get(stream_handle);
			if (sync_stream->bis_index <= lll->num_bis) {
				uint32_t payload_offset;
				uint16_t payload_index;

				lll->bis_curr = sync_stream->bis_index;
				lll->bn_curr = 1U;
				lll->irc_curr = 1U;
				lll->ptc_curr = 0U;

				/* flag that subevent where skipped */
				if (skipped != 0U) {
					skipped = 1U;
				}

				/* new BIS index */
				bis_idx_new = lll->bis_curr - 1U;

				/* Check payload buffer overflow */
				/* FIXME: Typically we should not have high latency, but do have an
				 *        assertion check to ensure we do not rollover in the
				 *        payload_index variable use. Alternatively, add
				 *        implementation to correctly skip subevents as buffers at
				 *        these high offsets are unavailable.
				 */
				payload_offset = (lll->latency_event * lll->bn);
				LL_ASSERT_ERR(payload_offset <= UINT8_MAX);

				/* Find the index of the (irc_curr)th bn = 1 Rx
				 * PDU buffer.
				 */
				payload_index = lll->payload_tail + payload_offset;
				if (payload_index >= lll->payload_count_max) {
					payload_index -= lll->payload_count_max;
				}

				/* Check if (irc_curr)th bn = 1 Rx PDU has been
				 * received.
				 */
				if (!lll->payload[stream_curr][payload_index]) {
					/* Receive the (irc_curr)th bn = 1 Rx
					 * PDU of bis_curr.
					 */
					bis = lll->bis_curr;

					goto isr_rx_next_subevent;
				} else {
					skipped++;

					/* flag to skip successive repetitions */
					if (lll->bn_curr >= lll->bn) {
						skipped++;

						new_burst = 1U;
					}

					/* BIS index */
					bis_idx = bis_idx_new;

					/* Find the missing (bn_curr)th Rx PDU
					 * of bis_curr
					 */
					goto isr_rx_find_subevent;
				}
			} else {
				lll->bis_curr = lll->num_bis;
				lll->ctrl_chan_use = CTRL_CHAN_DEFER;
			}
		} else {
			lll->bis_curr = lll->num_bis;
			lll->ctrl_chan_use = CTRL_CHAN_DEFER;
		}
	}

	if (skipped) {
		lll->ctrl_chan_use = CTRL_CHAN_DEFER;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL */

	goto isr_rx_ctrl;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED)
isr_rx_interleaved:
	/* Next BIS */
	if (lll->bis_curr < lll->num_bis) {
		/* Next selected stream */
		if ((lll->stream_curr + 1U) < lll->stream_count) {
			struct lll_sync_iso_stream *sync_stream;
			uint16_t stream_handle;

			stream_curr = ++lll->stream_curr;
			stream_handle = lll->stream_handle[stream_curr];
			sync_stream = ull_sync_iso_lll_stream_get(stream_handle);
			if (sync_stream->bis_index <= lll->num_bis) {
				uint8_t payload_offset;
				uint8_t payload_index;

				lll->bis_curr = sync_stream->bis_index;

				/* Check payload buffer overflow */
				payload_offset = (lll->bn_curr - 1U) +
						 (lll->ptc_curr * lll->pto);
				if (payload_offset > lll->payload_count_max) {
					const uint16_t event_counter =
						(lll->payload_count / lll->bn) - 1U;

					next_chan_calc_int(lll, event_counter);

					goto isr_rx_interleaved;
				}

				/* Find the index of the (bn_curr)th Rx PDU buffer */
				payload_index = lll->payload_tail + payload_offset;
				if (payload_index >= lll->payload_count_max) {
					payload_index -= lll->payload_count_max;
				}

				/* Check if (bn_curr)th Rx PDU has been received */
				if (lll->payload[stream_curr][payload_index]) {
					const uint16_t event_counter =
						(lll->payload_count / lll->bn) - 1U;

					next_chan_calc_int(lll, event_counter);

					goto isr_rx_interleaved;
				}

				bis = lll->bis_curr;

				goto isr_rx_next_subevent;
			} else {
				lll->bis_curr = lll->num_bis;
				lll->ctrl_chan_use = CTRL_CHAN_DEFER;
			}
		} else {
			lll->bis_curr = lll->num_bis;
			lll->ctrl_chan_use = CTRL_CHAN_DEFER;
		}
	}

	if (lll->bis_curr >= lll->num_bis) {
		struct lll_sync_iso_stream *sync_stream;
		uint16_t stream_handle;

		lll->stream_curr = 0U;
		stream_curr = 0U;
		stream_handle = lll->stream_handle[stream_curr];
		sync_stream = ull_sync_iso_lll_stream_get(stream_handle);
		if (sync_stream->bis_index <= lll->num_bis) {
			lll->bis_curr = sync_stream->bis_index;
			bis_idx = lll->bis_curr - 1U;
		} else {
			LL_ASSERT_DBG(false);
		}
	}

	if (lll->bn_curr < lll->bn) {
		uint8_t payload_offset;
		uint8_t payload_index;

		lll->bn_curr++;

		/* Check payload buffer overflow */
		payload_offset = (lll->bn_curr - 1U);
		if (payload_offset > lll->payload_count_max) {
			const uint16_t event_counter =
				(lll->payload_count / lll->bn) - 1U;

			next_chan_calc_int(lll, event_counter);

			goto isr_rx_interleaved;
		}

		/* Find the index of the (bn_curr)th Rx PDU buffer */
		payload_index = lll->payload_tail + payload_offset;
		if (payload_index >= lll->payload_count_max) {
			payload_index -= lll->payload_count_max;
		}

		/* Check if (bn_curr)th Rx PDU has been received */
		if (lll->payload[stream_curr][payload_index]) {
			const uint16_t event_counter =
				(lll->payload_count / lll->bn) - 1U;

			next_chan_calc_int(lll, event_counter);

			goto isr_rx_interleaved;
		}

		bis = lll->bis_curr;

		goto isr_rx_next_subevent;
	}

	if (lll->irc_curr < lll->irc) {
		uint8_t payload_index;

		lll->irc_curr++;
		lll->bn_curr = 1U;

		/* Find the index of the (irc_curr)th bn = 1 Rx PDU
		 * buffer.
		 */
		payload_index = lll->payload_tail;

		/* Check if (irc_curr)th bn = 1 Rx PDU has been
		 * received.
		 */
		if (lll->payload[stream_curr][payload_index]) {
			const uint16_t event_counter =
				(lll->payload_count / lll->bn) - 1U;

			next_chan_calc_int(lll, event_counter);

			goto isr_rx_interleaved;
		}

		bis = lll->bis_curr;

		goto isr_rx_next_subevent;
	}

	if (lll->ptc_curr < lll->ptc) {
		lll->ptc_curr++;

		bis = lll->bis_curr;

		goto isr_rx_next_subevent;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED */

isr_rx_ctrl:
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

isr_rx_mic_failure:
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_reserve_send(node_rx_prof);
	}

	isr_rx_lll_done(param);

	return;

isr_rx_next_subevent:
	/* Calculate the Access Address for the BIS event */
	util_bis_aa_le32(bis, lll->seed_access_addr, access_addr);
	data_chan_id = lll_chan_id(access_addr);

	/* Pre-compute the event counter used in channel selection and next
	 * subevent channel calculation below. Hoisted here so the 64-bit
	 * division is performed once per subevent setup instead of being
	 * repeated in each conditional branch that needs it.
	 */
	const uint16_t event_counter = (lll->payload_count / lll->bn) - 1U;

	/* Calculate the CRC init value for the BIS event,
	 * preset with the BaseCRCInit value from the BIGInfo data the most
	 * significant 2 octets and the BIS_Number for the specific BIS in the
	 * least significant octet.
	 */
	crc_init[0] = bis;
	(void)memcpy(&crc_init[1], lll->base_crc_init, sizeof(uint16_t));

	radio_aa_set(access_addr);
	radio_crc_configure(PDU_CRC_POLYNOMIAL, sys_get_le24(crc_init));

#if !defined(CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX)
	radio_switch_complete_and_disable();
#endif /* !CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */

	/* Set the channel to use */
	if (!bis) {
		if (lll->ctrl_chan_use == CTRL_CHAN_DEFER) {
			ctrl_chan_calc(lll, event_counter);
		} else {
			LL_ASSERT_ERR(lll->ctrl_chan_use != CTRL_CHAN_NONE);
		}

		data_chan_use = lll->ctrl_chan_use;
	} else if (!skipped) {
		data_chan_use = lll->next_chan_use;
	} else {
		uint8_t skip = skipped;

		/* Initialise to avoid compile error */
		data_chan_use = 0U;

		if (bis_idx_old != bis_idx_new) {
			if (skip != 0U) {
				/* Calculate the radio channel to use for next BIS */
				data_chan_use = lll_chan_iso_event(event_counter,
								   data_chan_id,
								   lll->data_chan_map,
								   lll->data_chan_count,
								   &lll->data_chan.prn_s,
								   &lll->data_chan.remap_idx);
				/* reset flagged skipped value */
				skip--;
			} else {
				data_chan_use = lll->next_chan_use;
			}

		}

		/* Calculate the radio channel to use for subevent */
		while (skip != 0U) {
			skip--;

			data_chan_use = lll_chan_iso_subevent(data_chan_id,
						lll->data_chan_map,
						lll->data_chan_count,
						&lll->data_chan.prn_s,
						&lll->data_chan.remap_idx);
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
			struct node_rx_pdu *node_rx;

			payload_count += payload_index_get(lll);

			/* By design, there shall always be one free node rx
			 * available for setting up radio for new PDU reception.
			 */
			node_rx = ull_iso_pdu_rx_alloc_peek(1U);
			LL_ASSERT_DBG(node_rx);

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
			struct node_rx_pdu *node_rx;

			/* By design, there shall always be one free node rx
			 * available for setting up radio for new PDU reception.
			 */
			node_rx = ull_iso_pdu_rx_alloc_peek(1U);
			LL_ASSERT_DBG(node_rx);

			pdu = (void *)node_rx->pdu;
		} else {
			/* Use the dedicated Control PDU buffer */
			pdu = radio_pkt_big_ctrl_get();
		}

		radio_pkt_rx_set(pdu);
	}

	/* assert if radio packet ptr is not set and radio started tx */
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		if (radio_is_ready()) {
			uint32_t end_se_us;

			end_se_us = radio_tmr_end_get();

			LL_ASSERT_MSG(false, "%s: trx_cnt %u crc_ok %u lll->se %u skipped %u"
				      " aa_se_us %u end_se_us %u Radio ISR latency: %u (%u)",
				      __func__, lll->trx_cnt, crc_ok, lll->se, skipped,
				      lll->aa_se, end_se_us, lll_prof_latency_get(),
				      lll_prof_cputime_get());
		}
	} else {
		LL_ASSERT_ERR(!radio_is_ready());
	}

	/* PDU Header Complete TimeOut, calculate the absolute timeout in
	 * microseconds by when a PDU header is to be received for each
	 * subevent.
	 */
	stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL) &&
	    is_sequential_packing) {
		nse = ((lll->bis_curr - stream->bis_index) *
		       ((lll->bn * lll->irc) + lll->ptc)) +
		      ((lll->irc_curr - 1U) * lll->bn) + (lll->bn_curr - 1U) +
		      lll->ptc_curr + lll->ctrl;
		hcto = lll->sub_interval * (nse - lll->se);
	} else if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED) &&
		   !is_sequential_packing) {
		nse = (lll->bis_curr - stream->bis_index) +
		       ((((lll->irc_curr - 1U) * lll->bn) +
			 (lll->bn_curr - 1U) + lll->ptc_curr) *
			lll->num_bis) + lll->ctrl;
		hcto = lll->bis_spacing * (nse - lll->se);
	} else {
		nse = 0U;
		hcto = 0U;

		LL_ASSERT_DBG(false);
	}

	const uint32_t rx_ready_delay = radio_rx_ready_delay_get(lll->phy, PHY_FLAGS_S8);
	const uint32_t addr_us = addr_us_get(lll->phy);

	if (lll->trx_cnt) {
		/* Setup radio packet timer header complete timeout for
		 * subsequent subevent PDU.
		 */
		hcto += lll->aa_se;
		hcto -= addr_us;
		hcto -= rx_ready_delay;

		/* Calculate the radio start with consideration of the drift
		 * based on the access address capture timestamp.
		 * Listen early considering +/- 2 us active clock jitter, i.e.
		 * listen early by 4 us.
		 */
		uint32_t jitter_max_us;
		uint32_t overhead_us;
		uint32_t jitter_us;

		/* Overhead within EVENT_IFS_US to exclude from max. jitter */
		/* Required radio ready duration, settling time */
		overhead_us = rx_ready_delay;
		/* If single timer used, then consider required max. latency */
		overhead_us += HAL_RADIO_ISR_LATENCY_MAX_US;
		/* Add chain delay overhead */
		overhead_us += radio_rx_chain_delay_get(lll->phy, PHY_FLAGS_S8);
		/* Add base clock jitter overhead */
		overhead_us += (EVENT_CLOCK_JITTER_US << 1);
		LL_ASSERT_DBG(EVENT_IFS_US > overhead_us);

		/* Max. available clock jitter */
		jitter_max_us = (EVENT_IFS_US - overhead_us) >> 1;
		jitter_max_us += lll->sub_interval * (nse - lll->se - 1U);

		/* Jitter for current subevent */
		jitter_us = (EVENT_CLOCK_JITTER_US << 1) * (nse - lll->se);
		if (jitter_us > jitter_max_us) {
			jitter_us = jitter_max_us;
		}

		LL_ASSERT_DBG(hcto > jitter_us);

		hcto -= jitter_us;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
		/* Yield the radio while waiting for the next subevent if an event pending in
		 * the prepare pipeline can receive its subevent before it.
		 */
		if (is_sequential_packing && (lll->ctrl == 0U)) {
			struct lll_event *e;

			lll->se_next = nse;
			lll->ticks_se_next = (radio_tmr_start_get() +
					      HAL_TICKER_US_TO_TICKS(hcto - 1U)) &
					     HAL_TICKER_CNTR_MASK;

			e = is_yield_to_pending(lll);
			if (e != NULL) {
				isr_rx_yield(lll, e, 0U);

				goto isr_rx_yield_done;
			}

			/* Yield the radio to a resume or a deferred scan prepare if the
			 * current event can resume before its next subevent.
			 */
			if (is_yield_to_other(lll)) {
				isr_rx_yield(lll, NULL, 1U);

				goto isr_rx_yield_done;
			}
		}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */

		/* +1 us radio_tmr_start_us compensation */
		start_us = hcto - 1U;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX)
		hcto = se_b2b_rx_start_us(lll, nse, start_us, is_sequential_packing);
#else /* !CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */
		hcto = radio_tmr_start_us(0U, start_us);
#endif /* !CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */

		if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
			lll_prof_cputime_capture();

			if (hcto != (start_us + 1U)) {
				uint32_t end_se_us;

				end_se_us = radio_tmr_end_get();

				LL_ASSERT_MSG(false, "%s: trx_cnt %u crc_ok %u lll->se %u nse %u "
					      "skipped %u jitter_us %u aa_se_us %u end_se_us %u "
					      "start_us %u hcto %u Radio ISR latency: %u (%u)",
					      __func__, lll->trx_cnt, crc_ok, lll->se, nse, skipped,
					      jitter_us, lll->aa_se, end_se_us, start_us, hcto,
					      lll_prof_latency_get(), lll_prof_cputime_get());
			}
		} else {
			LL_ASSERT_ERR(hcto == (start_us + 1U));
		}

		if (hcto != (start_us + 1U)) {
			/* Radio started late, listen from the actual start so
			 * that the header complete timeout lets the next
			 * subevent be received if this one is missed.
			 */
			start_us = hcto - 1U;
		}

		/* Add 8 us * subevents so far, as radio was setup to listen
		 * 4 us early and subevents could have a 4 us drift each until
		 * the current subevent we are listening.
		 */
		hcto += (jitter_us << 1);
	} else {
		/* First subevent PDU was not received, hence setup radio packet
		 * timer header complete timeout from where the first subevent
		 * PDU which is the BIG event anchor point would have been
		 * received.
		 */
		hcto += radio_tmr_ready_restore();

		/* +1 us radio_tmr_start_us compensation */
		start_us = hcto - 1U;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX)
		hcto = se_b2b_rx_start_us(lll, nse, start_us, is_sequential_packing);
#else /* !CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */
		hcto = radio_tmr_start_us(0U, start_us);
#endif /* !CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */

		if (hcto != (start_us + 1U)) {
			/* Radio started late, listen from the actual start so
			 * that the header complete timeout lets the next
			 * subevent be received if this one is missed.
			 */
			start_us = hcto - 1U;
		}

		hcto += ((EVENT_JITTER_US + EVENT_TICKER_RES_MARGIN_US +
			  lll->window_widening_event_us) << 1) +
			lll->window_size_event_us;
	}

	/* header complete timeout to consider the radio ready delay, chain
	 * delay and access address duration.
	 */
	hcto += rx_ready_delay;
	hcto += addr_us;
	hcto += radio_rx_address_delay_get(lll->phy, PHY_FLAGS_S8);

	/* setup absolute PDU header reception timeout */
	radio_tmr_hcto_configure_abs(hcto);

	/* Setup Access Address capture for subsequent subevents */
	radio_tmr_aa_capture();

	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		/* setup capture of PDU end timestamp */
		radio_tmr_end_capture();
	}

#if defined(HAL_RADIO_GPIO_HAVE_LNA_PIN)
	radio_gpio_lna_setup();

	radio_gpio_pa_lna_enable(start_us + rx_ready_delay - HAL_RADIO_GPIO_LNA_OFFSET);
#endif /* HAL_RADIO_GPIO_HAVE_LNA_PIN */

	/* Calculate ahead the next subevent channel index */
	if (false) {

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL)
	} else if (is_sequential_packing) {
		next_chan_calc_seq(lll, event_counter, data_chan_id);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL */

#if defined(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED)
	} else if (!is_sequential_packing) {
		next_chan_calc_int(lll, event_counter);
#endif /* CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED */

	} else {
		LL_ASSERT_DBG(false);
	}

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER)
isr_rx_yield_done:
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SLOT_WINDOW_JITTER */
	if (IS_ENABLED(CONFIG_BT_CTLR_PROFILE_ISR)) {
		lll_prof_reserve_send(node_rx_prof);
	}
}

static void isr_rx_lll_done(void *param)
{
	struct event_done_extra *e;
	struct lll_sync_iso *lll;
	uint16_t latency_event;
	uint16_t payload_index;
	uint8_t has_iso_rx;
	uint8_t bis_idx;

	lll = param;

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX)
	/* Cancel the subevent pre-armed by the last subevent received */
	radio_switch_complete_and_b2b_se_rx_disable();
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */

	/* Gracefully flush pipelined event on terminate received */
	if (unlikely(lll->is_lll_stop != 0U)) {
		goto isr_done_cleanup;
	}

	/* Catchup with ISO event latencies */
	latency_event = lll->latency_event;

	/* Dequeue sliding window */
	has_iso_rx = 0U;
	payload_index = lll->payload_tail;
	do {
		uint8_t stream_curr;

		stream_curr = 0U;
		for (bis_idx = 0U; bis_idx < lll->num_bis; bis_idx++) {
			struct lll_sync_iso_stream *stream;
			uint8_t stream_curr_inc;
			uint16_t stream_handle;
			uint8_t payload_tail;

			stream_handle = lll->stream_handle[stream_curr];
			stream = ull_sync_iso_lll_stream_get(stream_handle);
			/* Skip BIS indices not synchronized. bis_index is 0x01 to 0x1F,
			 * where as bis_idx is 0 indexed.
			 */
			if ((bis_idx + 1U) != stream->bis_index) {
				continue;
			}

			payload_tail = lll->payload_tail;
			for (uint8_t bn = 0U; bn < lll->bn; bn++) {
				if (lll->payload[stream_curr][payload_tail]) {
					struct node_rx_pdu *node_rx;

					node_rx = lll->payload[stream_curr][payload_tail];
					lll->payload[stream_curr][payload_tail] = NULL;

					iso_rx_put(node_rx->hdr.link, node_rx);

					has_iso_rx = 1U;
				} else {
					struct node_rx_pdu *node_rx;

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
						isr_rx_iso_data_invalid(lll, latency_event, bn,
									handle, node_rx);

						iso_rx_put(node_rx->hdr.link, node_rx);

						has_iso_rx = 1U;
					}
				}

				payload_index = payload_tail + 1U;
				if (payload_index >= lll->payload_count_max) {
					payload_index = 0U;
				}
				payload_tail = payload_index;
			}

			stream_curr_inc = stream_curr + 1U;
			if (stream_curr_inc < lll->stream_count) {
				stream_curr = stream_curr_inc;
			}
		}
		lll->payload_tail = payload_index;
	} while (latency_event--);

	if (has_iso_rx != 0U) {
		iso_rx_sched();
	}

	e = ull_event_done_extra_get();
	LL_ASSERT_ERR(e);

	/* Check if BIG terminate procedure received */
	if (lll->term_reason) {
		LL_ASSERT_DBG(lll->is_lll_stop == 0U);
		lll->is_lll_stop = 1U;

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
	e->estab_failed = 0U;
	e->trx_cnt = lll->trx_cnt;
	e->crc_valid = lll->crc_ok_anchor;

	if (lll->trx_cnt) {
		e->drift.preamble_to_addr_us = addr_us_get(lll->phy);
		e->drift.start_to_address_actual_us =
			radio_tmr_aa_restore() - radio_tmr_ready_restore();
		e->drift.window_widening_event_us =
			lll->window_widening_event_us;

		/* Reset window widening, as anchor point sync-ed */
		lll->window_widening_event_us = 0U;
		lll->window_size_event_us = 0U;

		/* Reset LLL abort count as LLL event is gracefully done and
		 * was not aborted by any other event when current event could
		 * have been using unreserved time space.
		 */
		lll->abort_count = 0U;
	}

isr_done_cleanup:
	lll_isr_cleanup(param);
}

static void isr_done(void *param)
{
	lll_isr_status_reset();

	isr_rx_lll_done(param);
}

static uint16_t payload_index_get(const struct lll_sync_iso *lll)
{
	uint16_t payload_index;

	if (lll->ptc_curr) {
		/* FIXME: Do not remember why ptc is 4 bits, it should be 5 bits as ptc is a
		 *        running buffer offset related to nse.
		 *        Fix ptc and ptc_curr definitions, until then there is an assertion
		 *        check when ptc is calculated in ptc_calc function.
		 */
		uint8_t ptx_idx = lll->ptc_curr - 1U; /* max. nse 5 bits */
		uint8_t ptx_payload_idx;
		uint16_t ptx_group_mult;
		uint8_t ptx_group_idx;

		/* Calculate group index and multiplier for deriving
		 * pre-transmission payload index.
		 */
		ptx_group_idx = ptx_idx / lll->bn; /* 5 bits */
		ptx_payload_idx = ptx_idx - ptx_group_idx * lll->bn; /* 8 bits */
		ptx_group_mult = (ptx_group_idx + 1U) * lll->pto; /* 9 bits */
		payload_index = ptx_payload_idx + ptx_group_mult * lll->bn; /* 13 bits */
	} else {
		payload_index  = lll->bn_curr - 1U; /* 3 bits */
	}

	return payload_index;
}

static void ctrl_chan_calc(struct lll_sync_iso *lll, uint16_t event_counter)
{
	uint8_t access_addr[4];
	uint16_t data_chan_id;
	uint16_t remap_idx;
	uint16_t prn_s;

	/* BIS_Number 0 is the BIG Control logical link */
	util_bis_aa_le32(0U, lll->seed_access_addr, access_addr);
	data_chan_id = lll_chan_id(access_addr);

	/* Calculate the radio channel to use for ISO event */
	lll->ctrl_chan_use = lll_chan_iso_event(event_counter, data_chan_id, lll->data_chan_map,
						lll->data_chan_count, &prn_s, &remap_idx);
}

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL)
static void next_chan_calc_seq(struct lll_sync_iso *lll, uint16_t event_counter,
			       uint16_t data_chan_id)
{
	/* Calculate ahead the next subevent channel index */
	if ((lll->bn_curr < lll->bn) ||
	    (lll->irc_curr < lll->irc) ||
	    (lll->ptc_curr < lll->ptc)) {
		/* Calculate the radio channel to use for next subevent */
		lll->next_chan_use =
			lll_chan_iso_subevent(data_chan_id,
					      lll->data_chan_map,
					      lll->data_chan_count,
					      &lll->data_chan.prn_s,
					      &lll->data_chan.remap_idx);
	} else if (lll->bis_curr < lll->num_bis) {
		uint8_t access_addr[4];

		/* Calculate the Access Address for the next BIS subevent */
		util_bis_aa_le32((lll->bis_curr + 1U), lll->seed_access_addr,
				 access_addr);
		data_chan_id = lll_chan_id(access_addr);

		/* Calculate the radio channel to use for next BIS */
		lll->next_chan_use =
			lll_chan_iso_event(event_counter,
					   data_chan_id,
					   lll->data_chan_map,
					   lll->data_chan_count,
					   &lll->data_chan.prn_s,
					   &lll->data_chan.remap_idx);
	} else {
		ctrl_chan_calc(lll, event_counter);
	}
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL */

#if defined(CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED)
static void next_chan_calc_int(struct lll_sync_iso *lll, uint16_t event_counter)
{
	struct lll_sync_iso_data_chan_interleaved *interleaved_data_chan;
	uint8_t stream_curr;
	uint8_t bis_prev;

	if ((lll->bis_curr >= lll->num_bis) &&
	    (lll->bn_curr >= lll->bn) &&
	    (lll->irc_curr >= lll->irc) &&
	    (lll->ptc_curr >= lll->ptc)) {
		ctrl_chan_calc(lll, event_counter);

		return;
	}

	/* Next selected stream */
	stream_curr = lll->stream_curr + 1U;
	if ((stream_curr < BT_CTLR_SYNC_ISO_STREAM_MAX) &&
	    (stream_curr < lll->stream_count)) {
		struct lll_sync_iso_stream *sync_stream;
		uint16_t stream_handle;

		stream_handle = lll->stream_handle[stream_curr];
		sync_stream = ull_sync_iso_lll_stream_get(stream_handle);
		if (sync_stream->bis_index <= lll->num_bis) {
			bis_prev = sync_stream->bis_index - 1U;
		} else {
			bis_prev = lll->num_bis;
		}
	} else {
		bis_prev = lll->num_bis;
	}

	if ((bis_prev < lll->num_bis) &&
	    (lll->bn_curr == 1U) &&
	    (lll->irc_curr == 1U) &&
	    (lll->ptc_curr == 0U)) {
		uint8_t access_addr[4];

		/* Calculate the Access Address for the next BIS subevent */
		util_bis_aa_le32((bis_prev + 1U), lll->seed_access_addr,
				 access_addr);

		interleaved_data_chan =
			&lll->interleaved_data_chan[bis_prev];
		interleaved_data_chan->id = lll_chan_id(access_addr);

		/* Calculate the radio channel to use for next BIS */
		lll->next_chan_use =
			lll_chan_iso_event(event_counter,
					   interleaved_data_chan->id,
					   lll->data_chan_map,
					   lll->data_chan_count,
					   &interleaved_data_chan->prn_s,
					   &interleaved_data_chan->remap_idx);
	} else {
		uint8_t bis_idx;

		if (bis_prev >= lll->num_bis) {
			struct lll_sync_iso_stream *sync_stream;

			sync_stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
			bis_idx = sync_stream->bis_index - 1U;
		} else {
			bis_idx = bis_prev;
		}

		interleaved_data_chan = &lll->interleaved_data_chan[bis_idx];

		/* Calculate the radio channel to use for next subevent */
		lll->next_chan_use =
			lll_chan_iso_subevent(interleaved_data_chan->id,
					      lll->data_chan_map,
					      lll->data_chan_count,
					      &interleaved_data_chan->prn_s,
					      &interleaved_data_chan->remap_idx);
	}
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_INTERLEAVED */

static void isr_rx_iso_data_valid(const struct lll_sync_iso *const lll,
				  uint16_t handle, struct node_rx_pdu *node_rx)
{
	struct lll_sync_iso_stream *stream;
	struct node_rx_iso_meta *iso_meta;

	node_rx->hdr.type = NODE_RX_TYPE_ISO_PDU;
	node_rx->hdr.handle = handle;

	iso_meta = &node_rx->rx_iso_meta;
	iso_meta->payload_number = lll->payload_count + payload_index_get(lll);
	/* Decrement BN as payload_count was pre-incremented */
	iso_meta->payload_number -= lll->bn;

	stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
	iso_meta->timestamp = HAL_TICKER_TICKS_TO_US(radio_tmr_start_restore()) +
			      radio_tmr_aa_restore() +
			      (DIV_ROUND_UP(lll->ptc_curr, lll->bn) *
			       lll->pto * lll->iso_interval *
			       PERIODIC_INT_UNIT_US) -
			      addr_us_get(lll->phy);

	const bool is_sequential_packing = (lll->bis_spacing >= (lll->sub_interval * lll->nse));

	if (is_sequential_packing) {
		iso_meta->timestamp -= (stream->bis_index - 1U) *
				       lll->sub_interval * lll->nse;
	} else {
		iso_meta->timestamp -= (stream->bis_index - 1U) *
				       lll->bis_spacing;
	}

	iso_meta->timestamp %=
		HAL_TICKER_TICKS_TO_US_64BIT(BIT64(HAL_TICKER_CNTR_MSBIT + 1U));

	iso_meta->status = 0U;
}

static void isr_rx_iso_data_invalid(const struct lll_sync_iso *const lll,
				    uint16_t latency, uint8_t bn,
				    uint16_t handle,
				    struct node_rx_pdu *node_rx)
{
	struct lll_sync_iso_stream *stream;
	struct node_rx_iso_meta *iso_meta;

	node_rx->hdr.type = NODE_RX_TYPE_ISO_PDU;
	node_rx->hdr.handle = handle;

	iso_meta = &node_rx->rx_iso_meta;
	iso_meta->payload_number = lll->payload_count + bn;
	/* Decrement BN as payload_count was pre-incremented */
	iso_meta->payload_number -= (latency + 1U) * lll->bn;

	stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
	iso_meta->timestamp = HAL_TICKER_TICKS_TO_US(radio_tmr_start_restore()) +
			      radio_tmr_aa_restore() - addr_us_get(lll->phy);

	const bool is_sequential_packing = (lll->bis_spacing >= (lll->sub_interval * lll->nse));

	if (is_sequential_packing) {
		iso_meta->timestamp -= (stream->bis_index - 1U) *
				       lll->sub_interval * lll->nse;
	} else {
		iso_meta->timestamp -= (stream->bis_index - 1U) *
				       lll->bis_spacing;
	}

	iso_meta->timestamp -= latency * lll->iso_interval *
			       PERIODIC_INT_UNIT_US;

	iso_meta->timestamp %=
		HAL_TICKER_TICKS_TO_US_64BIT(BIT64(HAL_TICKER_CNTR_MSBIT + 1U));

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

#if defined(CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX)
/* Index of the current subevent, counted from the first subevent of the first
 * selected BIS.
 */
static uint8_t se_index_get(const struct lll_sync_iso *lll,
			    bool is_sequential_packing)
{
	const struct lll_sync_iso_stream *stream;

	stream = ull_sync_iso_lll_stream_get(lll->stream_handle[0]);
	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL) &&
	    is_sequential_packing) {
		return ((lll->bis_curr - stream->bis_index) *
			((lll->bn * lll->irc) + lll->ptc)) +
		       ((lll->irc_curr - 1U) * lll->bn) + (lll->bn_curr - 1U) +
		       lll->ptc_curr + lll->ctrl;
	}

	return (lll->bis_curr - stream->bis_index) +
	       ((((lll->irc_curr - 1U) * lll->bn) +
		 (lll->bn_curr - 1U) + lll->ptc_curr) *
		lll->num_bis) + lll->ctrl;
}

/* Pre-arm the radio enable for the subevent after the subevent being setup,
 * relative to its radio READY.
 */
static void se_b2b_rx_arm(struct lll_sync_iso *lll, uint8_t se,
			  bool is_sequential_packing, bool is_chained)
{
	uint32_t se_spacing_us;

	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO_SEQUENTIAL) &&
	    is_sequential_packing) {
		se_spacing_us = lll->sub_interval;
	} else {
		se_spacing_us = lll->bis_spacing;
	}

	/* Listen early considering +/- 2 us active clock jitter */
	radio_tmr_tifs_set(se_spacing_us -
			   radio_rx_ready_delay_get(lll->phy, PHY_FLAGS_S8) -
			   (EVENT_CLOCK_JITTER_US << 1));

	if (is_chained) {
		radio_switch_complete_and_b2b_se_rx(lll->phy, PHY_FLAGS_S8,
						    lll->phy, PHY_FLAGS_S8);
	} else {
		radio_switch_complete_and_b2b_se_rx_first(lll->phy, PHY_FLAGS_S8,
							  lll->phy, PHY_FLAGS_S8);
	}

	lll->se_b2b = se + 1U;
}

/* Start the radio for the subevent being setup, using the radio enable
 * pre-armed at the READY of the previous subevent when this subevent is the
 * immediate next subevent, otherwise using the event timer. Returns the
 * actual start in microseconds plus 1, like radio_tmr_start_us().
 */
static uint32_t se_b2b_rx_start_us(struct lll_sync_iso *lll, uint8_t se,
				   uint32_t start_us,
				   bool is_sequential_packing)
{
	uint32_t actual_us;

	if (se == lll->se_b2b) {
		se_b2b_rx_arm(lll, se, is_sequential_packing, true);
		if (!radio_switch_b2b_se_rx_is_missed()) {
			return start_us + 1U;
		}
	}

	/* Subevents skipped or pre-armed radio enable missed, cancel the
	 * pre-armed radio enable before starting using the event timer.
	 */
	radio_switch_complete_and_b2b_se_rx_disable();

	se_b2b_rx_arm(lll, se, is_sequential_packing, false);

	actual_us = radio_tmr_start_us(0U, start_us);
	if (actual_us != (start_us + 1U)) {
		/* Radio started late, the subevent pre-armed relative to its
		 * READY would also be late, use event timer for next subevent.
		 */
		lll->se_b2b = UINT8_MAX;
	}

	return actual_us;
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO_SE_B2B_RX */
