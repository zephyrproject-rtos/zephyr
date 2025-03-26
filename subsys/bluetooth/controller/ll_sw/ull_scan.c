/*
 * Copyright (c) 2016-2019 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/bluetooth/hci_types.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_filter.h"

#include "ll_sw/ull_tx_queue.h"

#include "ull_adv_types.h"
#include "ull_filter.h"

#include "ull_conn_types.h"
#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_scan_types.h"
#include "ull_scan_internal.h"
#include "ull_sched_internal.h"

#include "ll.h"

#include "hal/debug.h"

static int init_reset(void);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);
static uint8_t disable(uint8_t handle);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#define IS_PHY_ENABLED(scan_ctx, scan_phy) ((scan_ctx)->lll.phy & (scan_phy))

static uint8_t is_scan_update(uint8_t handle, uint16_t duration,
			      uint16_t period, struct ll_scan_set **scan,
			      struct node_rx_pdu **node_rx_scan_term);
static uint8_t duration_period_setup(struct ll_scan_set *scan,
				     uint16_t duration, uint16_t period,
				     struct node_rx_pdu **node_rx_scan_term);
static uint8_t duration_period_update(struct ll_scan_set *scan,
				      uint8_t is_update);
static void ticker_stop_ext_op_cb(uint32_t status, void *param);
static void ext_disable(void *param);
static void ext_disabled_cb(void *param);
#endif /* CONFIG_BT_CTLR_ADV_EXT */

static struct ll_scan_set ll_scan[BT_CTLR_SCAN_SET];

#if defined(CONFIG_BT_TICKER_EXT)
static struct ticker_ext ll_scan_ticker_ext[BT_CTLR_SCAN_SET];
#endif /* CONFIG_BT_TICKER_EXT */

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

	/* NOTE: Pass invalid interval value to not start scanning using this
	 *       scan instance.
	 */
	if (!interval) {
		/* Set PHY to 0 to not start scanning on this instance */
		lll->phy = 0U;

		return 0;
	}

	/* If phy assigned is PHY_1M or PHY_CODED, then scanning on that
	 * PHY is enabled.
	 */
	lll->phy = phy;

#else /* !CONFIG_BT_CTLR_ADV_EXT */
	lll = &scan->lll;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	scan->own_addr_type = own_addr_type;

	scan->ticks_window = ull_scan_params_set(lll, type, interval, window,
						 filter_policy);

	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
uint8_t ll_scan_enable(uint8_t enable, uint16_t duration, uint16_t period)
{
	struct node_rx_pdu *node_rx_scan_term = NULL;
	uint8_t is_update_coded = 0U;
	uint8_t is_update_1m = 0U;
#else /* !CONFIG_BT_CTLR_ADV_EXT */
uint8_t ll_scan_enable(uint8_t enable)
{
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
	struct ll_scan_set *scan_coded = NULL;
	uint8_t own_addr_type = 0U;
	uint8_t is_coded_phy = 0U;
	struct ll_scan_set *scan;
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
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		is_update_1m = is_scan_update(SCAN_HANDLE_1M, duration, period,
					      &scan, &node_rx_scan_term);
		if (!is_update_1m)
#endif /* CONFIG_BT_CTLR_ADV_EXT */
		{
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_PHY_CODED)
	scan_coded = ull_scan_is_disabled_get(SCAN_HANDLE_PHY_CODED);
	if (!scan_coded) {
		is_update_coded = is_scan_update(SCAN_HANDLE_PHY_CODED,
						 duration, period, &scan_coded,
						 &node_rx_scan_term);
		if (!is_update_coded) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}
	}

	own_addr_type = scan_coded->own_addr_type;
	is_coded_phy = (scan_coded->lll.phy &
			BT_HCI_LE_EXT_SCAN_PHY_CODED);
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_PHY_CODED */

	if ((is_coded_phy && (own_addr_type & 0x1)) ||
	    (!is_coded_phy && (scan->own_addr_type & 0x1))) {
		if (!mem_nz(ll_addr_get(BT_ADDR_LE_RANDOM), BDADDR_SIZE)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	if (!is_coded_phy || IS_PHY_ENABLED(scan, PHY_1M))
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	{
		err = duration_period_setup(scan, duration, period,
					    &node_rx_scan_term);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED) &&
	    is_coded_phy) {
		err = duration_period_setup(scan_coded, duration, period,
					    &node_rx_scan_term);
		if (err) {
			return err;
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if (defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_JIT_SCHEDULING)) || \
	defined(CONFIG_BT_CTLR_PRIVACY)
	struct lll_scan *lll;

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) && is_coded_phy) {
		lll = &scan_coded->lll;

		/* TODO: Privacy support in Advertising Extensions */
	} else {
		lll = &scan->lll;
		own_addr_type = scan->own_addr_type;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	ull_filter_scan_update(lll->filter_policy);

	lll->rl_idx = FILTER_IDX_NONE;
	lll->rpa_gen = 0;

	if ((lll->type & 0x1) && (own_addr_type == BT_HCI_OWN_ADDR_RPA_OR_PUBLIC ||
				  own_addr_type == BT_HCI_OWN_ADDR_RPA_OR_RANDOM)) {
		/* Generate RPAs if required */
		ull_filter_rpa_update(false);
		lll->rpa_gen = 1;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	lll->scan_aux_score = 0;
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_JIT_SCHEDULING */
#endif /* (CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_JIT_SCHEDULING) || CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	if (!is_coded_phy || IS_PHY_ENABLED(scan, PHY_1M))
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	{
		err = duration_period_update(scan, is_update_1m);
		if (err) {
			return err;
		} else if (is_update_1m) {
			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED) &&
	    is_coded_phy) {
		err = duration_period_update(scan_coded, is_update_coded);
		if (err) {
			return err;
		} else if (is_update_coded) {
			return 0;
		}
	}

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	if (!is_coded_phy || IS_PHY_ENABLED(scan, PHY_1M))
#endif /* CONFIG_BT_CTLR_PHY_CODED */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
	{
		err = ull_scan_enable(scan);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
	    IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED) &&
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

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		/* Initialize PHY value to 0 to not start scanning on the scan
		 * instance if an explicit ll_scan_params_set() has not been
		 * invoked from HCI to enable scanning on that PHY.
		 */
		ll_scan[handle].lll.phy = 0U;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
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

uint32_t ull_scan_params_set(struct lll_scan *lll, uint8_t type,
			     uint16_t interval, uint16_t window,
			     uint8_t filter_policy)
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
	lll->ticks_window = HAL_TICKER_US_TO_TICKS((uint64_t)window *
						   SCAN_INT_UNIT_US);

	return lll->ticks_window;
}

uint8_t ull_scan_enable(struct ll_scan_set *scan)
{
	uint32_t ticks_slot_overhead;
	uint32_t volatile ret_cb;
	uint32_t ticks_interval;
	uint32_t ticks_anchor;
	uint32_t ticks_offset;
	struct lll_scan *lll;
	uint8_t handle;
	uint32_t ret;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* Initialize extend scan stop request */
	scan->is_stop = 0U;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	/* Initialize LLL scan context */
	lll = &scan->lll;
	lll->init_addr_type = scan->own_addr_type;
	(void)ll_addr_read(lll->init_addr_type, lll->init_addr);
	lll->chan = 0U;
	lll->is_stop = 0U;

	ull_hdr_init(&scan->ull);
	lll_hdr_init(lll, scan);

	ticks_interval = HAL_TICKER_US_TO_TICKS((uint64_t)lll->interval *
						SCAN_INT_UNIT_US);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	} else {
		ticks_slot_overhead = 0U;
	}

	handle = ull_scan_handle_get(scan);

	lll->ticks_window = scan->ticks_window;
	if ((lll->ticks_window +
	     HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US)) <
	    (ticks_interval - ticks_slot_overhead)) {
		scan->ull.ticks_slot =
			(lll->ticks_window +
			 HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US));

#if defined(CONFIG_BT_TICKER_EXT)
		ll_scan_ticker_ext[handle].ticks_slot_window =
			scan->ull.ticks_slot + ticks_slot_overhead;
#endif /* CONFIG_BT_TICKER_EXT */

	} else {
		if (IS_ENABLED(CONFIG_BT_CTLR_SCAN_UNRESERVED)) {
			scan->ull.ticks_slot = 0U;
		} else {
			scan->ull.ticks_slot = ticks_interval -
					       ticks_slot_overhead;
		}

		lll->ticks_window = 0U;

#if defined(CONFIG_BT_TICKER_EXT)
		ll_scan_ticker_ext[handle].ticks_slot_window = ticks_interval;
#endif /* CONFIG_BT_TICKER_EXT */
	}

	if (false) {

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_PHY_CODED)
	} else if (handle == SCAN_HANDLE_1M) {
		const struct ll_scan_set *scan_coded;

		scan_coded = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
		if (IS_PHY_ENABLED(scan_coded, PHY_CODED) &&
		    (lll->ticks_window != 0U)) {
			const struct lll_scan *lll_coded;
			uint32_t ticks_interval_coded;
			uint32_t ticks_window_sum_min;
			uint32_t ticks_window_sum_max;

			lll_coded = &scan_coded->lll;
			ticks_interval_coded = HAL_TICKER_US_TO_TICKS(
						(uint64_t)lll_coded->interval *
						SCAN_INT_UNIT_US);
			ticks_window_sum_min = lll->ticks_window +
					       lll_coded->ticks_window;
			ticks_window_sum_max = ticks_window_sum_min +
				HAL_TICKER_US_TO_TICKS(EVENT_TICKER_RES_MARGIN_US << 1);
			/* Check if 1M and Coded PHY scanning use same interval
			 * and the sum of the scan window duration equals their
			 * interval then use continuous scanning and avoid time
			 * reservation from overlapping.
			 */
			if ((ticks_interval == ticks_interval_coded) &&
			    IN_RANGE(ticks_interval, ticks_window_sum_min,
				     ticks_window_sum_max)) {
				if (IS_ENABLED(CONFIG_BT_CTLR_SCAN_UNRESERVED)) {
					scan->ull.ticks_slot = 0U;
				} else {
					scan->ull.ticks_slot =
						lll->ticks_window -
						ticks_slot_overhead -
						HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US) -
						HAL_TICKER_US_TO_TICKS(EVENT_TICKER_RES_MARGIN_US);
				}

				/* Continuous scanning, no scan window stop
				 * ticker to be started but we will zero the
				 * ticks_window value when coded PHY scan is
				 * enabled (the next following else clause).
				 * Due to this the first scan window will have
				 * the stop ticker started but consecutive
				 * scan window will not have the stop ticker
				 * started once coded PHY scan window has been
				 * enabled.
				 */
			}

#if defined(CONFIG_BT_TICKER_EXT)
			ll_scan_ticker_ext[handle].ticks_slot_window = 0U;
#endif /* CONFIG_BT_TICKER_EXT */
		}

		/* 1M scan window starts without any offset */
		ticks_offset = 0U;

	} else if (handle == SCAN_HANDLE_PHY_CODED) {
		struct ll_scan_set *scan_1m;

		scan_1m = ull_scan_set_get(SCAN_HANDLE_1M);
		if (IS_PHY_ENABLED(scan_1m, PHY_1M) &&
		    (lll->ticks_window != 0U)) {
			uint32_t ticks_window_sum_min;
			uint32_t ticks_window_sum_max;
			uint32_t ticks_interval_1m;
			struct lll_scan *lll_1m;

			lll_1m = &scan_1m->lll;
			ticks_interval_1m = HAL_TICKER_US_TO_TICKS(
						(uint64_t)lll_1m->interval *
						SCAN_INT_UNIT_US);
			ticks_window_sum_min = lll->ticks_window +
					       lll_1m->ticks_window;
			ticks_window_sum_max = ticks_window_sum_min +
				HAL_TICKER_US_TO_TICKS(EVENT_TICKER_RES_MARGIN_US << 1);
			/* Check if 1M and Coded PHY scanning use same interval
			 * and the sum of the scan window duration equals their
			 * interval then use continuous scanning and avoid time
			 * reservation from overlapping.
			 */
			if ((ticks_interval == ticks_interval_1m) &&
			    IN_RANGE(ticks_interval, ticks_window_sum_min,
				     ticks_window_sum_max)) {
				if (IS_ENABLED(CONFIG_BT_CTLR_SCAN_UNRESERVED)) {
					scan->ull.ticks_slot = 0U;
				} else {
					scan->ull.ticks_slot =
						lll->ticks_window -
						ticks_slot_overhead -
						HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US) -
						HAL_TICKER_US_TO_TICKS(EVENT_TICKER_RES_MARGIN_US);
				}
				/* Offset the coded PHY scan window, place
				 * after 1M scan window.
				 * Have some margin for jitter due to ticker
				 * resolution.
				 */
				ticks_offset = lll_1m->ticks_window;
				ticks_offset += HAL_TICKER_US_TO_TICKS(
					EVENT_TICKER_RES_MARGIN_US << 1);

				/* Continuous scanning, no scan window stop
				 * ticker started for both 1M and coded PHY.
				 */
				lll->ticks_window = 0U;
				lll_1m->ticks_window = 0U;

			} else {
				ticks_offset = 0U;
			}

#if defined(CONFIG_BT_TICKER_EXT)
			ll_scan_ticker_ext[handle].ticks_slot_window = 0U;
#endif /* CONFIG_BT_TICKER_EXT */
		} else {
			ticks_offset = 0U;
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_PHY_CODED */

	} else {
		ticks_offset = 0U;
	}

	ticks_anchor = ticker_ticks_now_get();
	ticks_anchor += HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_CTLR_SCHED_ADVANCED)
	if (!lll->conn) {
		uint32_t ticks_ref = 0U;
		uint32_t offset_us = 0U;
		int err;

		err = ull_sched_after_cen_slot_get(TICKER_USER_ID_THREAD,
						   (scan->ull.ticks_slot +
						    ticks_slot_overhead),
						   &ticks_ref, &offset_us);

		/* Use the ticks_ref as scanner's anchor if a free time space
		 * after any central role is available (indicated by a non-zero
		 * offset_us value).
		 */
		if (!err) {
			ticks_anchor = ticks_ref +
				       HAL_TICKER_US_TO_TICKS(offset_us);
		}
	}
#endif /* CONFIG_BT_CENTRAL && CONFIG_BT_CTLR_SCHED_ADVANCED */

	ret_cb = TICKER_STATUS_BUSY;

#if defined(CONFIG_BT_TICKER_EXT)
#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	ll_scan_ticker_ext[handle].expire_info_id = TICKER_NULL;
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
	ret = ticker_start_ext(
#else
	ret = ticker_start(
#endif /* CONFIG_BT_TICKER_EXT */
			   TICKER_INSTANCE_ID_CTLR,
			   TICKER_USER_ID_THREAD, TICKER_ID_SCAN_BASE + handle,
			   (ticks_anchor + ticks_offset), 0, ticks_interval,
			   HAL_TICKER_REMAINDER((uint64_t)lll->interval *
						SCAN_INT_UNIT_US),
			   TICKER_NULL_LAZY,
			   (scan->ull.ticks_slot + ticks_slot_overhead),
			   ticker_cb, scan,
			   ull_ticker_status_give, (void *)&ret_cb
#if defined(CONFIG_BT_TICKER_EXT)
			   ,
			   &ll_scan_ticker_ext[handle]
#endif /* CONFIG_BT_TICKER_EXT */
			   );
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
	int err;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* Request Extended Scan stop */
	scan->is_stop = 1U;
	cpu_dmb();
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	err = ull_ticker_stop_with_mark(TICKER_ID_SCAN_BASE + handle,
					scan, &scan->lll);
	LL_ASSERT_INFO2(err == 0 || err == -EALREADY, handle, err);
	if (err) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS)
	/* Stop associated auxiliary scan contexts */
	err = ull_scan_aux_stop(&scan->lll);
	if (err && (err != -EALREADY)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#else /* !CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
	/* Find and stop associated auxiliary scan contexts */
	for (uint8_t aux_handle = 0; aux_handle < CONFIG_BT_CTLR_SCAN_AUX_SET;
	     aux_handle++) {
		struct lll_scan_aux *aux_scan_lll;
		struct ll_scan_set *aux_scan;
		struct ll_scan_aux_set *aux;

		aux = ull_scan_aux_set_get(aux_handle);
		aux_scan_lll = aux->parent;
		if (!aux_scan_lll) {
			continue;
		}

		aux_scan = HDR_LLL2ULL(aux_scan_lll);
		if (aux_scan == scan) {
			void *parent;

			err = ull_scan_aux_stop(aux);
			if (err && (err != -EALREADY)) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}

			/* Use a local variable to assert on auxiliary context's
			 * release.
			 * Under race condition a released aux context can be
			 * allocated for reception of chain PDU of a periodic
			 * sync role.
			 */
			parent = aux->parent;
			LL_ASSERT(!parent || (parent != aux_scan_lll));
		}
	}
#endif /* !CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
void ull_scan_done(struct node_rx_event_done *done)
{
	struct node_rx_pdu *rx;
	struct ll_scan_set *scan;
	struct lll_scan *lll;
	uint8_t handle;
	uint32_t ret;

	/* Get reference to ULL context */
	scan = CONTAINER_OF(done->param, struct ll_scan_set, ull);
	lll = &scan->lll;

	if (likely(scan->duration_lazy || !lll->duration_reload ||
		   lll->duration_expire)) {
		return;
	}

	/* Prevent duplicate terminate event generation */
	lll->duration_reload = 0U;

	handle = ull_scan_handle_get(scan);
	LL_ASSERT(handle < BT_CTLR_SCAN_SET);

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	/* Prevent duplicate terminate event if ull_scan_done get called by
	 * the other scan instance.
	 */
	struct ll_scan_set *scan_other;

	if (handle == SCAN_HANDLE_1M) {
		scan_other = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
	} else {
		scan_other = ull_scan_set_get(SCAN_HANDLE_1M);
	}
	scan_other->lll.duration_reload = 0U;
#endif /* CONFIG_BT_CTLR_PHY_CODED */

	rx = (void *)scan->node_rx_scan_term;
	rx->hdr.type = NODE_RX_TYPE_EXT_SCAN_TERMINATE;
	rx->hdr.handle = handle;

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			  (TICKER_ID_SCAN_BASE + handle), ticker_stop_ext_op_cb,
			  scan);

	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

void ull_scan_term_dequeue(uint8_t handle)
{
	struct ll_scan_set *scan;

	scan = ull_scan_set_get(handle);
	LL_ASSERT(scan);

	scan->is_enabled = 0U;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
	if (handle == SCAN_HANDLE_1M) {
		struct ll_scan_set *scan_coded;

		scan_coded = ull_scan_set_get(SCAN_HANDLE_PHY_CODED);
		if (IS_PHY_ENABLED(scan_coded, PHY_CODED)) {
			uint8_t err;

			err = disable(SCAN_HANDLE_PHY_CODED);
			LL_ASSERT(!err);
		}
	} else {
		struct ll_scan_set *scan_1m;

		scan_1m = ull_scan_set_get(SCAN_HANDLE_1M);
		if (IS_PHY_ENABLED(scan_1m, PHY_1M)) {
			uint8_t err;

			err = disable(SCAN_HANDLE_1M);
			LL_ASSERT(!err);
		}
	}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

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

struct ll_scan_set *ull_scan_is_valid_get(struct ll_scan_set *scan)
{
	if (((uint8_t *)scan < (uint8_t *)ll_scan) ||
	    ((uint8_t *)scan > ((uint8_t *)ll_scan +
				(sizeof(struct ll_scan_set) *
				 (BT_CTLR_SCAN_SET - 1))))) {
		return NULL;
	}

	return scan;
}

struct lll_scan *ull_scan_lll_is_valid_get(struct lll_scan *lll)
{
	struct ll_scan_set *scan;

	scan = HDR_LLL2ULL(lll);
	scan = ull_scan_is_valid_get(scan);
	if (scan) {
		return &scan->lll;
	}

	return NULL;
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
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		scan = ull_scan_set_get(handle);

		return scan->periodic.sync ? ULL_SCAN_IS_SYNC : 0U;
#else
		return 0U;
#endif
	}

	return (((uint32_t)scan->is_enabled << scan->lll.type) |
#if defined(CONFIG_BT_CENTRAL)
		(scan->lll.conn ? ULL_SCAN_IS_INITIATOR : 0U) |
#endif
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		(scan->periodic.sync ? ULL_SCAN_IS_SYNC : 0U) |
#endif
		0U);
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
#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL) && \
	!defined(CONFIG_BT_CTLR_ADV_EXT)
	ll_scan[0].lll.tx_pwr_lvl = RADIO_TXP_DEFAULT;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL && !CONFIG_BT_CTLR_ADV_EXT */

	return 0;
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_scan_prepare};
	static struct lll_prepare_param p;
	struct ll_scan_set *scan;
	struct lll_scan *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_O(1);

	scan = param;
	lll = &scan->lll;

	/* Increment prepare reference count */
	ref = ull_ref_inc(&scan->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.param = lll;
	p.force = force;
	mfy.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!ret);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (lll->duration_expire) {
		uint16_t elapsed;

		elapsed = lazy + 1;
		if (lll->duration_expire > elapsed) {
			lll->duration_expire -= elapsed;
		} else {
			if (scan->duration_lazy) {
				uint8_t handle;
				uint16_t duration_lazy;

				duration_lazy = lll->duration_expire +
						scan->duration_lazy - elapsed;

				handle = ull_scan_handle_get(scan);
				LL_ASSERT(handle < BT_CTLR_SCAN_SET);

				ret = ticker_update(TICKER_INSTANCE_ID_CTLR,
						    TICKER_USER_ID_ULL_HIGH,
						    (TICKER_ID_SCAN_BASE +
						     handle), 0, 0, 0, 0,
						    duration_lazy, 0,
						    NULL, NULL);
				LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
					  (ret == TICKER_STATUS_BUSY));
			}

			lll->duration_expire = 0U;
		}
	} else if (lll->duration_reload && lazy) {
		uint8_t handle;

		handle = ull_scan_handle_get(scan);
		LL_ASSERT(handle < BT_CTLR_SCAN_SET);

		lll->duration_expire = lll->duration_reload;
		ret = ticker_update(TICKER_INSTANCE_ID_CTLR,
				    TICKER_USER_ID_ULL_HIGH,
				    (TICKER_ID_SCAN_BASE + handle),
				    0, 0, 0, 0, 1, 1, NULL, NULL);
		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (ret == TICKER_STATUS_BUSY));
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	DEBUG_RADIO_PREPARE_O(1);
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
static uint8_t is_scan_update(uint8_t handle, uint16_t duration,
			      uint16_t period, struct ll_scan_set **scan,
			      struct node_rx_pdu **node_rx_scan_term)
{
	*scan = ull_scan_set_get(handle);
	*node_rx_scan_term = (*scan)->node_rx_scan_term;
	return duration && period && (*scan)->lll.duration_reload &&
	       (*scan)->duration_lazy;
}

static uint8_t duration_period_setup(struct ll_scan_set *scan,
				     uint16_t duration, uint16_t period,
				     struct node_rx_pdu **node_rx_scan_term)
{
	struct lll_scan *lll;

	lll = &scan->lll;
	if (duration) {
		lll->duration_reload =
			ULL_SCAN_DURATION_TO_EVENTS(duration,
						    scan->lll.interval);
		if (period) {
			if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK) &&
			    (duration >= ULL_SCAN_PERIOD_TO_DURATION(period))) {
				return BT_HCI_ERR_INVALID_PARAM;
			}

			scan->duration_lazy =
				ULL_SCAN_PERIOD_TO_EVENTS(period,
							  scan->lll.interval);
			scan->duration_lazy -= lll->duration_reload;
			scan->node_rx_scan_term = NULL;
		} else {
			struct node_rx_pdu *node_rx;
			void *link_scan_term;

			scan->duration_lazy = 0U;

			if (*node_rx_scan_term) {
				scan->node_rx_scan_term = *node_rx_scan_term;

				return 0;
			}

			/* The alloc here used for ext scan termination event */
			link_scan_term = ll_rx_link_alloc();
			if (!link_scan_term) {
				return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
			}

			node_rx = ll_rx_alloc();
			if (!node_rx) {
				ll_rx_link_release(link_scan_term);

				return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
			}

			node_rx->hdr.link = (void *)link_scan_term;
			scan->node_rx_scan_term = node_rx;
			*node_rx_scan_term = node_rx;
		}
	} else {
		lll->duration_reload = 0U;
		scan->duration_lazy = 0U;
		scan->node_rx_scan_term = NULL;
	}

	return 0;
}

static uint8_t duration_period_update(struct ll_scan_set *scan,
				      uint8_t is_update)
{
	if (is_update) {
		uint32_t volatile ret_cb;
		uint32_t ret;

		scan->lll.duration_expire = 0U;

		ret_cb = TICKER_STATUS_BUSY;
		ret = ticker_update(TICKER_INSTANCE_ID_CTLR,
				    TICKER_USER_ID_THREAD,
				    (TICKER_ID_SCAN_BASE +
				     ull_scan_handle_get(scan)),
				    0, 0, 0, 0, 1, 1,
				    ull_ticker_status_give, (void *)&ret_cb);
		ret = ull_ticker_status_take(ret, &ret_cb);
		if (ret != TICKER_STATUS_SUCCESS) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		return 0;
	} else {
		scan->lll.duration_expire = scan->lll.duration_reload;
	}

	return 0;
}

static void ticker_stop_ext_op_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, ext_disable};
	uint32_t ret;

	/* Ignore if race between thread and ULL */
	if (status != TICKER_STATUS_SUCCESS) {
		/* TODO: detect race */

		return;
	}

	/* Check if any pending LLL events that need to be aborted */
	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
			     TICKER_USER_ID_ULL_HIGH, 0, &mfy);
	LL_ASSERT(!ret);
}

static void ext_disable(void *param)
{
	struct ll_scan_set *scan;
	struct ull_hdr *hdr;

	/* Check ref count to determine if any pending LLL events in pipeline */
	scan = param;
	hdr = &scan->ull;
	if (ull_ref_get(hdr)) {
		static memq_link_t link;
		static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
		uint32_t ret;

		mfy.param = &scan->lll;

		/* Setup disabled callback to be called when ref count
		 * returns to zero.
		 */
		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = mfy.param;
		hdr->disabled_cb = ext_disabled_cb;

		/* Trigger LLL disable */
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);
	} else {
		/* No pending LLL events */
		ext_disabled_cb(&scan->lll);
	}
}

static void ext_disabled_cb(void *param)
{
	struct node_rx_pdu *rx;
	struct ll_scan_set *scan;
	struct lll_scan *lll;

	/* Under race condition, if a connection has been established then
	 * node_rx is already utilized to send terminate event on connection
	 */
	lll = (void *)param;
	scan = HDR_LLL2ULL(lll);
	rx = scan->node_rx_scan_term;
	if (!rx) {
		return;
	}

	/* NOTE: parameters are already populated on disable,
	 * just enqueue here
	 */
	ll_rx_put_sched(rx->hdr.link, rx);
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

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

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	if (scan->node_rx_scan_term) {
		struct node_rx_pdu *node_rx_scan_term = scan->node_rx_scan_term;

		scan->node_rx_scan_term = NULL;

		ll_rx_link_release(node_rx_scan_term->hdr.link);
		ll_rx_release(node_rx_scan_term);
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

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
