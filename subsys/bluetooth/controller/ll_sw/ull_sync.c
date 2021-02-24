/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>
#include <bluetooth/hci.h>

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll/lll_vendor.h"
#include "lll_scan.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"

#include "ull_scan_types.h"
#include "ull_sync_types.h"

#include "ull_internal.h"
#include "ull_scan_internal.h"
#include "ull_sync_internal.h"

#include "ll.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_sync
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_sync_set *sync_acquire(void);
static void timeout_cleanup(struct ll_sync_set *sync);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param);
static void ticker_op_cb(uint32_t status, void *param);
static void ticker_update_sync_op_cb(uint32_t status, void *param);
static void ticker_stop_op_cb(uint32_t status, void *param);
static void sync_lost(void *param);

static struct ll_sync_set ll_sync_pool[CONFIG_BT_PER_ADV_SYNC_MAX];
static void *sync_free;

uint8_t ll_sync_create(uint8_t options, uint8_t sid, uint8_t adv_addr_type,
			    uint8_t *adv_addr, uint16_t skip,
			    uint16_t sync_timeout, uint8_t sync_cte_type)
{
	struct ll_scan_set *scan_coded;
	memq_link_t *link_sync_estab;
	memq_link_t *link_sync_lost;
	struct node_rx_hdr *node_rx;
	struct lll_sync *lll_sync;
	struct ll_scan_set *scan;
	struct ll_sync_set *sync;

	scan = ull_scan_set_get(SCAN_HANDLE_1M);
	if (!scan || scan->per_scan.sync) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
		if (!scan_coded || scan_coded->per_scan.sync) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
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

	sync = sync_acquire();
	if (!sync) {
		ll_rx_release(node_rx);
		ll_rx_link_release(link_sync_lost);
		ll_rx_link_release(link_sync_estab);

		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	node_rx->link = link_sync_estab;
	scan->per_scan.node_rx_estab = node_rx;
	scan->per_scan.state = LL_SYNC_STATE_IDLE;
	scan->per_scan.filter_policy = options & BIT(0);
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded->per_scan.state = LL_SYNC_STATE_IDLE;
		scan_coded->per_scan.node_rx_estab =
			scan->per_scan.node_rx_estab;
		scan_coded->per_scan.filter_policy =
			scan->per_scan.filter_policy;
	}

	if (!scan->per_scan.filter_policy) {
		scan->per_scan.sid = sid;
		scan->per_scan.adv_addr_type = adv_addr_type;
		memcpy(scan->per_scan.adv_addr, adv_addr, BDADDR_SIZE);

		if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
			scan_coded->per_scan.sid = scan->per_scan.sid;
			scan_coded->per_scan.adv_addr_type =
				scan->per_scan.adv_addr_type;
			memcpy(scan_coded->per_scan.adv_addr,
			       scan->per_scan.adv_addr, BDADDR_SIZE);
		}
	}

	sync->skip = skip;
	sync->timeout = sync_timeout;

	/* TODO: Support for CTE type */

	/* Initialize sync context */
	sync->timeout_reload = 0U;
	sync->timeout_expire = 0U;

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	/* Reset Broadcast Isochronous Group Sync Establishment */
	sync->iso.sync_iso = NULL;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

	/* Initialize sync LLL context */
	lll_sync = &sync->lll;
	lll_sync->skip_prepare = 0U;
	lll_sync->skip_event = 0U;
	lll_sync->data_chan_id = 0U;
	lll_sync->window_widening_prepare_us = 0U;
	lll_sync->window_widening_event_us = 0U;

	/* Reporting initially enabled/disabled */
	lll_sync->is_rx_enabled = options & BIT(1);

	/* sync_lost node_rx */
	sync->node_rx_lost.hdr.link = link_sync_lost;

	/* Initialise ULL and LLL headers */
	ull_hdr_init(&sync->ull);
	lll_hdr_init(lll_sync, sync);

	/* Enable scanner to create sync */
	scan->per_scan.sync = sync;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded->per_scan.sync = sync;
	}

	return 0;
}

uint8_t ll_sync_create_cancel(void **rx)
{
	struct ll_scan_set *scan_coded;
	memq_link_t *link_sync_estab;
	memq_link_t *link_sync_lost;
	struct node_rx_pdu *node_rx;
	struct ll_scan_set *scan;
	struct ll_sync_set *sync;
	struct node_rx_sync *se;

	scan = ull_scan_set_get(SCAN_HANDLE_1M);
	if (!scan || !scan->per_scan.sync) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
		if (!scan_coded || !scan_coded->per_scan.sync) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}

	/* Check for race condition where in sync is established when sync
	 * context was set to NULL.
	 */
	sync = scan->per_scan.sync;
	if (!sync || sync->timeout_reload) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	scan->per_scan.sync = NULL;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded->per_scan.sync = NULL;
	}

	node_rx = (void *)scan->per_scan.node_rx_estab;
	link_sync_estab = node_rx->hdr.link;
	link_sync_lost = sync->node_rx_lost.hdr.link;

	ll_rx_link_release(link_sync_lost);
	ll_rx_link_release(link_sync_estab);
	ll_rx_release(node_rx);

	node_rx = (void *)&sync->node_rx_lost;
	node_rx->hdr.type = NODE_RX_TYPE_SYNC;
	node_rx->hdr.handle = 0xffff;

	/* NOTE: struct node_rx_lost has uint8_t member following the
	 *       struct node_rx_hdr to store the reason.
	 */
	se = (void *)node_rx->pdu;
	se->status = BT_HCI_ERR_OP_CANCELLED_BY_HOST;

	/* NOTE: Since NODE_RX_TYPE_SYNC is only generated from ULL context,
	 *       pass ULL context as parameter.
	 */
	node_rx->hdr.rx_ftr.param = sync;

	*rx = node_rx;

	return 0;
}

uint8_t ll_sync_terminate(uint16_t handle)
{
	memq_link_t *link_sync_lost;
	struct ll_sync_set *sync;
	int err;

	sync = ull_sync_is_enabled_get(handle);
	if (!sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	err = ull_ticker_stop_with_mark(TICKER_ID_SCAN_SYNC_BASE + handle,
					sync, &sync->lll);
	LL_ASSERT(err == 0 || err == -EALREADY);
	if (err) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	link_sync_lost = sync->node_rx_lost.hdr.link;
	ll_rx_link_release(link_sync_lost);

	ull_sync_release(sync);

	return 0;
}

uint8_t ll_sync_recv_enable(uint16_t handle, uint8_t enable)
{
	/* TODO: */
	return BT_HCI_ERR_CMD_DISALLOWED;
}

int ull_sync_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_sync_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

struct ll_sync_set *ull_sync_set_get(uint16_t handle)
{
	if (handle >= CONFIG_BT_PER_ADV_SYNC_MAX) {
		return NULL;
	}

	return &ll_sync_pool[handle];
}

struct ll_sync_set *ull_sync_is_enabled_get(uint16_t handle)
{
	struct ll_sync_set *sync;

	sync = ull_sync_set_get(handle);
	if (!sync || !sync->timeout_reload) {
		return NULL;
	}

	return sync;
}

uint16_t ull_sync_handle_get(struct ll_sync_set *sync)
{
	return mem_index_get(sync, ll_sync_pool, sizeof(struct ll_sync_set));
}

uint16_t ull_sync_lll_handle_get(struct lll_sync *lll)
{
	return ull_sync_handle_get((void *)HDR_LLL2EVT(lll));
}

void ull_sync_release(struct ll_sync_set *sync)
{
	mem_release(sync, &sync_free);
}

void ull_sync_setup(struct ll_scan_set *scan, struct ll_scan_aux_set *aux,
		    struct node_rx_hdr *node_rx, struct pdu_adv_sync_info *si)
{
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;
	struct ll_sync_set *sync;
	struct node_rx_sync *se;
	struct node_rx_ftr *ftr;
	uint32_t sync_offset_us;
	uint32_t ready_delay_us;
	struct node_rx_pdu *rx;
	struct lll_sync *lll;
	uint16_t sync_handle;
	uint32_t interval_us;
	struct pdu_adv *pdu;
	uint16_t interval;
	uint32_t ret;
	uint8_t sca;

	sync = scan->per_scan.sync;
	scan->per_scan.sync = NULL;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		struct ll_scan_set *scan_1m;

		scan_1m = ull_scan_set_get(SCAN_HANDLE_1M);
		if (scan == scan_1m) {
			struct ll_scan_set *scan_coded;

			scan_coded = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
			scan_coded->per_scan.sync = NULL;
		} else {
			scan_1m->per_scan.sync = NULL;
		}
	}

	lll = &sync->lll;
	memcpy(lll->data_chan_map, si->sca_chm, sizeof(lll->data_chan_map));
	lll->data_chan_map[4] &= ~0xE0;
	lll->data_chan_count = util_ones_count_get(&lll->data_chan_map[0],
						   sizeof(lll->data_chan_map));
	if (lll->data_chan_count < 2) {
		return;
	}

	memcpy(lll->access_addr, &si->aa, sizeof(lll->access_addr));
	memcpy(lll->crc_init, si->crc_init, sizeof(lll->crc_init));
	lll->event_counter = si->evt_cntr;
	lll->phy = aux->lll.phy;

	sca = si->sca_chm[4] >> 5;
	interval = sys_le16_to_cpu(si->interval);
	interval_us = interval * CONN_INT_UNIT_US;

	sync->timeout_reload = RADIO_SYNC_EVENTS((sync->timeout * 10U * 1000U),
						 interval_us);

	lll->window_widening_periodic_us =
		(((lll_clock_ppm_local_get() + lll_clock_ppm_get(sca)) *
		  interval_us) + (1000000 - 1)) / 1000000U;
	lll->window_widening_max_us = (interval_us >> 1) - EVENT_IFS_US;
	if (si->offs_units) {
		lll->window_size_event_us = 300U;
	} else {
		lll->window_size_event_us = 30U;
	}

	sync_handle = ull_sync_handle_get(sync);

	rx = (void *)scan->per_scan.node_rx_estab;
	rx->hdr.type = NODE_RX_TYPE_SYNC;
	rx->hdr.handle = sync_handle;
	rx->hdr.rx_ftr.param = scan;
	se = (void *)rx->pdu;
	se->status = BT_HCI_ERR_SUCCESS;
	se->interval = interval;
	se->phy = lll->phy;
	se->sca = sca;

	ll_rx_put(rx->hdr.link, rx);
	ll_rx_sched();

	ftr = &node_rx->rx_ftr;
	pdu = (void *)((struct node_rx_pdu *)node_rx)->pdu;

	ready_delay_us = lll_radio_rx_ready_delay_get(lll->phy, 1);

	sync_offset_us = ftr->radio_end_us;
	sync_offset_us += (uint32_t)si->offs * lll->window_size_event_us;
	/* offs_adjust may be 1 only if sync setup by LL_PERIODIC_SYNC_IND */
	sync_offset_us += (si->offs_adjust ? OFFS_ADJUST_US : 0U);
	sync_offset_us -= PKT_AC_US(pdu->len, 0, lll->phy);
	sync_offset_us -= EVENT_OVERHEAD_START_US;
	sync_offset_us -= EVENT_JITTER_US;
	sync_offset_us -= ready_delay_us;

	interval_us -= lll->window_widening_periodic_us;

	/* TODO: active_to_start feature port */
	sync->evt.ticks_active_to_start = 0U;
	sync->evt.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	sync->evt.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	sync->evt.ticks_slot =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US +
				       ready_delay_us +
				       PKT_AC_US(PDU_AC_EXT_PAYLOAD_SIZE_MAX,
						 0, lll->phy) +
				       EVENT_OVERHEAD_END_US);

	ticks_slot_offset = MAX(sync->evt.ticks_active_to_start,
				sync->evt.ticks_xtal_to_start);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}

	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			   (TICKER_ID_SCAN_SYNC_BASE + sync_handle),
			   ftr->ticks_anchor - ticks_slot_offset,
			   HAL_TICKER_US_TO_TICKS(sync_offset_us),
			   HAL_TICKER_US_TO_TICKS(interval_us),
			   HAL_TICKER_REMAINDER(interval_us),
			   TICKER_NULL_LAZY,
			   (sync->evt.ticks_slot + ticks_slot_overhead),
			   ticker_cb, sync, ticker_op_cb, (void *)__LINE__);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

void ull_sync_done(struct node_rx_event_done *done)
{
	struct lll_sync *lll = (void *)HDR_ULL2LLL(done->param);
	struct ll_sync_set *sync = (void *)HDR_LLL2EVT(lll);
	uint32_t ticks_drift_minus;
	uint32_t ticks_drift_plus;
	uint16_t elapsed_event;
	uint16_t skip_event;
	uint16_t lazy;
	uint8_t force;

	/* Events elapsed used in timeout checks below */
	skip_event = lll->skip_event;
	elapsed_event = skip_event + 1;

	/* Sync drift compensation and new skip calculation
	 */
	ticks_drift_plus = 0U;
	ticks_drift_minus = 0U;
	if (done->extra.trx_cnt) {
		/* Calculate drift in ticks unit */
		ull_drift_ticks_get(done, &ticks_drift_plus,
				    &ticks_drift_minus);

		/* Enforce skip */
		lll->skip_event = sync->skip;
	}

	/* Reset supervision countdown */
	if (done->extra.crc_valid) {
		sync->timeout_expire = 0U;
	}

	/* if anchor point not sync-ed, start timeout countdown, and break
	 * skip if any.
	 */
	else {
		if (!sync->timeout_expire) {
			sync->timeout_expire = sync->timeout_reload;
		}
	}

	/* check timeout */
	force = 0U;
	if (sync->timeout_expire) {
		if (sync->timeout_expire > elapsed_event) {
			sync->timeout_expire -= elapsed_event;

			/* break skip */
			lll->skip_event = 0U;

			if (skip_event) {
				force = 1U;
			}
		} else {
			timeout_cleanup(sync);

			return;
		}
	}

	/* check if skip needs update */
	lazy = 0U;
	if ((force) || (skip_event != lll->skip_event)) {
		lazy = lll->skip_event + 1U;
	}

	/* Update Sync ticker instance */
	if (ticks_drift_plus || ticks_drift_minus || lazy || force) {
		uint16_t sync_handle = ull_sync_handle_get(sync);
		uint32_t ticker_status;

		/* Call to ticker_update can fail under the race
		 * condition where in the periodic sync role is being stopped
		 * but at the same time it is preempted by periodic sync event
		 * that gets into close state. Accept failure when periodic sync
		 * role is being stopped.
		 */
		ticker_status = ticker_update(TICKER_INSTANCE_ID_CTLR,
					      TICKER_USER_ID_ULL_HIGH,
					      (TICKER_ID_SCAN_SYNC_BASE +
					       sync_handle),
					      ticks_drift_plus,
					      ticks_drift_minus, 0, 0,
					      lazy, force,
					      ticker_update_sync_op_cb, sync);
		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY) ||
			  ((void *)sync == ull_disable_mark_get()));
	}
}

static int init_reset(void)
{
	/* Initialize sync pool. */
	mem_init(ll_sync_pool, sizeof(struct ll_sync_set),
		 sizeof(ll_sync_pool) / sizeof(struct ll_sync_set),
		 &sync_free);

	return 0;
}

static inline struct ll_sync_set *sync_acquire(void)
{
	return mem_acquire(&sync_free);
}

static void timeout_cleanup(struct ll_sync_set *sync)
{
	uint16_t sync_handle = ull_sync_handle_get(sync);
	uint32_t ret;

	/* Stop Periodic Sync Ticker */
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			  TICKER_ID_SCAN_SYNC_BASE + sync_handle,
			  ticker_stop_op_cb, (void *)sync);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_sync_prepare};
	static struct lll_prepare_param p;
	struct ll_sync_set *sync = param;
	struct lll_sync *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_O(1);

	lll = &sync->lll;

	/* Increment prepare reference count */
	ref = ull_ref_inc(&sync->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.param = lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
			     TICKER_USER_ID_LLL, 0, &mfy);
	LL_ASSERT(!ret);

	DEBUG_RADIO_PREPARE_O(1);
}

static void ticker_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

static void ticker_update_sync_op_cb(uint32_t status, void *param)
{
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_disable_mark_get());
}

static void ticker_stop_op_cb(uint32_t status, void *param)
{
	uint32_t retval;
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, sync_lost};

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	mfy.param = param;

	retval = mayfly_enqueue(TICKER_USER_ID_ULL_LOW, TICKER_USER_ID_ULL_HIGH,
				0, &mfy);
	LL_ASSERT(!retval);
}

static void sync_lost(void *param)
{
	struct ll_sync_set *sync = param;
	struct node_rx_pdu *rx;

	/* Generate Periodic advertising sync lost */
	rx = (void *)&sync->node_rx_lost;
	rx->hdr.handle = ull_sync_handle_get(sync);
	rx->hdr.type = NODE_RX_TYPE_SYNC_LOST;
	rx->hdr.rx_ftr.param = sync;

	/* Enqueue the sync lost towards ULL context */
	ll_rx_put(rx->hdr.link, rx);
	ll_rx_sched();
}
