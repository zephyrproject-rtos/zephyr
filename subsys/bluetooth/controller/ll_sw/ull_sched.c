/*
 * Copyright (c) 2018-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>

#include <zephyr/bluetooth/hci_types.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"
#include "util/mem.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_adv_sync.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ll_sw/ull_tx_queue.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_conn_types.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_conn_internal.h"
#include "ull_conn_iso_internal.h"

#include "ll_feat.h"

#include "hal/debug.h"


#if defined(CONFIG_BT_CENTRAL)
static void after_cen_offset_get(uint16_t conn_interval, uint32_t ticks_slot,
				 uint32_t ticks_anchor,
				 uint32_t *win_offset_us);
static bool ticker_match_cen_op_cb(uint8_t ticker_id, uint32_t ticks_slot,
				   uint32_t ticks_to_expire, void *op_context);
#endif /* CONFIG_BT_CENTRAL */

typedef struct ull_hdr *(*ull_hdr_get_func)(uint8_t ticker_id,
					    uint32_t *ticks_slot);
static uint8_t after_match_slot_get(uint8_t user_id, uint32_t ticks_slot_abs,
				    ticker_op_match_func ticker_match_op_cb,
				    ull_hdr_get_func ull_hdr_get_cb_fn,
				    uint32_t *ticks_anchor,
				    uint32_t *ticks_to_expire_match,
				    uint32_t *remainder_match,
				    uint32_t *ticks_slot_match);
static void ticker_op_cb(uint32_t status, void *param);
static struct ull_hdr *ull_hdr_get_cb(uint8_t ticker_id, uint32_t *ticks_slot);

#if (defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_BROADCASTER)) || \
	defined(CONFIG_BT_CTLR_CENTRAL_ISO)
static int group_free_slot_get(uint8_t user_id, uint32_t ticks_slot_abs,
			       uint32_t *ticks_to_expire_prev,
			       uint32_t *ticks_slot_prev,
			       uint32_t *ticks_anchor);
static bool ticker_match_any_op_cb(uint8_t ticker_id, uint32_t ticks_slot,
				   uint32_t ticks_to_expire, void *op_context);
#endif /* (CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_BROADCASTER) ||
	* CONFIG_BT_CTLR_CENTRAL_ISO
	*/

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_BROADCASTER)
int ull_sched_adv_aux_sync_free_anchor_get(uint32_t ticks_slot_abs,
					   uint32_t *ticks_anchor)
{
	uint32_t ticks_to_expire;
	uint32_t ticks_slot;

	return group_free_slot_get(TICKER_USER_ID_THREAD, ticks_slot_abs,
				   &ticks_to_expire, &ticks_slot, ticks_anchor);
}
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
int ull_sched_conn_iso_free_offset_get(uint32_t ticks_slot_abs,
				       uint32_t *ticks_to_expire)
{
	uint32_t ticks_anchor;
	uint32_t ticks_slot;
	int err;

	err = group_free_slot_get(TICKER_USER_ID_ULL_LOW, ticks_slot_abs,
				  ticks_to_expire, &ticks_slot, &ticks_anchor);
	if (err) {
		return err;
	}

	*ticks_to_expire += ticks_slot;

	return err;
}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CENTRAL)
int ull_sched_after_cen_slot_get(uint8_t user_id, uint32_t ticks_slot_abs,
				 uint32_t *ticks_anchor, uint32_t *us_offset)
{
	uint32_t ticks_to_expire;
	uint32_t ticks_slot;
	uint32_t remainder = 0U;
	uint8_t ticker_id;

	ticker_id = after_match_slot_get(user_id, ticks_slot_abs,
					 ticker_match_cen_op_cb, ull_hdr_get_cb,
					 ticks_anchor, &ticks_to_expire,
					 &remainder, &ticks_slot);
	if (ticker_id != TICKER_NULL) {
		uint32_t central_spacing_us;
		uint32_t remainder_us;
		uint32_t slot_us;

		/* When CONFIG_BT_CTLR_CENTRAL_SPACING is used then ticks_slot
		 * returned converts to slot_us that is less than or equal to
		 * CONFIG_BT_CTLR_CENTRAL_SPACING, because floor value in ticks
		 * is returned as ticks_slot.
		 * Hence, use CONFIG_BT_CTLR_CENTRAL_SPACING without margin.
		 *
		 * If actual ticks_slot in microseconds is larger than
		 * CONFIG_BT_CTLR_CENTRAL_SPACING, then add margin between
		 * central radio events.
		 */
		slot_us = HAL_TICKER_TICKS_TO_US(ticks_slot);
		if (slot_us > CONFIG_BT_CTLR_CENTRAL_SPACING) {
			central_spacing_us = slot_us;
			central_spacing_us += (EVENT_TICKER_RES_MARGIN_US << 1);
		} else {
			central_spacing_us = CONFIG_BT_CTLR_CENTRAL_SPACING;
		}

		remainder_us = remainder;
		hal_ticker_remove_jitter(&ticks_to_expire, &remainder_us);

		*us_offset = HAL_TICKER_TICKS_TO_US(ticks_to_expire) +
			     remainder_us + central_spacing_us;

		return 0;
	}

	return -ECHILD;
}

void ull_sched_mfy_after_cen_offset_get(void *param)
{
	struct lll_prepare_param *p = param;
	struct lll_scan *lll = p->param;
	uint32_t ticks_slot_overhead;
	uint32_t ticks_at_expire;
	uint32_t remainder_us;
	struct ll_conn *conn;

	conn = HDR_LLL2ULL(lll->conn);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	} else {
		ticks_slot_overhead = 0U;
	}

	ticks_at_expire = p->ticks_at_expire;
	remainder_us = p->remainder;
	hal_ticker_remove_jitter(&ticks_at_expire, &remainder_us);

	after_cen_offset_get(lll->conn->interval,
			     (ticks_slot_overhead + conn->ull.ticks_slot),
			     ticks_at_expire, &lll->conn_win_offset_us);
	if (lll->conn_win_offset_us) {
		lll->conn_win_offset_us +=
			HAL_TICKER_TICKS_TO_US(p->ticks_at_expire -
					       ticks_at_expire) -
			remainder_us;
	}
}
#endif /* CONFIG_BT_CENTRAL */

void ull_sched_mfy_win_offset_use(void *param)
{
	uint32_t ticks_slot_overhead;

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	} else {
		ticks_slot_overhead = 0U;
	}

	/*
	 * TODO: update calculationg of the win_offset
	 * when updating the connection update procedure
	 * see the legacy code from Zephyr v3.3 for inspiration
	 */
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
void ull_sched_mfy_free_win_offset_calc(void *param)
{
	uint32_t ticks_to_offset_default = 0U;
	uint32_t *ticks_to_offset_next;

	ticks_to_offset_next = &ticks_to_offset_default;

	/*
	 * TODO: update when updating the connection update procedure
	 * see the legacy code from Zephyr v3.3 for inspiration
	 */
}

void ull_sched_mfy_win_offset_select(void *param)
{
#define OFFSET_S_MAX 6
#define OFFSET_M_MAX 6

	/*
	 * TODO: update calculation of win_offset when
	 * updating the connection update procedure
	 * see the legacy code from Zephyr v3.3 for inspiration
	 */

#undef OFFSET_S_MAX
#undef OFFSET_M_MAX
}

/*
 * TODO: probably we need a function for calculating the window offset
 * see the legacy code from Zephyr v3.3 for inspiration
 */
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CENTRAL)
static void after_cen_offset_get(uint16_t conn_interval, uint32_t ticks_slot,
				 uint32_t ticks_anchor,
				 uint32_t *win_offset_us)
{
	uint32_t ticks_anchor_offset = ticks_anchor;
	int err;

	err = ull_sched_after_cen_slot_get(TICKER_USER_ID_ULL_LOW, ticks_slot,
					   &ticks_anchor_offset,
					   win_offset_us);
	if (err) {
		return;
	}

	if ((ticks_anchor_offset - ticks_anchor) & BIT(HAL_TICKER_CNTR_MSBIT)) {
		*win_offset_us -= HAL_TICKER_TICKS_TO_US(
			ticker_ticks_diff_get(ticks_anchor,
					      ticks_anchor_offset));
	} else {
		*win_offset_us += HAL_TICKER_TICKS_TO_US(
			ticker_ticks_diff_get(ticks_anchor_offset,
					      ticks_anchor));
	}

	/* Round positive offset value in the future to within one connection
	 * interval value.
	 * Offsets in the past, value with MSBit set, are handled by caller by
	 * adding radio end time and connection interval as necessary to get a
	 * window offset in future when establishing a connection.
	 */
	if ((*win_offset_us & BIT(31)) == 0) {
		uint32_t conn_interval_us = conn_interval * CONN_INT_UNIT_US;

		while (*win_offset_us > conn_interval_us) {
			*win_offset_us -= conn_interval_us;
		}
	}
}
#endif /* CONFIG_BT_CENTRAL */
#endif /* CONFIG_BT_CONN */

#if (defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_BROADCASTER)) || \
	defined(CONFIG_BT_CTLR_CENTRAL_ISO)
static int group_free_slot_get(uint8_t user_id, uint32_t ticks_slot_abs,
			       uint32_t *ticks_to_expire_prev,
			       uint32_t *ticks_slot_prev,
			       uint32_t *ticks_anchor)
{
	uint32_t remainder_prev;
	uint8_t ticker_id;

	ticker_id = after_match_slot_get(user_id, ticks_slot_abs,
					 ticker_match_any_op_cb, ull_hdr_get_cb,
					 ticks_anchor, ticks_to_expire_prev,
					 &remainder_prev, ticks_slot_prev);
	if (ticker_id != TICKER_NULL) {
		uint32_t ticks_to_expire;
		uint32_t ticks_slot;

		ticks_to_expire = *ticks_to_expire_prev;
		ticks_slot = *ticks_slot_prev;

		if (false) {

#if defined(CONFIG_BT_BROADCASTER) && CONFIG_BT_CTLR_ADV_AUX_SET > 0
		} else if (IN_RANGE(ticker_id, TICKER_ID_ADV_AUX_BASE,
				    TICKER_ID_ADV_AUX_LAST)) {
			*ticks_anchor += ticks_to_expire;
			*ticks_anchor += ticks_slot;
			*ticks_anchor += HAL_TICKER_US_TO_TICKS(
				EVENT_TICKER_RES_MARGIN_US << 1);

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
			const struct ll_adv_aux_set *aux;

			aux = ull_adv_aux_get(ticker_id -
					      TICKER_ID_ADV_AUX_BASE);
			if (aux->lll.adv->sync) {
				const struct ll_adv_sync_set *sync;

				sync = HDR_LLL2ULL(aux->lll.adv->sync);
				if (sync->is_started) {
					*ticks_anchor += sync->ull.ticks_slot;
					*ticks_anchor += HAL_TICKER_US_TO_TICKS(
						EVENT_TICKER_RES_MARGIN_US << 1);
				}
			}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

			return 0;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
		} else if (IN_RANGE(ticker_id, TICKER_ID_ADV_SYNC_BASE,
				    TICKER_ID_ADV_SYNC_LAST)) {
			*ticks_anchor += ticks_to_expire;
			*ticks_anchor += ticks_slot;
			*ticks_anchor += HAL_TICKER_US_TO_TICKS(
				EVENT_TICKER_RES_MARGIN_US << 1);

			return 0;

#if defined(CONFIG_BT_CTLR_ADV_ISO)
		} else if (IN_RANGE(ticker_id, TICKER_ID_ADV_ISO_BASE,
				    TICKER_ID_ADV_ISO_LAST)) {
			*ticks_anchor += ticks_to_expire;
			*ticks_anchor += ticks_slot;
			*ticks_anchor += HAL_TICKER_US_TO_TICKS(
				EVENT_TICKER_RES_MARGIN_US << 1);

			return 0;

#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_BROADCASTER && CONFIG_BT_CTLR_ADV_AUX_SET > 0 */

#if defined(CONFIG_BT_CONN)
		} else if (IN_RANGE(ticker_id, TICKER_ID_CONN_BASE,
				    TICKER_ID_CONN_LAST)) {
			*ticks_anchor += ticks_to_expire;
			*ticks_anchor += ticks_slot;
			*ticks_anchor += HAL_TICKER_US_TO_TICKS(
				EVENT_TICKER_RES_MARGIN_US << 1);

			return 0;
#endif /* CONFIG_BT_CONN */

		}
	}

	return -ECHILD;
}
#endif /* (CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_BROADCASTER) ||
	* CONFIG_BT_CTLR_CENTRAL_ISO
	*/

static uint8_t after_match_slot_get(uint8_t user_id, uint32_t ticks_slot_abs,
				    ticker_op_match_func ticker_match_op_cb,
				    ull_hdr_get_func ull_hdr_get_cb_fn,
				    uint32_t *ticks_anchor,
				    uint32_t *ticks_to_expire_match,
				    uint32_t *remainder_match,
				    uint32_t *ticks_slot_match)
{
	uint32_t ticks_to_expire_prev;
	uint32_t ticks_slot_abs_prev;
	uint32_t ticks_anchor_prev;
	uint32_t ticks_to_expire;
	uint32_t remainder_prev;
	uint32_t remainder;
	uint8_t ticker_id_prev;
	uint8_t ticker_id;
	uint8_t retry;

	/* As we may place the event between two other events, ensure there is
	 * space for 4 times +/- 16 us jitter. I.e. with 32KHz sleep clock,
	 * ~30.517 us after previous event, ~30.517 us before and after current
	 * event, and an ~30.517 us before next event. Hence 8 time of ceil
	 * value 16 us (30.517 / 2) or 4 times EVENT_TICKER_RES_MARGIN_US.
	 */
	ticks_slot_abs += HAL_TICKER_US_TO_TICKS(EVENT_TICKER_RES_MARGIN_US << 2);

	/* There is a possibility that ticker nodes expire during iterations in
	 * this function causing the reference ticks_anchor returned for the
	 * found ticker to change. In this case the iterations have to be
	 * restarted with the new reference ticks_anchor value.
	 * Simultaneous continuous scanning on 1M and Coded PHY, alongwith
	 * directed advertising and N other state/role could expire in quick
	 * succession, hence have a retry count of UINT8_MAX, which is possible
	 * maximum implementation limit for ticker nodes.
	 */
	retry = UINT8_MAX;

	/* Initialize variable required for iterations to find a free slot */
	ticker_id = ticker_id_prev = TICKER_NULL;
	ticks_anchor_prev = 0U;
	ticks_to_expire = ticks_to_expire_prev = 0U;
	remainder = remainder_prev = 0U;
	ticks_slot_abs_prev = 0U;
	while (1) {
		uint32_t ticks_slot_abs_curr = 0U;
		uint32_t ticks_to_expire_normal;
		uint32_t volatile ret_cb;
		uint32_t ticks_slot;
		struct ull_hdr *hdr;
		uint32_t ret;
		bool success;

		ret_cb = TICKER_STATUS_BUSY;
#if defined(CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH)
		ret = ticker_next_slot_get_ext(TICKER_INSTANCE_ID_CTLR, user_id,
					       &ticker_id, ticks_anchor,
					       &ticks_to_expire, &remainder,
					       NULL, /* lazy */
					       ticker_match_op_cb,
					       NULL, /* match_op_context */
					       ticker_op_cb, (void *)&ret_cb);
#else /* !CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH */
		ret = ticker_next_slot_get(TICKER_INSTANCE_ID_CTLR, user_id,
					   &ticker_id, ticks_anchor,
					   &ticks_to_expire,
					   ticker_op_cb, (void *)&ret_cb);
#endif /* !CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH */
		if (ret == TICKER_STATUS_BUSY) {
			while (ret_cb == TICKER_STATUS_BUSY) {
				ticker_job_sched(TICKER_INSTANCE_ID_CTLR,
						 user_id);
			}
		}

		/* Using a local variable to address the Coverity rule:
		 * Incorrect expression  (ASSERT_SIDE_EFFECT)
		 * Argument "ret_cb" of LL_ASSERT() has a side effect
		 * because the variable is volatile.  The containing function
		 * might work differently in a non-debug build.
		 */
		success = (ret_cb == TICKER_STATUS_SUCCESS);
		LL_ASSERT(success);

		/* There is a possibility that tickers expire while we
		 * iterate through the active list of tickers, start over with
		 * a fresh iteration.
		 */
		if ((ticker_id_prev != TICKER_NULL) &&
		    (*ticks_anchor != ticks_anchor_prev)) {
			LL_ASSERT(retry);
			retry--;

			ticker_id = ticker_id_prev = TICKER_NULL;

			continue;
		}

		/* No more active tickers with slot */
		if (ticker_id == TICKER_NULL) {
			break;
		}

#if !defined(CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH)
		if (!ticker_match_op_cb(ticker_id, 0, 0, NULL)) {
			continue;
		}
#endif /* CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH */

		hdr = ull_hdr_get_cb_fn(ticker_id, &ticks_slot);
		if (!hdr) {
			continue;
		}

#if defined(CONFIG_BT_CONN)
		/* Consider Peripheral role ticks_slot as available */
		if (IN_RANGE(ticker_id, TICKER_ID_CONN_BASE, TICKER_ID_CONN_LAST)) {
			const struct ll_conn *conn;

			conn = ll_conn_get(ticker_id - TICKER_ID_CONN_BASE);
			if (conn && (conn->lll.role == BT_HCI_ROLE_PERIPHERAL)) {
				continue;
			}
		}
#endif /* CONFIG_BT_CONN */

		ticks_to_expire_normal = ticks_to_expire;

#if defined(CONFIG_BT_CTLR_LOW_LAT)
		ticks_slot_abs_curr = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
#endif

		ticks_slot_abs_curr += ticks_slot;

		if ((ticker_id_prev != TICKER_NULL) &&
		    (ticker_ticks_diff_get(ticks_to_expire_normal,
					   ticks_to_expire_prev) >
		     (ticks_slot_abs_prev + ticks_slot_abs))) {
			break;
		}

		ticks_anchor_prev = *ticks_anchor;
		ticker_id_prev = ticker_id;
		ticks_to_expire_prev = ticks_to_expire_normal;
		remainder_prev = remainder;
		ticks_slot_abs_prev = ticks_slot_abs_curr;
	}

	if (ticker_id_prev != TICKER_NULL) {
		*ticks_to_expire_match = ticks_to_expire_prev;
		*remainder_match = remainder_prev;
		*ticks_slot_match = ticks_slot_abs_prev;
	}

	return ticker_id_prev;
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}

#if (defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_BROADCASTER)) || \
	defined(CONFIG_BT_CTLR_CENTRAL_ISO)
static bool ticker_match_any_op_cb(uint8_t ticker_id, uint32_t ticks_slot,
				   uint32_t ticks_to_expire, void *op_context)
{
	ARG_UNUSED(ticks_slot);
	ARG_UNUSED(ticks_to_expire);
	ARG_UNUSED(op_context);

	return false ||

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_BROADCASTER)
	       IN_RANGE(ticker_id, TICKER_ID_ADV_AUX_BASE,
			TICKER_ID_ADV_AUX_LAST) ||

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	       IN_RANGE(ticker_id, TICKER_ID_ADV_SYNC_BASE,
			TICKER_ID_ADV_SYNC_LAST) ||

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	       IN_RANGE(ticker_id, TICKER_ID_ADV_ISO_BASE,
			TICKER_ID_ADV_ISO_LAST) ||
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_CONN)
	       IN_RANGE(ticker_id, TICKER_ID_CONN_BASE,
			TICKER_ID_CONN_LAST) ||

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	       IN_RANGE(ticker_id, TICKER_ID_CONN_ISO_BASE,
			TICKER_ID_CONN_ISO_LAST) ||
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */
#endif /* CONFIG_BT_CONN */

	       false;
}
#endif /* (CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_BROADCASTER) ||
	* CONFIG_BT_CTLR_CENTRAL_ISO
	*/

#if defined(CONFIG_BT_CENTRAL)
static bool ticker_match_cen_op_cb(uint8_t ticker_id, uint32_t ticks_slot,
				   uint32_t ticks_to_expire, void *op_context)
{
	ARG_UNUSED(ticks_slot);
	ARG_UNUSED(ticks_to_expire);
	ARG_UNUSED(op_context);

	return IN_RANGE(ticker_id, TICKER_ID_CONN_BASE,
			TICKER_ID_CONN_LAST) ||
#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) && \
		(CONFIG_BT_CTLR_CENTRAL_SPACING == 0)
	       IN_RANGE(ticker_id, TICKER_ID_CONN_ISO_BASE,
			TICKER_ID_CONN_ISO_LAST) ||
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO &&
	*  (CONFIG_BT_CTLR_CENTRAL_SPACING == 0)
	*/
	       false;
}
#endif /* CONFIG_BT_CENTRAL */

static struct ull_hdr *ull_hdr_get_cb(uint8_t ticker_id, uint32_t *ticks_slot)
{
	if (false) {

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_BROADCASTER)
	} else if (IN_RANGE(ticker_id, TICKER_ID_ADV_AUX_BASE,
		     TICKER_ID_ADV_AUX_LAST)) {
		struct ll_adv_aux_set *aux;

		aux = ull_adv_aux_get(ticker_id - TICKER_ID_ADV_AUX_BASE);
		if (aux) {
			if (IS_ENABLED(CONFIG_BT_CTLR_ADV_RESERVE_MAX)) {
				uint32_t time_us;

				time_us = ull_adv_aux_time_get(aux, PDU_AC_PAYLOAD_SIZE_MAX,
							       PDU_AC_PAYLOAD_SIZE_MAX);
				*ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(time_us);
			} else {
				*ticks_slot = aux->ull.ticks_slot;

#if defined(CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET) && \
	(CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET != 0)
				struct ll_adv_sync_set *sync;

				sync = HDR_LLL2ULL(aux->lll.adv->sync);
				if (sync->ull.ticks_slot > *ticks_slot) {
					*ticks_slot = sync->ull.ticks_slot;
				}
#endif /* CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET */

			}

			return &aux->ull;
		}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	} else if (IN_RANGE(ticker_id, TICKER_ID_ADV_SYNC_BASE,
		     TICKER_ID_ADV_SYNC_LAST)) {
		struct ll_adv_sync_set *sync;

		sync = ull_adv_sync_get(ticker_id - TICKER_ID_ADV_SYNC_BASE);
		if (sync) {
			if (IS_ENABLED(CONFIG_BT_CTLR_ADV_RESERVE_MAX)) {
				uint32_t time_us;

				time_us = ull_adv_sync_time_get(sync, PDU_AC_PAYLOAD_SIZE_MAX);
				*ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(time_us);
			} else {
				*ticks_slot = sync->ull.ticks_slot;
			}

			return &sync->ull;
		}

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	} else if (IN_RANGE(ticker_id, TICKER_ID_ADV_ISO_BASE,
			    TICKER_ID_ADV_ISO_LAST)) {
		struct ll_adv_iso_set *adv_iso;

		adv_iso = ull_adv_iso_get(ticker_id - TICKER_ID_ADV_ISO_BASE);
		if (adv_iso) {
			uint32_t time_us;

			time_us = ull_adv_iso_max_time_get(adv_iso);
			*ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(time_us);

			return &adv_iso->ull;
		}

#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_CONN)
	} else if (IN_RANGE(ticker_id, TICKER_ID_CONN_BASE,
			    TICKER_ID_CONN_LAST)) {
		struct ll_conn *conn;

		conn = ll_conn_get(ticker_id - TICKER_ID_CONN_BASE);
		if (conn) {
			if (IS_ENABLED(CONFIG_BT_CTLR_CENTRAL_RESERVE_MAX) &&
			    (conn->lll.role == BT_HCI_ROLE_CENTRAL)) {
				uint32_t ready_delay_us;
				uint16_t max_tx_time;
				uint16_t max_rx_time;
				uint32_t time_us;

#if defined(CONFIG_BT_CTLR_PHY)
				ready_delay_us =
					lll_radio_tx_ready_delay_get(conn->lll.phy_tx,
								     conn->lll.phy_flags);
#else
				ready_delay_us =
					lll_radio_tx_ready_delay_get(0U, 0U);
#endif

#if defined(CONFIG_BT_CTLR_PHY_CODED)
				max_tx_time = PDU_DC_MAX_US(LL_LENGTH_OCTETS_TX_MAX,
							    PHY_CODED);
				max_rx_time = PDU_DC_MAX_US(LL_LENGTH_OCTETS_RX_MAX,
							    PHY_CODED);
#else /* !CONFIG_BT_CTLR_PHY_CODED */
				max_tx_time = PDU_DC_MAX_US(LL_LENGTH_OCTETS_TX_MAX,
							    PHY_1M);
				max_rx_time = PDU_DC_MAX_US(LL_LENGTH_OCTETS_RX_MAX,
							    PHY_1M);
#endif /* !CONFIG_BT_CTLR_PHY_CODED */

				time_us = EVENT_OVERHEAD_START_US +
					  EVENT_OVERHEAD_END_US +
					  ready_delay_us +  max_rx_time +
					  EVENT_IFS_US + max_tx_time +
					  (EVENT_CLOCK_JITTER_US << 1);
				*ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(time_us);
			} else {
				*ticks_slot = conn->ull.ticks_slot;
			}

			if ((conn->lll.role == BT_HCI_ROLE_CENTRAL) &&
			    (*ticks_slot <
			     HAL_TICKER_US_TO_TICKS(CONFIG_BT_CTLR_CENTRAL_SPACING))) {
				*ticks_slot =
					HAL_TICKER_US_TO_TICKS(CONFIG_BT_CTLR_CENTRAL_SPACING);
			}

			return &conn->ull;
		}

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	} else if (IN_RANGE(ticker_id, TICKER_ID_CONN_ISO_BASE,
			    TICKER_ID_CONN_ISO_LAST)) {
		struct ll_conn_iso_group *cig;

		cig = ll_conn_iso_group_get(ticker_id - TICKER_ID_CONN_ISO_BASE);
		if (cig) {
			*ticks_slot = cig->ull.ticks_slot;

			return &cig->ull;
		}

#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */
#endif /* CONFIG_BT_CONN */

	}

	return NULL;
}
