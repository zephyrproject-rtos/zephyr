/*
 * Copyright (c) 2016-2019 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <bluetooth/hci.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ll.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_adv.h"
#include "lll_scan.h"
#include "lll_conn.h"
#include "lll_filter.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_filter.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_scan_internal.h"
#include "ull_sched_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_scan
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder, uint16_t lazy,
		      void *param);
static uint8_t disable(uint8_t handle);

static struct ll_scan_set ll_scan[BT_CTLR_SCAN_SET];

uint8_t ll_scan_params_set(uint8_t type, uint16_t interval, uint16_t window,
			uint8_t own_addr_type, uint8_t filter_policy)
{
	struct ll_scan_set *scan;
	struct lll_scan *lll;

	scan = ull_scan_is_disabled_get(SCAN_HANDLE_1M);
	if (!scan) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	uint8_t phy;

	phy  = type >> 1;
	if (phy & BT_HCI_LE_EXT_SCAN_PHY_CODED) {
		struct ll_scan_set *scan_coded;

		if (!IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		scan_coded = ull_scan_is_disabled_get(SCAN_HANDLE_PHY_CODED);
		if (!scan_coded) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		scan = scan_coded;
	}

	lll = &scan->lll;

	if (!interval) {
		lll->phy = 0U;

		return 0;
	}

	lll->phy = phy;

#else /* !CONFIG_BT_CTLR_ADV_EXT */
	lll = &scan->lll;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	scan->own_addr_type = own_addr_type;

	ull_scan_params_set(lll, type, interval, window, filter_policy);

	return 0;
}

uint8_t ll_scan_enable(uint8_t enable)
{
	struct ll_scan_set *scan_coded = NULL;
	struct ll_scan_set *scan;
	uint8_t own_addr_type = 0U;
	uint8_t is_coded_phy = 0U;
	uint8_t err;

	if (!enable) {
		err = disable(SCAN_HANDLE_1M);

		if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
		    IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
			uint8_t err_coded;

			err_coded = disable(SCAN_HANDLE_PHY_CODED);
			if (!err_coded) {
				err = 0U;
			}
		}

		return err;
	}

	scan = ull_scan_is_disabled_get(SCAN_HANDLE_1M);
	if (!scan) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_PHY_CODED)
	scan_coded = ull_scan_is_disabled_get(SCAN_HANDLE_PHY_CODED);
	if (!scan_coded) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	own_addr_type = scan_coded->own_addr_type;
	is_coded_phy = (scan_coded->lll.phy &
			BT_HCI_LE_EXT_SCAN_PHY_CODED);
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_PHY_CODED */

	if ((IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) && is_coded_phy &&
	     (own_addr_type & 0x1)) ||
	    (scan->own_addr_type & 0x1)) {
		if (!mem_nz(ll_addr_get(1, NULL), BDADDR_SIZE)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	struct lll_scan *lll;

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) && is_coded_phy) {
		lll = &scan_coded->lll;

		/* TODO: Privacy support in Advertising Extensions */
	} else {
		lll = &scan->lll;
		own_addr_type = scan->own_addr_type;
	}

	ull_filter_scan_update(lll->filter_policy);

	lll->rl_idx = FILTER_IDX_NONE;
	lll->rpa_gen = 0;

	if ((lll->type & 0x1) &&
	    (own_addr_type == BT_ADDR_LE_PUBLIC_ID ||
	     own_addr_type == BT_ADDR_LE_RANDOM_ID)) {
		/* Generate RPAs if required */
		ull_filter_rpa_update(false);
		lll->rpa_gen = 1;
	}
#endif

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_PHY_CODED)
	if (!is_coded_phy || (scan->lll.phy & BT_HCI_LE_EXT_SCAN_PHY_1M))
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_PHY_CODED */
	{
		err = ull_scan_enable(scan);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
	    is_coded_phy) {
		err = ull_scan_enable(scan_coded);
		if (err) {
			return err;
		}
	}

	return 0;
}

int ull_scan_init(void)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)) {
		err = ull_scan_aux_init();
		if (err) {
			return err;
		}
	}

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_scan_reset(void)
{
	uint8_t handle;
	int err;

	for (handle = 0U; handle < BT_CTLR_SCAN_SET; handle++) {
		(void)disable(handle);
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)) {
		err = ull_scan_aux_reset();
		if (err) {
			return err;
		}
	}

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

void ull_scan_params_set(struct lll_scan *lll, uint8_t type, uint16_t interval,
			 uint16_t window, uint8_t filter_policy)
{
	/* type value:
	 * 0000b - legacy 1M passive
	 * 0001b - legacy 1M active
	 * 0010b - Ext. 1M passive
	 * 0011b - Ext. 1M active
	 * 0100b - invalid
	 * 0101b - invalid
	 * 0110b - invalid
	 * 0111b - invalid
	 * 1000b - Ext. Coded passive
	 * 1001b - Ext. Coded active
	 */
	lll->type = type;
	lll->filter_policy = filter_policy;
	lll->interval = interval;
	lll->ticks_window = HAL_TICKER_US_TO_TICKS((uint64_t)window * 625U);
}

uint8_t ull_scan_enable(struct ll_scan_set *scan)
{
	struct lll_scan *lll = &scan->lll;
	uint32_t ticks_slot_overhead;
	uint32_t volatile ret_cb;
	uint32_t ticks_interval;
	uint32_t ticks_anchor;
	uint32_t ret;

	lll->chan = 0;
	lll->init_addr_type = scan->own_addr_type;
	ll_addr_get(lll->init_addr_type, lll->init_addr);

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	lll->tx_pwr_lvl = RADIO_TXP_DEFAULT;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

	ull_hdr_init(&scan->ull);
	lll_hdr_init(lll, scan);

	ticks_interval = HAL_TICKER_US_TO_TICKS((uint64_t)lll->interval * 625U);

	/* TODO: active_to_start feature port */
	scan->evt.ticks_active_to_start = 0U;
	scan->evt.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	scan->evt.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	if ((lll->ticks_window +
	     HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US)) <
	    (ticks_interval -
	     HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US))) {
		scan->evt.ticks_slot =
			(lll->ticks_window +
			 HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US));
	} else {
		scan->evt.ticks_slot =
			(ticks_interval -
			 HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US));
		lll->ticks_window = 0;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(scan->evt.ticks_active_to_start,
					  scan->evt.ticks_xtal_to_start);
	} else {
		ticks_slot_overhead = 0U;
	}

	ticks_anchor = ticker_ticks_now_get();

#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	if (!lll->conn) {
		uint32_t ticks_ref = 0U;
		uint32_t offset_us = 0U;

		ull_sched_after_mstr_slot_get(TICKER_USER_ID_THREAD,
					      (scan->evt.ticks_slot +
					       ticks_slot_overhead),
					      &ticks_ref, &offset_us);

		/* Use the ticks_ref as scanner's anchor if a free time space
		 * after any master role is available (indicated by a non-zero
		 * offset_us value).
		 */
		if (offset_us) {
			ticks_anchor = ticks_ref +
				       HAL_TICKER_US_TO_TICKS(offset_us);
		}
	}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_CTLR_SCHED_ADVANCED */

	uint8_t handle = ull_scan_handle_get(scan);

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR,
			   TICKER_USER_ID_THREAD, TICKER_ID_SCAN_BASE + handle,
			   ticks_anchor, 0, ticks_interval,
			   HAL_TICKER_REMAINDER((uint64_t)lll->interval * 625U),
			   TICKER_NULL_LAZY,
			   (scan->evt.ticks_slot + ticks_slot_overhead),
			   ticker_cb, scan,
			   ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret != TICKER_STATUS_SUCCESS) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	scan->is_enabled = 1U;

#if defined(CONFIG_BT_CTLR_PRIVACY)
#if defined(CONFIG_BT_BROADCASTER)
	if (!ull_adv_is_enabled_get(0))
#endif
	{
		ull_filter_adv_scan_state_cb(BIT(1));
	}
#endif

	return 0;
}

uint8_t ull_scan_disable(uint8_t handle, struct ll_scan_set *scan)
{
	uint32_t volatile ret_cb;
	void *mark;
	uint32_t ret;

	mark = ull_disable_mark(scan);
	LL_ASSERT(mark == scan);

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			  TICKER_ID_SCAN_BASE + handle,
			  ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret) {
		mark = ull_disable_unmark(scan);
		LL_ASSERT(mark == scan);

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ret = ull_disable(&scan->lll);
	LL_ASSERT(!ret);

	mark = ull_disable_unmark(scan);
	LL_ASSERT(mark == scan);

	return 0;
}

struct ll_scan_set *ull_scan_set_get(uint8_t handle)
{
	if (handle >= BT_CTLR_SCAN_SET) {
		return NULL;
	}

	return &ll_scan[handle];
}

uint8_t ull_scan_handle_get(struct ll_scan_set *scan)
{
	return ((uint8_t *)scan - (uint8_t *)ll_scan) / sizeof(*scan);
}

uint8_t ull_scan_lll_handle_get(struct lll_scan *lll)
{
	return ull_scan_handle_get((void *)lll->hdr.parent);
}

struct ll_scan_set *ull_scan_is_enabled_get(uint8_t handle)
{
	struct ll_scan_set *scan;

	scan = ull_scan_set_get(handle);
	if (!scan || !scan->is_enabled) {
		return NULL;
	}

	return scan;
}

struct ll_scan_set *ull_scan_is_disabled_get(uint8_t handle)
{
	struct ll_scan_set *scan;

	scan = ull_scan_set_get(handle);
	if (!scan || scan->is_enabled) {
		return NULL;
	}

	return scan;
}

uint32_t ull_scan_is_enabled(uint8_t handle)
{
	struct ll_scan_set *scan;

	scan = ull_scan_is_enabled_get(handle);
	if (!scan) {
		return 0;
	}

	/* NOTE: BIT(0) - passive scanning enabled
	 *       BIT(1) - active scanning enabled
	 *       BIT(2) - initiator enabled
	 */
	return (((uint32_t)scan->is_enabled << scan->lll.type) |
#if defined(CONFIG_BT_CENTRAL)
		(scan->lll.conn ? BIT(2) : 0) |
#endif
		0);
}

uint32_t ull_scan_filter_pol_get(uint8_t handle)
{
	struct ll_scan_set *scan;

	scan = ull_scan_is_enabled_get(handle);
	if (!scan) {
		return 0;
	}

	return scan->lll.filter_policy;
}

static int init_reset(void)
{
	return 0;
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder, uint16_t lazy,
		      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_scan_prepare};
	static struct lll_prepare_param p;
	struct ll_scan_set *scan = param;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_O(1);

	/* Increment prepare reference count */
	ref = ull_ref_inc(&scan->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.param = &scan->lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!ret);

	DEBUG_RADIO_PREPARE_O(1);
}

static uint8_t disable(uint8_t handle)
{
	struct ll_scan_set *scan;
	uint8_t ret;

	scan = ull_scan_is_enabled_get(handle);
	if (!scan) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

#if defined(CONFIG_BT_CENTRAL)
	if (scan->lll.conn) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif

	ret = ull_scan_disable(handle, scan);
	if (ret) {
		return ret;
	}

	scan->is_enabled = 0U;

#if defined(CONFIG_BT_CTLR_PRIVACY)
#if defined(CONFIG_BT_BROADCASTER)
	if (!ull_adv_is_enabled_get(0))
#endif
	{
		ull_filter_adv_scan_state_cb(0);
	}
#endif

	return 0;
}
