/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <soc.h>
#include <sys/byteorder.h>
#include <bluetooth/hci.h>

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll_clock.h"
#include "lll/lll_vendor.h"
#include "lll_chan.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"

#include "ull_filter.h"
#include "ull_scan_types.h"
#include "ull_sync_types.h"

#include "ull_internal.h"
#include "ull_scan_internal.h"
#include "ull_sync_internal.h"
#include "ull_df_types.h"
#include "ull_df_internal.h"

#include "ll.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_sync
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_sync_set *sync_acquire(void);
static void sync_ticker_cleanup(struct ll_sync_set *sync, ticker_op_func stop_op_cb);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);
static void ticker_start_op_cb(uint32_t status, void *param);
static void ticker_update_op_cb(uint32_t status, void *param);
static void ticker_stop_sync_expire_op_cb(uint32_t status, void *param);
static void sync_expire(void *param);
static void ticker_stop_sync_lost_op_cb(uint32_t status, void *param);
static void sync_lost(void *param);
#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC)
static bool peer_sid_sync_exists(uint8_t const peer_id_addr_type,
				 uint8_t const *const peer_id_addr,
				 uint8_t sid);
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC */
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING) && \
	!defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
static struct pdu_cte_info *pdu_cte_info_get(struct pdu_adv *pdu);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING && !CONFIG_BT_CTLR_CTEINLINE_SUPPORT */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
static void ticker_update_op_status_give(uint32_t status, void *param);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

static struct ll_sync_set ll_sync_pool[CONFIG_BT_PER_ADV_SYNC_MAX];
static void *sync_free;

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
/* Semaphore to wakeup thread on ticker API callback */
static struct k_sem sem_ticker_cb;
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

static memq_link_t link_lll_prepare;
static struct mayfly mfy_lll_prepare = { 0, 0, &link_lll_prepare, NULL, lll_sync_prepare };

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

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC)
	/* Do not sync twice to the same peer and same SID */
	if (((options & BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_USE_LIST) == 0U) &&
	    peer_sid_sync_exists(adv_addr_type, adv_addr, sid)) {
		return BT_HCI_ERR_CONN_ALREADY_EXISTS;
	}
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC */

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
		if (!scan_coded || scan_coded->per_scan.sync) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}

	/* FIXME: Check for already synchronized to same peer */

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

	scan->per_scan.state = LL_SYNC_STATE_IDLE;
	scan->per_scan.filter_policy =
		options & BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_USE_LIST;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded->per_scan.state = LL_SYNC_STATE_IDLE;
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

	/* Initialize sync context */
	node_rx->link = link_sync_estab;
	sync->node_rx_sync_estab = node_rx;
	sync->node_rx_lost.hdr.link = link_sync_lost;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
	sync->nodups = (options &
			BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_FILTER_DUPLICATE) ?
		       1U : 0U;
#endif
	sync->skip = skip;

	/* NOTE: Use timeout not zero to represent sync context used for sync
	 * create.
	 */
	sync->timeout = sync_timeout;

	/* NOTE: Use timeout_reload not zero to represent sync established. */
	sync->timeout_reload = 0U;
	sync->timeout_expire = 0U;

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC)
	/* Remember the peer address when periodic advertiser list is not
	 * used.
	 * NOTE: Peer address will be filled/overwritten with correct identity
	 * address on sync setup when privacy is enabled.
	 */
	if ((options & BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_USE_LIST) == 0U) {
		sync->peer_id_addr_type = adv_addr_type;
		(void)memcpy(sync->peer_id_addr, adv_addr,
			     sizeof(sync->peer_id_addr));
	}

	/* Remember the SID */
	sync->sid = sid;
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC */

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	/* Reset Broadcast Isochronous Group Sync Establishment */
	sync->iso.sync_iso = NULL;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

	/* Initialize sync LLL context */
	lll_sync = &sync->lll;
	lll_sync->skip_prepare = 0U;
	lll_sync->skip_event = 0U;
	lll_sync->window_widening_prepare_us = 0U;
	lll_sync->window_widening_event_us = 0U;
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING)
	lll_sync->cte_type = sync_cte_type;
	lll_sync->filter_policy = scan->per_scan.filter_policy;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING */

	/* TODO: Add support for reporting initially enabled/disabled */
	lll_sync->is_rx_enabled =
		!(options & BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_REPORTS_DISABLED);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	ull_df_sync_cfg_init(&lll_sync->df_cfg);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

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
	 *
	 * Setting `scan->per_scan.sync` to NULL represents cancellation
	 * requested in the thread context. Checking `sync->timeout_reload`
	 * confirms if synchronization was established before
	 * `scan->per_scan.sync` was set to NULL.
	 */
	sync = scan->per_scan.sync;
	scan->per_scan.sync = NULL;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		scan_coded->per_scan.sync = NULL;
	}
	cpu_dmb();
	if (!sync || sync->timeout_reload) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Mark the sync context as sync create cancelled */
	if (IS_ENABLED(CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC)) {
		sync->timeout = 0U;
	}

	node_rx = (void *)sync->node_rx_sync_estab;
	link_sync_estab = node_rx->hdr.link;
	link_sync_lost = sync->node_rx_lost.hdr.link;

	ll_rx_link_release(link_sync_lost);
	ll_rx_link_release(link_sync_estab);
	ll_rx_release(node_rx);

	node_rx = (void *)&sync->node_rx_lost;
	node_rx->hdr.type = NODE_RX_TYPE_SYNC;
	node_rx->hdr.handle = LLL_HANDLE_INVALID;

	/* NOTE: struct node_rx_lost has uint8_t member following the
	 *       struct node_rx_hdr to store the reason.
	 */
	se = (void *)node_rx->pdu;
	se->status = BT_HCI_ERR_OP_CANCELLED_BY_HOST;

	/* NOTE: Since NODE_RX_TYPE_SYNC is only generated from ULL context,
	 *       pass ULL sync context as parameter.
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
	/* TODO: Add support for reporting enable/disable */
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
	uint16_t handle;
	void *rx;
	int err;

	(void)ll_sync_create_cancel(&rx);

	for (handle = 0U; handle < CONFIG_BT_PER_ADV_SYNC_MAX; handle++) {
		(void)ll_sync_terminate(handle);
	}

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

struct ll_sync_set *ull_sync_is_valid_get(struct ll_sync_set *sync)
{
	if (((uint8_t *)sync < (uint8_t *)ll_sync_pool) ||
	    ((uint8_t *)sync > ((uint8_t *)ll_sync_pool +
	     (sizeof(struct ll_sync_set) * (CONFIG_BT_PER_ADV_SYNC_MAX - 1))))) {
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
	return ull_sync_handle_get(HDR_LLL2ULL(lll));
}

void ull_sync_release(struct ll_sync_set *sync)
{
	/* Mark the sync context as sync create cancelled */
	if (IS_ENABLED(CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC)) {
		sync->timeout = 0U;
	}

	/* Mark sync context not sync established */
	sync->timeout_reload = 0U;

	mem_release(sync, &sync_free);
}

void ull_sync_setup_addr_check(struct ll_scan_set *scan, uint8_t addr_type,
			       uint8_t *addr, uint8_t rl_idx)
{
	/* Check if Periodic Advertiser list to be used */
	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST) &&
	    scan->per_scan.filter_policy) {
		/* Check in Periodic Advertiser List */
		if (ull_filter_ull_pal_addr_match(addr_type, addr)) {
			/* Remember the address, to check with
			 * SID in Sync Info
			 */
			scan->per_scan.adv_addr_type = addr_type;
			(void)memcpy(scan->per_scan.adv_addr, addr,
				     BDADDR_SIZE);

			/* Address matched */
			scan->per_scan.state = LL_SYNC_STATE_ADDR_MATCH;

		/* Check in Resolving List */
		} else if (IS_ENABLED(CONFIG_BT_CTLR_PRIVACY) &&
			   ull_filter_ull_pal_listed(rl_idx, &addr_type,
						     scan->per_scan.adv_addr)) {
			/* Remember the address, to check with the
			 * SID in Sync Info
			 */
			scan->per_scan.adv_addr_type = addr_type;

			/* Address matched */
			scan->per_scan.state = LL_SYNC_STATE_ADDR_MATCH;
		}

	/* Check with explicitly supplied address */
	} else if ((addr_type == scan->per_scan.adv_addr_type) &&
		   !memcmp(addr, scan->per_scan.adv_addr, BDADDR_SIZE)) {
		/* Address matched */
		scan->per_scan.state = LL_SYNC_STATE_ADDR_MATCH;
	}
}

bool ull_sync_setup_sid_match(struct ll_scan_set *scan, uint8_t sid)
{
	return (scan->per_scan.state == LL_SYNC_STATE_ADDR_MATCH) &&
		((IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST) &&
		  scan->per_scan.filter_policy &&
		  ull_filter_ull_pal_match(scan->per_scan.adv_addr_type,
					   scan->per_scan.adv_addr, sid)) ||
		 (!scan->per_scan.filter_policy &&
		  (sid == scan->per_scan.sid)));
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
	uint8_t *data_chan_map;
	struct lll_sync *lll;
	uint16_t sync_handle;
	uint32_t interval_us;
	struct pdu_adv *pdu;
	uint16_t interval;
	uint8_t chm_last;
	uint32_t ret;
	uint8_t sca;

	/* Populate the LLL context */
	sync = scan->per_scan.sync;
	lll = &sync->lll;

	/* Copy channel map from sca_chm field in sync_info structure, and
	 * clear the SCA bits.
	 */
	chm_last = lll->chm_first;
	lll->chm_last = chm_last;
	data_chan_map = lll->chm[chm_last].data_chan_map;
	(void)memcpy(data_chan_map, si->sca_chm,
		     sizeof(lll->chm[chm_last].data_chan_map));
	data_chan_map[PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET] &=
		~PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK;
	lll->chm[chm_last].data_chan_count =
		util_ones_count_get(data_chan_map,
				    sizeof(lll->chm[chm_last].data_chan_map));
	if (lll->chm[chm_last].data_chan_count < CHM_USED_COUNT_MIN) {
		/* Ignore sync setup, invalid available channel count */
		return;
	}

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC) || \
	defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
	/* Remember the peer address.
	 * NOTE: Peer identity address is copied here when privacy is enable.
	 */
	sync->peer_id_addr_type = scan->per_scan.adv_addr_type;
	(void)memcpy(sync->peer_id_addr, scan->per_scan.adv_addr,
		     sizeof(sync->peer_id_addr));
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC ||
	* CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT
	*/

	memcpy(lll->access_addr, &si->aa, sizeof(lll->access_addr));
	lll->data_chan_id = lll_chan_id(lll->access_addr);
	memcpy(lll->crc_init, si->crc_init, sizeof(lll->crc_init));
	lll->event_counter = si->evt_cntr;
	lll->phy = aux->lll.phy;

	interval = sys_le16_to_cpu(si->interval);
	interval_us = interval * CONN_INT_UNIT_US;

	/* Convert fromm 10ms units to interval units */
	sync->timeout_reload = RADIO_SYNC_EVENTS((sync->timeout * 10U *
						  USEC_PER_MSEC), interval_us);

	sync->sync_expire = CONN_ESTAB_COUNTDOWN;

	/* Extract the SCA value from the sca_chm field of the sync_info
	 * structure.
	 */
	sca = (si->sca_chm[PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET] &
	       PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK) >>
	      PDU_SYNC_INFO_SCA_CHM_SCA_BIT_POS;

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	lll->sca = sca;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

	lll->window_widening_periodic_us =
		ceiling_fraction(((lll_clock_ppm_local_get() +
				   lll_clock_ppm_get(sca)) *
				  interval_us), USEC_PER_SEC);
	lll->window_widening_max_us = (interval_us >> 1) - EVENT_IFS_US;
	if (si->offs_units) {
		lll->window_size_event_us = OFFS_UNIT_300_US;
	} else {
		lll->window_size_event_us = OFFS_UNIT_30_US;
	}

	/* Set the state to sync create */
	scan->per_scan.state = LL_SYNC_STATE_CREATED;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		struct ll_scan_set *scan_1m;

		scan_1m = ull_scan_set_get(SCAN_HANDLE_1M);
		if (scan == scan_1m) {
			struct ll_scan_set *scan_coded;

			scan_coded = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
			scan_coded->per_scan.state = LL_SYNC_STATE_CREATED;
		} else {
			scan_1m->per_scan.state = LL_SYNC_STATE_CREATED;
		}
	}

	sync_handle = ull_sync_handle_get(sync);

	/* Prepare sync notification, dispatch only on successful AUX_SYNC_IND
	 * reception.
	 */
	rx = (void *)sync->node_rx_sync_estab;
	rx->hdr.type = NODE_RX_TYPE_SYNC;
	rx->hdr.handle = sync_handle;
	rx->hdr.rx_ftr.param = scan;
	se = (void *)rx->pdu;
	se->interval = interval;
	se->phy = lll->phy;
	se->sca = sca;

	/* Calculate offset and schedule sync radio events */
	ftr = &node_rx->rx_ftr;
	pdu = (void *)((struct node_rx_pdu *)node_rx)->pdu;

	ready_delay_us = lll_radio_rx_ready_delay_get(lll->phy, 1);

	sync_offset_us = ftr->radio_end_us;
	sync_offset_us += (uint32_t)sys_le16_to_cpu(si->offs) *
			  lll->window_size_event_us;
	/* offs_adjust may be 1 only if sync setup by LL_PERIODIC_SYNC_IND */
	sync_offset_us += (si->offs_adjust ? OFFS_ADJUST_US : 0U);
	sync_offset_us -= PDU_AC_US(pdu->len, lll->phy, ftr->phy_flags);
	sync_offset_us -= EVENT_TICKER_RES_MARGIN_US;
	sync_offset_us -= EVENT_JITTER_US;
	sync_offset_us -= ready_delay_us;

	interval_us -= lll->window_widening_periodic_us;

	/* TODO: active_to_start feature port */
	sync->ull.ticks_active_to_start = 0U;
	sync->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	sync->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	sync->ull.ticks_slot = HAL_TICKER_US_TO_TICKS(
			EVENT_OVERHEAD_START_US + ready_delay_us +
			PDU_AC_MAX_US(PDU_AC_EXT_PAYLOAD_SIZE_MAX, lll->phy) +
			EVENT_OVERHEAD_END_US);

	ticks_slot_offset = MAX(sync->ull.ticks_active_to_start,
				sync->ull.ticks_prepare_to_start);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}
	ticks_slot_offset += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	mfy_lll_prepare.fp = lll_sync_create_prepare;

	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			   (TICKER_ID_SCAN_SYNC_BASE + sync_handle),
			   ftr->ticks_anchor - ticks_slot_offset,
			   HAL_TICKER_US_TO_TICKS(sync_offset_us),
			   HAL_TICKER_US_TO_TICKS(interval_us),
			   HAL_TICKER_REMAINDER(interval_us),
			   TICKER_NULL_LAZY,
			   (sync->ull.ticks_slot + ticks_slot_overhead),
			   ticker_cb, sync,
			   ticker_start_op_cb, (void *)__LINE__);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

void ull_sync_setup_complete(struct ll_scan_set *scan)
{
	/* Remove the sync context from being associated with scan contexts */
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
}

void ull_sync_established_report(memq_link_t *link, struct node_rx_hdr *rx)
{
	struct node_rx_pdu *rx_establ;
	struct ll_sync_set *ull_sync;
	struct node_rx_ftr *ftr;
	struct node_rx_sync *se;
	struct lll_sync *lll;

	ftr = &rx->rx_ftr;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING)
	enum sync_status sync_status;

#if defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
	sync_status = ftr->sync_status;
#else
	struct pdu_cte_info *rx_cte_info;

	lll = ftr->param;

	rx_cte_info = pdu_cte_info_get((struct pdu_adv *)((struct node_rx_pdu *)rx)->pdu);
	if (rx_cte_info != NULL) {
		sync_status = lll_sync_cte_is_allowed(lll->cte_type, lll->filter_policy,
						      rx_cte_info->time, rx_cte_info->type);
	} else {
		sync_status = lll_sync_cte_is_allowed(lll->cte_type, lll->filter_policy, 0,
						      BT_HCI_LE_NO_CTE);
	}
#endif /* CONFIG_BT_CTLR_CTEINLINE_SUPPORT */

	/* Send periodic advertisement sync established report when sync has correct CTE type
	 * or the CTE type is incorrect and filter policy doesn't allow to continue scanning.
	 */
	if (sync_status != SYNC_STAT_READY_OR_CONT_SCAN) {
#else
	if (1) {
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING */

		/* Set the sync handle corresponding to the LLL context passed in the node rx
		 * footer field.
		 */
		lll = ftr->param;
		ull_sync = HDR_LLL2ULL(lll);

		/* Prepare and dispatch sync notification */
		rx_establ = (void *)ull_sync->node_rx_sync_estab;
		rx_establ->hdr.type = NODE_RX_TYPE_SYNC;
		se = (void *)rx_establ->pdu;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING)
		se->status = (ftr->sync_status == SYNC_STAT_TERM) ?
					   BT_HCI_ERR_UNSUPP_REMOTE_FEATURE :
					   BT_HCI_ERR_SUCCESS;

#if !defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
		/* Notify done event handler to terminate sync scan if required. */
		ull_sync->sync_term = sync_status == SYNC_STAT_TERM;
#endif /* !CONFIG_BT_CTLR_CTEINLINE_SUPPORT */
#else
		se->status = BT_HCI_ERR_SUCCESS;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING */

		/* NOTE: footer param has already been populated during sync
		 * setup.
		 */

		ll_rx_put(rx_establ->hdr.link, rx_establ);
		ll_rx_sched();
	}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING)
	/* Handle periodic advertising PDU and send periodic advertising scan report when
	 * the sync was found or was established in the past. The report is not send if
	 * scanning is terminated due to wrong CTE type.
	 */
	if (sync_status != SYNC_STAT_TERM) {
#else
	if (1) {
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING */
		/* Switch sync event prepare function to one reposnsible for regular PDUs receive */
		mfy_lll_prepare.fp = lll_sync_prepare;

		/* Change node type to appropriately handle periodic advertising PDU report */
		rx->type = NODE_RX_TYPE_SYNC_REPORT;
		ull_scan_aux_setup(link, rx);
	}
}

void ull_sync_done(struct node_rx_event_done *done)
{
	uint32_t ticks_drift_minus;
	uint32_t ticks_drift_plus;
	struct ll_sync_set *sync;
	uint16_t elapsed_event;
	struct lll_sync *lll;
	uint16_t skip_event;
	uint16_t lazy;
	uint8_t force;

	/* Get reference to ULL context */
	sync = CONTAINER_OF(done->param, struct ll_sync_set, ull);
	lll = &sync->lll;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING)
#if defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
	if (done->extra.sync_term) {
#else
	if (sync->sync_term) {
#endif /* CONFIG_BT_CTLR_CTEINLINE_SUPPORT */
		/* Stop periodic advertising scan ticker */
		sync_ticker_cleanup(sync, NULL);
	} else
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING */
	{
		/* Events elapsed used in timeout checks below */
		skip_event = lll->skip_event;
		elapsed_event = skip_event + 1;

		/* Sync drift compensation and new skip calculation */
		ticks_drift_plus = 0U;
		ticks_drift_minus = 0U;
		if (done->extra.trx_cnt) {
			/* Calculate drift in ticks unit */
			ull_drift_ticks_get(done, &ticks_drift_plus, &ticks_drift_minus);

			/* Enforce skip */
			lll->skip_event = sync->skip;

			/* Reset failed to establish sync countdown */
			sync->sync_expire = 0U;
		}

		/* Reset supervision countdown */
		if (done->extra.crc_valid) {
			sync->timeout_expire = 0U;
		}

		/* check sync failed to establish */
		else if (sync->sync_expire) {
			if (sync->sync_expire > elapsed_event) {
				sync->sync_expire -= elapsed_event;
			} else {
				sync_ticker_cleanup(sync, ticker_stop_sync_expire_op_cb);

				return;
			}
		}

		/* If anchor point not sync-ed, start timeout countdown, and break skip if any */
		else if (!sync->timeout_expire) {
			sync->timeout_expire = sync->timeout_reload;
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
				sync_ticker_cleanup(sync, ticker_stop_sync_lost_op_cb);

				return;
			}
		}

		/* Check if skip needs update */
		lazy = 0U;
		if ((force) || (skip_event != lll->skip_event)) {
			lazy = lll->skip_event + 1U;
		}

		/* Update Sync ticker instance */
		if (ticks_drift_plus || ticks_drift_minus || lazy || force) {
			uint16_t sync_handle = ull_sync_handle_get(sync);
			uint32_t ticker_status;

			/* Call to ticker_update can fail under the race
			 * condition where in the periodic sync role is being
			 * stopped but at the same time it is preempted by
			 * periodic sync event that gets into close state.
			 * Accept failure when periodic sync role is being
			 * stopped.
			 */
			ticker_status =
				ticker_update(TICKER_INSTANCE_ID_CTLR,
					      TICKER_USER_ID_ULL_HIGH,
					      (TICKER_ID_SCAN_SYNC_BASE +
					       sync_handle),
					      ticks_drift_plus,
					      ticks_drift_minus, 0, 0,
					      lazy, force,
					      ticker_update_op_cb, sync);
			LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
				  (ticker_status == TICKER_STATUS_BUSY) ||
				  ((void *)sync == ull_disable_mark_get()));
		}
	}
}

void ull_sync_chm_update(uint8_t sync_handle, uint8_t *acad, uint8_t acad_len)
{
	struct pdu_adv_sync_chm_upd_ind *chm_upd_ind;
	struct ll_sync_set *sync;
	struct lll_sync *lll;
	uint8_t chm_last;
	uint16_t ad_len;

	/* Get reference to LLL context */
	sync = ull_sync_set_get(sync_handle);
	LL_ASSERT(sync);
	lll = &sync->lll;

	/* Ignore if already in progress */
	if (lll->chm_last != lll->chm_first) {
		return;
	}

	/* Find the Channel Map Update Indication */
	do {
		/* Pick the length and find the Channel Map Update Indication */
		ad_len = acad[PDU_ADV_DATA_HEADER_LEN_OFFSET];
		if (ad_len &&
		    (acad[PDU_ADV_DATA_HEADER_TYPE_OFFSET] ==
		     BT_DATA_CHANNEL_MAP_UPDATE_IND)) {
			break;
		}

		/* Add length field size */
		ad_len += 1U;
		if (ad_len < acad_len) {
			acad_len -= ad_len;
		} else {
			return;
		}

		/* Move to next AD data */
		acad += ad_len;
	} while (acad_len);

	/* Validate the size of the Channel Map Update Indication */
	if (ad_len != (sizeof(*chm_upd_ind) + 1U)) {
		return;
	}

	/* Pick the parameters into the procedure context */
	chm_last = lll->chm_last + 1U;
	if (chm_last == DOUBLE_BUFFER_SIZE) {
		chm_last = 0U;
	}

	chm_upd_ind = (void *)&acad[PDU_ADV_DATA_HEADER_DATA_OFFSET];
	(void)memcpy(lll->chm[chm_last].data_chan_map, chm_upd_ind->chm,
		     sizeof(lll->chm[chm_last].data_chan_map));
	lll->chm[chm_last].data_chan_count =
		util_ones_count_get(lll->chm[chm_last].data_chan_map,
				    sizeof(lll->chm[chm_last].data_chan_map));
	if (lll->chm[chm_last].data_chan_count < CHM_USED_COUNT_MIN) {
		/* Ignore channel map, invalid available channel count */
		return;
	}

	lll->chm_instant = sys_le16_to_cpu(chm_upd_ind->instant);

	/* Set Channel Map Update Procedure in progress */
	lll->chm_last = chm_last;
}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
/* @brief Function updates periodic sync slot duration.
 *
 * @param[in] sync              Pointer to sync instance
 * @param[in] slot_plus_us      Number of microsecond to add to ticker slot
 * @param[in] slot_minus_us     Number of microsecond to subtracks from ticker slot
 *
 * @retval 0            Successful ticker slot update.
 * @retval -ENOENT      Ticker node related with provided sync is already stopped.
 * @retval -ENOMEM      Couldn't enqueue update ticker job.
 * @retval -EFAULT      Somethin else went wrong.
 */
int ull_sync_slot_update(struct ll_sync_set *sync, uint32_t slot_plus_us,
			 uint32_t slot_minus_us)
{
	uint32_t ret;
	uint32_t ret_cb;

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_update(TICKER_INSTANCE_ID_CTLR,
			    TICKER_USER_ID_THREAD,
			    (TICKER_ID_SCAN_SYNC_BASE +
			    ull_sync_handle_get(sync)),
			    0, 0,
			    slot_plus_us,
			    slot_minus_us,
			    0, 0,
			    ticker_update_op_status_give,
			    (void *)&ret_cb);
	if (ret == TICKER_STATUS_BUSY || ret == TICKER_STATUS_SUCCESS) {
		/* Wait for callback or clear semaphore is callback was already
		 * executed.
		 */
		k_sem_take(&sem_ticker_cb, K_FOREVER);

		if (ret_cb == TICKER_STATUS_FAILURE) {
			return -EFAULT; /* Something went wrong */
		} else {
			return 0;
		}
	} else {
		if (ret_cb != TICKER_STATUS_BUSY) {
			/* Ticker callback was executed and job enqueue was successful.
			 * Call k_sem_take to clear ticker callback semaphore.
			 */
			k_sem_take(&sem_ticker_cb, K_FOREVER);
		}
		/* Ticker was already stopped or job was not enqueued. */
		return (ret_cb == TICKER_STATUS_FAILURE) ? -ENOENT : -ENOMEM;
	}
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

static int init_reset(void)
{
	/* Initialize sync pool. */
	mem_init(ll_sync_pool, sizeof(struct ll_sync_set),
		 sizeof(ll_sync_pool) / sizeof(struct ll_sync_set),
		 &sync_free);

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	k_sem_init(&sem_ticker_cb, 0, 1);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

	return 0;
}

static inline struct ll_sync_set *sync_acquire(void)
{
	return mem_acquire(&sync_free);
}

static void sync_ticker_cleanup(struct ll_sync_set *sync, ticker_op_func stop_op_cb)
{
	uint16_t sync_handle = ull_sync_handle_get(sync);
	uint32_t ret;

	/* Stop Periodic Sync Ticker */
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			  TICKER_ID_SCAN_SYNC_BASE + sync_handle, stop_op_cb, (void *)sync);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
{
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
	p.force = force;
	p.param = lll;
	mfy_lll_prepare.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL, 0, &mfy_lll_prepare);
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

static void ticker_stop_sync_expire_op_cb(uint32_t status, void *param)
{
	uint32_t retval;
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, sync_expire};

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	mfy.param = param;

	retval = mayfly_enqueue(TICKER_USER_ID_ULL_LOW, TICKER_USER_ID_ULL_HIGH,
				0, &mfy);
	LL_ASSERT(!retval);
}

static void sync_expire(void *param)
{
	struct ll_sync_set *sync = param;
	struct node_rx_sync *se;
	struct node_rx_pdu *rx;

	/* Generate Periodic advertising sync failed to establish */
	rx = (void *)sync->node_rx_sync_estab;
	rx->hdr.type = NODE_RX_TYPE_SYNC;
	rx->hdr.handle = LLL_HANDLE_INVALID;

	/* NOTE: struct node_rx_sync_estab has uint8_t member following the
	 *       struct node_rx_hdr to store the reason.
	 */
	se = (void *)rx->pdu;
	se->status = BT_HCI_ERR_CONN_FAIL_TO_ESTAB;

	/* NOTE: footer param has already been populated during sync setup */

	/* Enqueue the sync failed to established towards ULL context */
	ll_rx_put(rx->hdr.link, rx);
	ll_rx_sched();
}

static void ticker_stop_sync_lost_op_cb(uint32_t status, void *param)
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

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC)
static struct ll_sync_set *sync_is_create_get(uint16_t handle)
{
	struct ll_sync_set *sync;

	sync = ull_sync_set_get(handle);
	if (!sync || !sync->timeout) {
		return NULL;
	}

	return sync;
}

static bool peer_sid_sync_exists(uint8_t const peer_id_addr_type,
				 uint8_t const *const peer_id_addr,
				 uint8_t sid)
{
	uint16_t handle;

	for (handle = 0U; handle < CONFIG_BT_PER_ADV_SYNC_MAX; handle++) {
		struct ll_sync_set *sync = sync_is_create_get(handle);

		if (sync &&
		    (sync->peer_id_addr_type == peer_id_addr_type) &&
		    !memcmp(sync->peer_id_addr, peer_id_addr, BDADDR_SIZE) &&
		    (sync->sid == sid)) {
			return true;
		}
	}

	return false;
}
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_SYNC */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
static void ticker_update_op_status_give(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;

	k_sem_give(&sem_ticker_cb);
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING) && \
	!defined(CONFIG_BT_CTLR_CTEINLINE_SUPPORT)
static struct pdu_cte_info *pdu_cte_info_get(struct pdu_adv *pdu)
{
	struct pdu_adv_com_ext_adv *com_hdr;
	struct pdu_adv_ext_hdr *hdr;
	uint8_t *dptr;

	com_hdr = &pdu->adv_ext_ind;
	hdr = &com_hdr->ext_hdr;

	if (!com_hdr->ext_hdr_len || (com_hdr->ext_hdr_len != 0 && !hdr->cte_info)) {
		return NULL;
	}

	/* Skip flags in extended advertising header */
	dptr = hdr->data;

	/* Make sure there are no fields that are not allowd for AUX_SYNC_IND and AUX_CHAIN_IND */
	LL_ASSERT(!hdr->adv_addr);
	LL_ASSERT(!hdr->tgt_addr);

	return (struct pdu_cte_info *)hdr->data;
}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING && !CONFIG_BT_CTLR_CTEINLINE_SUPPORT */
