/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci_types.h>

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll_clock.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"

#include "isoal.h"

#include "ull_scan_types.h"
#include "ull_sync_types.h"
#include "ull_iso_types.h"

#include "ull_internal.h"
#include "ull_scan_internal.h"
#include "ull_sync_internal.h"
#include "ull_iso_internal.h"
#include "ull_sync_iso_internal.h"

#include "ll.h"

#include "bt_crypto.h"

#include "hal/debug.h"

static int init_reset(void);
static struct ll_sync_iso_set *sync_iso_get(uint8_t handle);
static uint8_t sync_iso_handle_get(struct ll_sync_iso_set *sync);
static struct stream *sync_iso_stream_acquire(void);
static uint16_t sync_iso_stream_handle_get(struct lll_sync_iso_stream *stream);
static void timeout_cleanup(struct ll_sync_iso_set *sync_iso);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);
static void ticker_start_op_cb(uint32_t status, void *param);
static void ticker_update_op_cb(uint32_t status, void *param);
static void ticker_stop_op_cb(uint32_t status, void *param);
static void sync_iso_disable(void *param);
static void disabled_cb(void *param);

static memq_link_t link_lll_prepare;
static struct mayfly mfy_lll_prepare = {0U, 0U, &link_lll_prepare, NULL, NULL};

static struct ll_sync_iso_set ll_sync_iso[CONFIG_BT_CTLR_SCAN_SYNC_ISO_SET];
static struct lll_sync_iso_stream
			stream_pool[CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT];
static struct ll_iso_rx_test_mode
			test_mode[CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT];
static void *stream_free;

uint8_t ll_big_sync_create(uint8_t big_handle, uint16_t sync_handle,
			   uint8_t encryption, uint8_t *bcode, uint8_t mse,
			   uint16_t sync_timeout, uint8_t num_bis,
			   uint8_t *bis)
{
	struct ll_sync_iso_set *sync_iso;
	memq_link_t *link_sync_estab;
	memq_link_t *link_sync_lost;
	struct node_rx_pdu *node_rx;
	struct ll_sync_set *sync;
	struct lll_sync_iso *lll;
	int8_t last_index;

	sync = ull_sync_is_enabled_get(sync_handle);
	if (!sync || sync->iso.sync_iso) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	sync_iso = sync_iso_get(big_handle);
	if (!sync_iso) {
		/* Host requested more than supported number of ISO Synchronized
		 * Receivers.
		 * Or probably HCI handles where not translated to zero-indexed
		 * controller handles?
		 */
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Check if this ISO already is associated with a Periodic Sync */
	if (sync_iso->sync) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* TODO: Check remaining parameters */

	/* Check BIS indices */
	last_index = -1;
	for (uint8_t i = 0U; i < num_bis; i++) {
		/* Stream index must be in valid range and in ascending order */
		if (!IN_RANGE(bis[i], 0x01, 0x1F) || (bis[i] <= last_index)) {
			return BT_HCI_ERR_INVALID_PARAM;

		} else if (bis[i] > sync->num_bis) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}
		last_index = bis[i];
	}

	/* Check if requested encryption matches */
	if (encryption != sync->enc) {
		return BT_HCI_ERR_ENC_MODE_NOT_ACCEPTABLE;
	}

	/* Check if free BISes available */
	if (mem_free_count_get(stream_free) < num_bis) {
		return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
	}

	link_sync_estab = ll_rx_link_alloc();
	if (!link_sync_estab) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	link_sync_lost = ll_rx_link_alloc();
	if (!link_sync_lost) {
		ll_rx_link_release(link_sync_estab);

		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	node_rx = ll_rx_alloc();
	if (!node_rx) {
		ll_rx_link_release(link_sync_lost);
		ll_rx_link_release(link_sync_estab);

		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Initialize the ISO sync ULL context */
	sync_iso->sync = sync;
	sync_iso->timeout = sync_timeout;
	sync_iso->timeout_reload = 0U;
	sync_iso->timeout_expire = 0U;

	/* Setup the periodic sync to establish ISO sync */
	node_rx->hdr.link = link_sync_estab;
	sync->iso.node_rx_estab = node_rx;
	sync_iso->node_rx_lost.rx.hdr.link = link_sync_lost;

	/* Initialize sync LLL context */
	lll = &sync_iso->lll;
	lll->latency_prepare = 0U;
	lll->latency_event = 0U;
	lll->window_widening_prepare_us = 0U;
	lll->window_widening_event_us = 0U;
	lll->ctrl = 0U;
	lll->cssn_curr = 0U;
	lll->cssn_next = 0U;
	lll->term_reason = 0U;

	if (encryption) {
		const uint8_t BIG1[16] = {0x31, 0x47, 0x49, 0x42, };
		const uint8_t BIG2[4]  = {0x32, 0x47, 0x49, 0x42};
		uint8_t igltk[16];
		int err;

		/* Calculate GLTK */
		err = bt_crypto_h7(BIG1, bcode, igltk);
		LL_ASSERT(!err);
		err = bt_crypto_h6(igltk, BIG2, sync_iso->gltk);
		LL_ASSERT(!err);

		lll->enc = 1U;
	} else {
		lll->enc = 0U;
	}

	/* TODO: Implement usage of MSE to limit listening to subevents */

	/* Allocate streams */
	lll->stream_count = num_bis;
	for (uint8_t i = 0U; i < num_bis; i++) {
		struct lll_sync_iso_stream *stream;

		stream = (void *)sync_iso_stream_acquire();
		stream->big_handle = big_handle;
		stream->bis_index = bis[i];
		stream->dp = NULL;
		stream->test_mode = &test_mode[i];
		memset(stream->test_mode, 0, sizeof(struct ll_iso_rx_test_mode));
		lll->stream_handle[i] = sync_iso_stream_handle_get(stream);
	}

	/* Initialize ULL and LLL headers */
	ull_hdr_init(&sync_iso->ull);
	lll_hdr_init(lll, sync_iso);

	/* Enable periodic advertising to establish ISO sync */
	sync->iso.sync_iso = sync_iso;

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_big_sync_terminate(uint8_t big_handle, void **rx)
{
	struct ll_sync_iso_set *sync_iso;
	memq_link_t *link_sync_estab;
	struct node_rx_pdu *node_rx;
	memq_link_t *link_sync_lost;
	struct ll_sync_set *sync;
	int err;

	sync_iso = sync_iso_get(big_handle);
	if (!sync_iso) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	sync = sync_iso->sync;
	if (sync && sync->iso.sync_iso) {
		struct node_rx_sync_iso *se;

		if (sync->iso.sync_iso != sync_iso) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
		sync->iso.sync_iso = NULL;

		node_rx = sync->iso.node_rx_estab;
		link_sync_estab = node_rx->hdr.link;
		link_sync_lost = sync_iso->node_rx_lost.rx.hdr.link;

		ll_rx_link_release(link_sync_lost);
		ll_rx_link_release(link_sync_estab);
		ll_rx_release(node_rx);

		node_rx = (void *)&sync_iso->node_rx_lost;
		node_rx->hdr.type = NODE_RX_TYPE_SYNC_ISO;
		node_rx->hdr.handle = big_handle;

		/* NOTE: Since NODE_RX_TYPE_SYNC_ISO is only generated from ULL
		 *       context, pass ULL context as parameter.
		 */
		node_rx->rx_ftr.param = sync_iso;

		/* NOTE: struct node_rx_lost has uint8_t member store the reason.
		 */
		se = (void *)node_rx->pdu;
		se->status = BT_HCI_ERR_OP_CANCELLED_BY_HOST;

		*rx = node_rx;

		return BT_HCI_ERR_SUCCESS;
	}

	err = ull_ticker_stop_with_mark((TICKER_ID_SCAN_SYNC_ISO_BASE +
					 big_handle), sync_iso, &sync_iso->lll);
	LL_ASSERT_INFO2(err == 0 || err == -EALREADY, big_handle, err);
	if (err) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ull_sync_iso_stream_release(sync_iso);

	link_sync_lost = sync_iso->node_rx_lost.rx.hdr.link;
	ll_rx_link_release(link_sync_lost);

	return BT_HCI_ERR_SUCCESS;
}

int ull_sync_iso_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_sync_iso_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

uint8_t ull_sync_iso_lll_handle_get(struct lll_sync_iso *lll)
{
	return sync_iso_handle_get(HDR_LLL2ULL(lll));
}

struct ll_sync_iso_set *ull_sync_iso_by_stream_get(uint16_t handle)
{
	if (handle >= CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT) {
		return NULL;
	}

	return sync_iso_get(stream_pool[handle].big_handle);
}

struct lll_sync_iso_stream *ull_sync_iso_stream_get(uint16_t handle)
{
	struct ll_sync_iso_set *sync_iso;

	/* Get the BIG Sync context and check for not being terminated */
	sync_iso = ull_sync_iso_by_stream_get(handle);
	if (!sync_iso || !sync_iso->sync) {
		return NULL;
	}

	return &stream_pool[handle];
}

struct lll_sync_iso_stream *ull_sync_iso_lll_stream_get(uint16_t handle)
{
	return ull_sync_iso_stream_get(handle);
}

void ull_sync_iso_stream_release(struct ll_sync_iso_set *sync_iso)
{
	struct lll_sync_iso *lll;

	lll = &sync_iso->lll;
	while (lll->stream_count--) {
		struct lll_sync_iso_stream *stream;
		struct ll_iso_datapath *dp;
		uint16_t stream_handle;

		stream_handle = lll->stream_handle[lll->stream_count];
		stream = ull_sync_iso_stream_get(stream_handle);
		LL_ASSERT(stream);

		dp = stream->dp;
		if (dp) {
			stream->dp = NULL;
			isoal_sink_destroy(dp->sink_hdl);
			ull_iso_datapath_release(dp);
		}

		mem_release(stream, &stream_free);
	}

	sync_iso->sync = NULL;
}

void ull_sync_iso_setup(struct ll_sync_iso_set *sync_iso,
			struct node_rx_pdu *node_rx,
			uint8_t *acad, uint8_t acad_len)
{
	struct lll_sync_iso_stream *stream;
	uint32_t ticks_slot_overhead;
	uint32_t sync_iso_offset_us;
	uint32_t ticks_slot_offset;
	uint32_t ticks_threshold;
	struct lll_sync_iso *lll;
	struct node_rx_ftr *ftr;
	struct pdu_big_info *bi;
	uint32_t ready_delay_us;
	uint32_t ticks_expire;
	uint32_t ctrl_spacing;
	uint32_t pdu_spacing;
	uint32_t interval_us;
	uint32_t ticks_diff;
	struct pdu_adv *pdu;
	uint32_t slot_us;
	uint8_t num_bis;
	uint8_t bi_size;
	uint8_t handle;
	uint32_t ret;
	uint8_t sca;

	while (acad_len) {
		const uint8_t hdr_len = acad[PDU_ADV_DATA_HEADER_LEN_OFFSET];

		if ((hdr_len >= PDU_ADV_DATA_HEADER_TYPE_SIZE) &&
		    (acad[PDU_ADV_DATA_HEADER_TYPE_OFFSET] ==
		     BT_DATA_BIG_INFO)) {
			break;
		}

		if (acad_len < (hdr_len + PDU_ADV_DATA_HEADER_LEN_SIZE)) {
			return;
		}

		acad_len -= hdr_len + PDU_ADV_DATA_HEADER_LEN_SIZE;
		acad += hdr_len + PDU_ADV_DATA_HEADER_LEN_SIZE;
	}

	if ((acad_len < (PDU_BIG_INFO_CLEARTEXT_SIZE +
			 PDU_ADV_DATA_HEADER_SIZE)) ||
	    ((acad[PDU_ADV_DATA_HEADER_LEN_OFFSET] !=
	      (PDU_ADV_DATA_HEADER_TYPE_SIZE + PDU_BIG_INFO_CLEARTEXT_SIZE)) &&
	     (acad[PDU_ADV_DATA_HEADER_LEN_OFFSET] !=
	      (PDU_ADV_DATA_HEADER_TYPE_SIZE + PDU_BIG_INFO_ENCRYPTED_SIZE)))) {
		return;
	}

	bi_size = acad[PDU_ADV_DATA_HEADER_LEN_OFFSET] -
		  PDU_ADV_DATA_HEADER_TYPE_SIZE;
	bi = (void *)&acad[PDU_ADV_DATA_HEADER_DATA_OFFSET];

	lll = &sync_iso->lll;
	(void)memcpy(lll->seed_access_addr, &bi->seed_access_addr,
		     sizeof(lll->seed_access_addr));
	(void)memcpy(lll->base_crc_init, &bi->base_crc_init,
		     sizeof(lll->base_crc_init));

	(void)memcpy(lll->data_chan_map, bi->chm_phy,
		     sizeof(lll->data_chan_map));
	lll->data_chan_map[4] &= 0x1F;
	lll->data_chan_count = util_ones_count_get(lll->data_chan_map,
						   sizeof(lll->data_chan_map));
	if (lll->data_chan_count < CHM_USED_COUNT_MIN) {
		return;
	}

	/* Reset ISO create BIG flag in the periodic advertising context */
	sync_iso->sync->iso.sync_iso = NULL;

	lll->phy = BIT(bi->chm_phy[4] >> 5);

	lll->num_bis = PDU_BIG_INFO_NUM_BIS_GET(bi);
	lll->bn = PDU_BIG_INFO_BN_GET(bi);
	lll->nse = PDU_BIG_INFO_NSE_GET(bi);
	lll->sub_interval = PDU_BIG_INFO_SUB_INTERVAL_GET(bi);
	lll->max_pdu = bi->max_pdu;
	lll->pto = PDU_BIG_INFO_PTO_GET(bi);
	if (lll->pto) {
		lll->ptc = lll->bn;
	} else {
		lll->ptc = 0U;
	}
	lll->bis_spacing = PDU_BIG_INFO_SPACING_GET(bi);
	lll->irc = PDU_BIG_INFO_IRC_GET(bi);
	lll->sdu_interval = PDU_BIG_INFO_SDU_INTERVAL_GET(bi);

	/* Pick the 39-bit payload count, 1 MSb is framing bit */
	lll->payload_count = (uint64_t)bi->payload_count_framing[0];
	lll->payload_count |= (uint64_t)bi->payload_count_framing[1] << 8;
	lll->payload_count |= (uint64_t)bi->payload_count_framing[2] << 16;
	lll->payload_count |= (uint64_t)bi->payload_count_framing[3] << 24;
	lll->payload_count |= (uint64_t)(bi->payload_count_framing[4] & 0x7f) << 32;

	/* Set establishment event countdown */
	lll->establish_events = CONN_ESTAB_COUNTDOWN;

	if (lll->enc && (bi_size == PDU_BIG_INFO_ENCRYPTED_SIZE)) {
		const uint8_t BIG3[4]  = {0x33, 0x47, 0x49, 0x42};
		struct ccm *ccm_rx;
		uint8_t gsk[16];
		int err;

		/* Copy the GIV in BIGInfo */
		(void)memcpy(lll->giv, bi->giv, sizeof(lll->giv));

		/* Calculate GSK */
		err = bt_crypto_h8(sync_iso->gltk, bi->gskd, BIG3, gsk);
		LL_ASSERT(!err);

		/* Prepare the CCM parameters */
		ccm_rx = &lll->ccm_rx;
		ccm_rx->direction = 1U;
		(void)memcpy(&ccm_rx->iv[4], &lll->giv[4], 4U);
		(void)mem_rcopy(ccm_rx->key, gsk, sizeof(ccm_rx->key));

		/* NOTE: counter is filled in LLL */
	} else {
		lll->enc = 0U;
	}

	/* Initialize payload pointers */
	lll->payload_count_max = PDU_BIG_PAYLOAD_COUNT_MAX;
	lll->payload_tail = 0U;
	for (int i = 0; i < CONFIG_BT_CTLR_SYNC_ISO_STREAM_MAX; i++) {
		for (int j = 0; j < lll->payload_count_max; j++) {
			lll->payload[i][j] = NULL;
		}
	}

	lll->iso_interval = PDU_BIG_INFO_ISO_INTERVAL_GET(bi);
	interval_us = lll->iso_interval * PERIODIC_INT_UNIT_US;

	sync_iso->timeout_reload =
		RADIO_SYNC_EVENTS((sync_iso->timeout * 10U * USEC_PER_MSEC),
				  interval_us);

	sca = sync_iso->sync->lll.sca;
	lll->window_widening_periodic_us =
		DIV_ROUND_UP(((lll_clock_ppm_local_get() +
				   lll_clock_ppm_get(sca)) *
				 interval_us), USEC_PER_SEC);
	lll->window_widening_max_us = (interval_us >> 1) - EVENT_IFS_US;
	if (PDU_BIG_INFO_OFFS_UNITS_GET(bi)) {
		lll->window_size_event_us = OFFS_UNIT_300_US;
	} else {
		lll->window_size_event_us = OFFS_UNIT_30_US;
	}

	ftr = &node_rx->rx_ftr;
	pdu = (void *)((struct node_rx_pdu *)node_rx)->pdu;

	ready_delay_us = lll_radio_rx_ready_delay_get(lll->phy, PHY_FLAGS_S8);

	/* Calculate the BIG Offset in microseconds */
	sync_iso_offset_us = ftr->radio_end_us;
	sync_iso_offset_us += PDU_BIG_INFO_OFFS_GET(bi) *
			      lll->window_size_event_us;
	/* Skip to first selected BIS subevent */
	/* FIXME: add support for interleaved packing */
	stream = ull_sync_iso_stream_get(lll->stream_handle[0]);
	sync_iso_offset_us += (stream->bis_index - 1U) * lll->sub_interval *
			      ((lll->irc * lll->bn) + lll->ptc);
	sync_iso_offset_us -= PDU_AC_US(pdu->len, sync_iso->sync->lll.phy,
					ftr->phy_flags);
	sync_iso_offset_us -= EVENT_TICKER_RES_MARGIN_US;
	sync_iso_offset_us -= EVENT_JITTER_US;
	sync_iso_offset_us -= ready_delay_us;

	interval_us -= lll->window_widening_periodic_us;

	/* Calculate ISO Receiver BIG event timings */
	pdu_spacing = PDU_BIS_US(lll->max_pdu, lll->enc, lll->phy,
				 PHY_FLAGS_S8) +
		      EVENT_MSS_US;
	ctrl_spacing = PDU_BIS_US(sizeof(struct pdu_big_ctrl), lll->enc,
				  lll->phy, PHY_FLAGS_S8);

	/* Number of maximum BISes to sync from the first BIS to sync */
	/* NOTE: When ULL scheduling is implemented for subevents, then update
	 * the time reservation as required.
	 */
	num_bis = lll->num_bis - (stream->bis_index - 1U);

	/* 1. Maximum PDU transmission time in 1M/2M/S8 PHY is 17040 us, or
	 * represented in 15-bits.
	 * 2. NSE in the range 1 to 31 is represented in 5-bits
	 * 3. num_bis in the range 1 to 31 is represented in 5-bits
	 *
	 * Hence, worst case event time can be represented in 25-bits plus
	 * one each bit for added ctrl_spacing and radio event overheads. I.e.
	 * 27-bits required and sufficiently covered by using 32-bit data type
	 * for time_us.
	 */

	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_ISO_RESERVE_MAX)) {
		/* Maximum time reservation for both sequential and interleaved
		 * packing.
		 */
		slot_us = (pdu_spacing * lll->nse * num_bis) + ctrl_spacing;

	} else if (lll->bis_spacing >= (lll->sub_interval * lll->nse)) {
		/* Time reservation omitting PTC subevents in sequetial
		 * packing.
		 */
		slot_us = pdu_spacing * ((lll->nse * num_bis) - lll->ptc);

	} else {
		/* Time reservation omitting PTC subevents in interleaved
		 * packing.
		 */
		slot_us = pdu_spacing * ((lll->nse - lll->ptc) * num_bis);
	}

	/* Add radio ready delay */
	slot_us += ready_delay_us;

	/* Add implementation defined radio event overheads */
	if (IS_ENABLED(CONFIG_BT_CTLR_EVENT_OVERHEAD_RESERVE_MAX)) {
		slot_us += EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;
	}

	/* TODO: active_to_start feature port */
	sync_iso->ull.ticks_active_to_start = 0U;
	sync_iso->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	sync_iso->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	sync_iso->ull.ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(slot_us);

	ticks_slot_offset = MAX(sync_iso->ull.ticks_active_to_start,
				sync_iso->ull.ticks_prepare_to_start);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}
	ticks_slot_offset += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	/* Check and skip to next interval if CPU processing introduces latency
	 * that can delay scheduling the first ISO event.
	 */
	ticks_expire = ftr->ticks_anchor - ticks_slot_offset +
		       HAL_TICKER_US_TO_TICKS(sync_iso_offset_us);
	ticks_threshold = ticker_ticks_now_get() +
			  HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);
	ticks_diff = ticker_ticks_diff_get(ticks_expire, ticks_threshold);
	if (ticks_diff & BIT(HAL_TICKER_CNTR_MSBIT)) {
		sync_iso_offset_us += interval_us -
			lll->window_widening_periodic_us;
		lll->window_widening_event_us +=
			lll->window_widening_periodic_us;
		lll->payload_count += lll->bn;
	}

	/* setup to use ISO create prepare function until sync established */
	mfy_lll_prepare.fp = lll_sync_iso_create_prepare;

	handle = sync_iso_handle_get(sync_iso);
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			   (TICKER_ID_SCAN_SYNC_ISO_BASE + handle),
			   ftr->ticks_anchor - ticks_slot_offset,
			   HAL_TICKER_US_TO_TICKS(sync_iso_offset_us),
			   HAL_TICKER_US_TO_TICKS(interval_us),
			   HAL_TICKER_REMAINDER(interval_us),
#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_CTLR_LOW_LAT)
			   TICKER_LAZY_MUST_EXPIRE,
#else
			   TICKER_NULL_LAZY,
#endif /* !CONFIG_BT_TICKER_LOW_LAT && !CONFIG_BT_CTLR_LOW_LAT */
			   (sync_iso->ull.ticks_slot + ticks_slot_overhead),
			   ticker_cb, sync_iso,
			   ticker_start_op_cb, (void *)__LINE__);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

void ull_sync_iso_estab_done(struct node_rx_event_done *done)
{
	struct ll_sync_iso_set *sync_iso;
	struct node_rx_sync_iso *se;
	struct node_rx_pdu *rx;

	if (done->extra.trx_cnt || done->extra.estab_failed) {
		/* Switch to normal prepare */
		mfy_lll_prepare.fp = lll_sync_iso_prepare;

		/* Get reference to ULL context */
		sync_iso = CONTAINER_OF(done->param, struct ll_sync_iso_set, ull);

		/* Prepare BIG Sync Established */
		rx = (void *)sync_iso->sync->iso.node_rx_estab;
		rx->hdr.type = NODE_RX_TYPE_SYNC_ISO;
		rx->hdr.handle = sync_iso_handle_get(sync_iso);
		rx->rx_ftr.param = sync_iso;

		se = (void *)rx->pdu;
		se->status = done->extra.estab_failed ?
			BT_HCI_ERR_CONN_FAIL_TO_ESTAB : BT_HCI_ERR_SUCCESS;

		ll_rx_put_sched(rx->hdr.link, rx);
	}

	ull_sync_iso_done(done);
}

void ull_sync_iso_done(struct node_rx_event_done *done)
{
	struct ll_sync_iso_set *sync_iso;
	uint32_t ticks_drift_minus;
	uint32_t ticks_drift_plus;
	struct lll_sync_iso *lll;
	uint16_t elapsed_event;
	uint16_t latency_event;
	uint16_t lazy;
	uint8_t force;

	/* Get reference to ULL context */
	sync_iso = CONTAINER_OF(done->param, struct ll_sync_iso_set, ull);
	lll = &sync_iso->lll;

	/* Events elapsed used in timeout checks below */
	latency_event = lll->latency_event;
	if (lll->latency_prepare) {
		elapsed_event = latency_event + lll->latency_prepare;
	} else {
		elapsed_event = latency_event + 1U;
	}

	/* Sync drift compensation and new skip calculation
	 */
	ticks_drift_plus = 0U;
	ticks_drift_minus = 0U;
	if (done->extra.trx_cnt) {
		/* Calculate drift in ticks unit */
		ull_drift_ticks_get(done, &ticks_drift_plus,
				    &ticks_drift_minus);

		/* Reset latency */
		lll->latency_event = 0U;
	}

	/* Reset supervision countdown */
	if (done->extra.crc_valid) {
		sync_iso->timeout_expire = 0U;
	} else {
		/* if anchor point not sync-ed, start timeout countdown */
		if (!sync_iso->timeout_expire) {
			sync_iso->timeout_expire = sync_iso->timeout_reload;
		}
	}

	/* check timeout */
	force = 0U;
	if (sync_iso->timeout_expire) {
		if (sync_iso->timeout_expire > elapsed_event) {
			sync_iso->timeout_expire -= elapsed_event;

			/* break skip */
			lll->latency_event = 0U;

			if (latency_event) {
				force = 1U;
			}
		} else {
			timeout_cleanup(sync_iso);

			return;
		}
	}

	/* check if skip needs update */
	lazy = 0U;
	if (force || (latency_event != lll->latency_event)) {
		lazy = lll->latency_event + 1U;
	}

	/* Update Sync ticker instance */
	if (ticks_drift_plus || ticks_drift_minus || lazy || force) {
		uint8_t handle = sync_iso_handle_get(sync_iso);
		uint32_t ticker_status;

		/* Call to ticker_update can fail under the race
		 * condition where in the periodic sync role is being stopped
		 * but at the same time it is preempted by periodic sync event
		 * that gets into close state. Accept failure when periodic sync
		 * role is being stopped.
		 */
		ticker_status = ticker_update(TICKER_INSTANCE_ID_CTLR,
					      TICKER_USER_ID_ULL_HIGH,
					      (TICKER_ID_SCAN_SYNC_ISO_BASE +
					       handle),
					      ticks_drift_plus,
					      ticks_drift_minus, 0U, 0U,
					      lazy, force,
					      ticker_update_op_cb,
					      sync_iso);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY) ||
			  ((void *)sync_iso == ull_disable_mark_get()));
	}
}

void ull_sync_iso_done_terminate(struct node_rx_event_done *done)
{
	struct ll_sync_iso_set *sync_iso;
	struct lll_sync_iso *lll;
	struct node_rx_pdu *rx;
	uint8_t handle;
	uint32_t ret;

	/* Get reference to ULL context */
	sync_iso = CONTAINER_OF(done->param, struct ll_sync_iso_set, ull);
	lll = &sync_iso->lll;

	/* Populate the Sync Lost which will be enqueued in disabled_cb */
	rx = (void *)&sync_iso->node_rx_lost;
	rx->hdr.handle = sync_iso_handle_get(sync_iso);
	rx->hdr.type = NODE_RX_TYPE_SYNC_ISO_LOST;
	rx->rx_ftr.param = sync_iso;
	*((uint8_t *)rx->pdu) = lll->term_reason;

	/* Stop Sync ISO Ticker */
	handle = sync_iso_handle_get(sync_iso);
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			  (TICKER_ID_SCAN_SYNC_ISO_BASE + handle),
			  ticker_stop_op_cb, (void *)sync_iso);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

uint32_t ull_big_sync_delay(const struct lll_sync_iso *lll_iso)
{
	/* BT Core v5.4 - Vol 6, Part B, Section 4.4.6.4:
	 * BIG_Sync_Delay = (Num_BIS – 1) × BIS_Spacing + (NSE – 1) × Sub_Interval + MPT.
	 */
	return (lll_iso->num_bis - 1) * lll_iso->bis_spacing +
		(lll_iso->nse - 1) * lll_iso->sub_interval +
		BYTES2US(PDU_OVERHEAD_SIZE(lll_iso->phy) +
			lll_iso->max_pdu + (lll_iso->enc ? 4 : 0),
			lll_iso->phy);
}

static void disable(uint8_t sync_idx)
{
	struct ll_sync_iso_set *sync_iso;
	int err;

	sync_iso = &ll_sync_iso[sync_idx];

	err = ull_ticker_stop_with_mark(TICKER_ID_SCAN_SYNC_ISO_BASE +
					sync_idx, sync_iso, &sync_iso->lll);
	LL_ASSERT_INFO2(err == 0 || err == -EALREADY, sync_idx, err);
}

static int init_reset(void)
{
	uint8_t idx;

	/* Disable all active BIGs (uses blocking ull_ticker_stop_with_mark) */
	for (idx = 0U; idx < CONFIG_BT_CTLR_SCAN_SYNC_ISO_SET; idx++) {
		disable(idx);
	}

	mem_init((void *)stream_pool, sizeof(struct lll_sync_iso_stream),
		 CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT, &stream_free);

	memset(&ll_sync_iso, 0, sizeof(ll_sync_iso));

	/* Initialize LLL */
	return lll_sync_iso_init();
}

static struct ll_sync_iso_set *sync_iso_get(uint8_t handle)
{
	if (handle >= CONFIG_BT_CTLR_SCAN_SYNC_ISO_SET) {
		return NULL;
	}

	return &ll_sync_iso[handle];
}

static uint8_t sync_iso_handle_get(struct ll_sync_iso_set *sync)
{
	return mem_index_get(sync, ll_sync_iso, sizeof(*sync));
}

static struct stream *sync_iso_stream_acquire(void)
{
	return mem_acquire(&stream_free);
}

static uint16_t sync_iso_stream_handle_get(struct lll_sync_iso_stream *stream)
{
	return mem_index_get(stream, stream_pool, sizeof(*stream));
}

static void timeout_cleanup(struct ll_sync_iso_set *sync_iso)
{
	struct node_rx_pdu *rx;
	uint8_t handle;
	uint32_t ret;

	/* Populate the Sync Lost which will be enqueued in disabled_cb */
	rx = (void *)&sync_iso->node_rx_lost;
	rx->hdr.handle = sync_iso_handle_get(sync_iso);
	rx->rx_ftr.param = sync_iso;

	if (mfy_lll_prepare.fp == lll_sync_iso_prepare) {
		rx->hdr.type = NODE_RX_TYPE_SYNC_ISO_LOST;
		*((uint8_t *)rx->pdu) = BT_HCI_ERR_CONN_TIMEOUT;
	} else {
		rx->hdr.type = NODE_RX_TYPE_SYNC_ISO;
		*((uint8_t *)rx->pdu) = BT_HCI_ERR_CONN_FAIL_TO_ESTAB;
	}

	/* Stop Sync ISO Ticker */
	handle = sync_iso_handle_get(sync_iso);
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			  (TICKER_ID_SCAN_SYNC_ISO_BASE + handle),
			  ticker_stop_op_cb, (void *)sync_iso);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
{
	static struct lll_prepare_param p;
	struct ll_sync_iso_set *sync_iso;
	struct lll_sync_iso *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_O(1);

	if (!IS_ENABLED(CONFIG_BT_TICKER_LOW_LAT) &&
	    !IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) &&
	    (lazy == TICKER_LAZY_MUST_EXPIRE)) {
		/* FIXME: generate ISO PDU with status set to invalid */

		DEBUG_RADIO_PREPARE_O(0);
		return;
	}

	sync_iso = param;
	lll = &sync_iso->lll;

	/* Increment prepare reference count */
	ref = ull_ref_inc(&sync_iso->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.force = force;
	p.param = lll;
	mfy_lll_prepare.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL, 0U,
			     &mfy_lll_prepare);
	LL_ASSERT(!ret);

	DEBUG_RADIO_PREPARE_O(1);
}

static void ticker_start_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

static void ticker_update_op_cb(uint32_t status, void *param)
{
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_disable_mark_get());
}

static void ticker_stop_op_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0U, 0U, &link, NULL, sync_iso_disable};
	uint32_t ret;

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	/* Check if any pending LLL events that need to be aborted */
	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
			     TICKER_USER_ID_ULL_HIGH, 0U, &mfy);
	LL_ASSERT(!ret);
}

static void sync_iso_disable(void *param)
{
	struct ll_sync_iso_set *sync_iso;
	struct ull_hdr *hdr;

	/* Check ref count to determine if any pending LLL events in pipeline */
	sync_iso = param;
	hdr = &sync_iso->ull;
	if (ull_ref_get(hdr)) {
		static memq_link_t link;
		static struct mayfly mfy = {0U, 0U, &link, NULL, lll_disable};
		uint32_t ret;

		mfy.param = &sync_iso->lll;

		/* Setup disabled callback to be called when ref count
		 * returns to zero.
		 */
		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = mfy.param;
		hdr->disabled_cb = disabled_cb;

		/* Trigger LLL disable */
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0U, &mfy);
		LL_ASSERT(!ret);
	} else {
		/* No pending LLL events */
		disabled_cb(&sync_iso->lll);
	}
}

static void disabled_cb(void *param)
{
	struct ll_sync_iso_set *sync_iso;
	struct node_rx_pdu *rx;
	memq_link_t *link;

	/* Get reference to ULL context */
	sync_iso = HDR_LLL2ULL(param);

	/* Generate BIG sync lost */
	rx = (void *)&sync_iso->node_rx_lost;
	LL_ASSERT(rx->hdr.link);
	link = rx->hdr.link;
	rx->hdr.link = NULL;

	/* Enqueue the BIG sync lost towards ULL context */
	ll_rx_put_sched(link, rx);
}
