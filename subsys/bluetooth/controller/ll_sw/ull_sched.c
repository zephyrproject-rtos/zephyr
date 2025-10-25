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

#if !defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
#include "ull_tx_queue.h"
#endif /* CONFIG_BT_LL_SW_LLCP_LEGACY */

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
#include "ull_sched_internal.h"

#include "ll_feat.h"

#include "hal/debug.h"

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
static void win_offset_calc(struct ll_conn *conn_curr, uint8_t is_select,
			    uint32_t *ticks_to_offset_next,
			    uint16_t conn_interval, uint8_t *offset_max,
			    uint8_t *win_offset);
#endif /* CONFIG_BT_LL_SW_LLCP_LEGACY */
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CENTRAL)
static void after_cen_offset_get(uint16_t conn_interval, uint32_t ticks_slot,
				 uint32_t ticks_anchor,
				 uint32_t *win_offset_us);
static bool ticker_match_cen_op_cb(uint8_t ticker_id, uint32_t ticks_slot,
				   uint32_t ticks_to_expire, void *op_context);
#endif /* CONFIG_BT_CENTRAL */
#endif /* CONFIG_BT_CONN */

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
	uint32_t remainder;
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
		ticks_slot_overhead = MAX(conn->ull.ticks_active_to_start,
					  conn->ull.ticks_prepare_to_start);
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

void ull_sched_mfy_win_offset_use(void *param)
{
	/*
	 * TODO: update when updating the connection update procedure
	 */
#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
	struct ll_conn *conn = param;
	uint32_t ticks_slot_overhead;
	uint16_t win_offset;

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(conn->ull.ticks_active_to_start,
					  conn->ull.ticks_prepare_to_start);
	} else {
		ticks_slot_overhead = 0U;
	}

	after_cen_offset_get(conn->lll.interval,
			     (ticks_slot_overhead + conn->ull.ticks_slot),
			     conn->llcp.conn_upd.ticks_anchor,
			     &conn->llcp_cu.win_offset_us);

	win_offset = conn->llcp_cu.win_offset_us / CONN_INT_UNIT_US;

	sys_put_le16(win_offset, (void *)conn->llcp.conn_upd.pdu_win_offset);

	conn->llcp.conn_upd.select_conn_interval = conn->lll.interval;

	/* move to offset calculated state */
	conn->llcp_cu.state = LLCP_CUI_STATE_OFFS_RDY;
#endif /* CONFIG_BT_LL_SW_LLCP_LEGACY */
}
#endif /* CONFIG_BT_CENTRAL */

uint16_t ull_sched_offset_at_instant(uint16_t offset, uint16_t reference,
				     uint16_t instant, uint16_t interval_old,
				     uint16_t interval_new)
{
	uint16_t elapsed_old, elapsed_new, offset_at_instant;
	uint16_t latency_old, latency_new;

	latency_old = instant - reference;
	elapsed_old = latency_old * interval_old;

	latency_new = DIV_ROUND_UP(elapsed_old, interval_new);
	elapsed_new = latency_new * interval_new;

	offset_at_instant = offset + (elapsed_new - elapsed_old);
	while (offset_at_instant >= interval_new) {
		offset_at_instant -= interval_new;
	}

	return offset_at_instant;
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
void ull_sched_mfy_free_win_offset_calc(void *param)
{
	uint32_t ticks_to_offset_default = 0U;
	uint32_t *ticks_to_offset_next;

	/*
	 * TODO: update when updating the connection update procedure
	 */
#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
	uint8_t offset_max = 6U;
	struct ll_conn *conn = param;
#endif /* CONFIG_BT_LL_SW_LLCP_LEGACY */

	ticks_to_offset_next = &ticks_to_offset_default;

	/*
	 * TODO: update when updating the connection update procedure
	 */
#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)

#if defined(CONFIG_BT_PERIPHERAL)
	if (conn->lll.role) {
		conn->llcp_conn_param.ticks_to_offset_next =
			conn->periph.ticks_to_offset;

		ticks_to_offset_next =
			&conn->llcp_conn_param.ticks_to_offset_next;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	win_offset_calc(conn, 0, ticks_to_offset_next,
			conn->llcp_conn_param.interval_max, &offset_max,
			(void *)conn->llcp_conn_param.pdu_win_offset0);

	/* move to offset calculated state */
	conn->llcp_conn_param.state = LLCP_CPR_STATE_OFFS_RDY;
#endif /* CONFIG_BT_LL_SW_LLCP_LEGACY */
}

void ull_sched_mfy_win_offset_select(void *param)
{
	/*
	 * TODO: update when updating the connection update procedure
	 */

#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
#define OFFSET_S_MAX 6
#define OFFSET_M_MAX 6

	uint16_t win_offset_m[OFFSET_M_MAX] = {0, };
	uint8_t offset_m_max = OFFSET_M_MAX;
	struct ll_conn *conn = param;
	uint8_t offset_index_s = 0U;
	uint8_t has_offset_s = 0U;
	uint16_t win_offset_s;
	uint32_t ticks_to_offset;
	uint16_t win_offset;

	win_offset = ull_sched_offset_at_instant(
		conn->llcp_conn_param.offset0,
		conn->llcp_conn_param.reference_conn_event_count,
		conn->llcp.conn_upd.select_conn_event_count,
		conn->lll.interval,
		conn->llcp_cu.interval);

	ticks_to_offset = HAL_TICKER_US_TO_TICKS(win_offset * CONN_INT_UNIT_US);

	win_offset_calc(conn, 1, &ticks_to_offset,
			conn->llcp_conn_param.interval_max, &offset_m_max,
			(void *)win_offset_m);

	while (offset_index_s < OFFSET_S_MAX) {
		uint8_t offset_index_m = 0U;

		win_offset_s =
			sys_get_le16((uint8_t *)&conn->llcp_conn_param.offset0 +
				     (sizeof(uint16_t) * offset_index_s));

		win_offset = ull_sched_offset_at_instant(win_offset_s,
			conn->llcp_conn_param.reference_conn_event_count,
			conn->llcp.conn_upd.select_conn_event_count,
			conn->lll.interval,
			conn->llcp_cu.interval);

		while (offset_index_m < offset_m_max) {
			if (win_offset_s != 0xffff) {
				if (win_offset ==
				    win_offset_m[offset_index_m]) {
					break;
				}

				has_offset_s = 1U;
			}

			offset_index_m++;
		}

		if (offset_index_m < offset_m_max) {
			break;
		}

		offset_index_s++;
	}

	if (offset_index_s < OFFSET_S_MAX) {
		conn->llcp_cu.win_offset_us = win_offset_s * CONN_INT_UNIT_US;
		sys_put_le16(win_offset_s,
			     (void *)conn->llcp.conn_upd.pdu_win_offset);
		conn->llcp.conn_upd.select_conn_event_count =
			conn->llcp_conn_param.reference_conn_event_count;
		conn->llcp.conn_upd.select_conn_interval = conn->llcp_cu.interval;
		/* move to offset calculated state */
		conn->llcp_cu.state = LLCP_CUI_STATE_OFFS_RDY;
	} else if (!has_offset_s) {
		conn->llcp_cu.win_offset_us = win_offset_m[0] *
					      CONN_INT_UNIT_US;
		sys_put_le16(win_offset_m[0],
			     (void *)conn->llcp.conn_upd.pdu_win_offset);
		conn->llcp.conn_upd.select_conn_interval = conn->lll.interval;
		/* move to offset calculated state */
		conn->llcp_cu.state = LLCP_CUI_STATE_OFFS_RDY;
	} else {
		struct pdu_data *pdu_ctrl_tx;

		/* send reject_ind_ext */
		pdu_ctrl_tx = CONTAINER_OF(conn->llcp.conn_upd.pdu_win_offset,
					   struct pdu_data,
					   llctrl.conn_update_ind.win_offset);
		pdu_ctrl_tx->ll_id = PDU_DATA_LLID_CTRL;
		pdu_ctrl_tx->len =
			offsetof(struct pdu_data_llctrl, reject_ext_ind) +
			sizeof(struct pdu_data_llctrl_reject_ext_ind);
		pdu_ctrl_tx->llctrl.opcode =
			PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND;
		pdu_ctrl_tx->llctrl.reject_ext_ind.reject_opcode =
			PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ;
		pdu_ctrl_tx->llctrl.reject_ext_ind.error_code =
			BT_HCI_ERR_UNSUPP_LL_PARAM_VAL;
		/* move to conn param reject */
		conn->llcp_cu.state = LLCP_CUI_STATE_REJECT;
	}
#undef OFFSET_S_MAX
#undef OFFSET_M_MAX
#endif /* CONFIG_BT_LL_SW_LLCP_LEGACY */
}

#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
static void win_offset_calc(struct ll_conn *conn_curr, uint8_t is_select,
			    uint32_t *ticks_to_offset_next,
			    uint16_t conn_interval, uint8_t *offset_max,
			    uint8_t *win_offset)
{
	uint32_t ticks_prepare_reduced = 0U;
	uint32_t ticks_to_expire_prev;
	uint32_t ticks_slot_abs_prev;
	uint32_t ticks_slot_abs = 0U;
	uint32_t ticks_anchor_prev;
	uint32_t ticks_to_expire;
	uint8_t ticker_id_prev;
	uint32_t ticks_anchor;
	uint8_t offset_index;
	uint8_t ticker_id;
	uint16_t offset;

	bool is_conn_interval = false;
	bool is_after_conn_interval = false;

#if defined(CONFIG_BT_CTLR_LOW_LAT)
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
	if (conn_curr->ull.ticks_prepare_to_start & XON_BITMASK) {
		uint32_t ticks_prepare_to_start =
			MAX(conn_curr->ull.ticks_active_to_start,
			    conn_curr->ull.ticks_preempt_to_start);

		ticks_slot_abs = conn_curr->ull.ticks_prepare_to_start &
				 ~XON_BITMASK;
		ticks_prepare_reduced = ticks_slot_abs - ticks_prepare_to_start;
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		uint32_t ticks_prepare_to_start =
			MAX(conn_curr->ull.ticks_active_to_start,
			    conn_curr->ull.ticks_prepare_to_start);

		ticks_slot_abs = ticks_prepare_to_start;
	}
#endif

	ticks_slot_abs += conn_curr->ull.ticks_slot;
	if (conn_curr->lll.role) {
		ticks_slot_abs += HAL_TICKER_US_TO_TICKS(EVENT_TIES_US);
	}

	if (ticks_slot_abs < HAL_TICKER_US_TO_TICKS(CONFIG_BT_CTLR_CENTRAL_SPACING)) {
		ticks_slot_abs =
			HAL_TICKER_US_TO_TICKS(CONFIG_BT_CTLR_CENTRAL_SPACING);
	}

	ticker_id = ticker_id_prev = TICKER_NULL;
	ticks_to_expire = ticks_to_expire_prev = ticks_anchor =
		ticks_anchor_prev = offset_index = offset = 0U;
	ticks_slot_abs_prev = 0U;
	do {
		uint32_t ticks_slot_abs_curr = 0U;
		uint32_t ticks_slot_margin = 0U;
		uint32_t ticks_to_expire_normal;
		uint32_t volatile ret_cb;
		struct ull_hdr *hdr;
		uint32_t ticks_slot;
		uint32_t ret;
		bool success;

		ret_cb = TICKER_STATUS_BUSY;
		ret = ticker_next_slot_get(TICKER_INSTANCE_ID_CTLR,
					   TICKER_USER_ID_ULL_LOW,
					   &ticker_id, &ticks_anchor,
					   &ticks_to_expire, ticker_op_cb,
					   (void *)&ret_cb);
		if (ret == TICKER_STATUS_BUSY) {
			while (ret_cb == TICKER_STATUS_BUSY) {
				ticker_job_sched(TICKER_INSTANCE_ID_CTLR,
						 TICKER_USER_ID_ULL_LOW);
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

		if (ticker_id == TICKER_NULL) {
			break;
		}

		/* ticks_anchor shall not change during this loop */
		if ((ticker_id_prev != TICKER_NULL) &&
		    (ticks_anchor != ticks_anchor_prev)) {
			LL_ASSERT(0);
		}

		/* Save the reference anchor and ticker id to check updated
		 * ticker list.
		 */
		ticks_anchor_prev = ticks_anchor;
		ticker_id_prev = ticker_id;

		/* Get ULL header and time reservation for non-overlapping
		 * tickers.
		 */
		hdr = ull_hdr_get_cb(ticker_id, &ticks_slot);
		if (!hdr) {
			/* TODO: check overlapping scanner */
			LL_ASSERT(!is_select);

			continue;
		}

		ticks_to_expire_normal = ticks_to_expire +
					 ticks_prepare_reduced;

#if defined(CONFIG_BT_CTLR_LOW_LAT)
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
		if (hdr->ticks_prepare_to_start & XON_BITMASK) {
			const uint32_t ticks_prepare_to_start =
				MAX(hdr->ticks_active_to_start,
				    hdr->ticks_preempt_to_start);

			ticks_slot_abs_curr = hdr->ticks_prepare_to_start &
					      ~XON_BITMASK;
			ticks_to_expire_normal -= ticks_slot_abs_curr -
						  ticks_prepare_to_start;
		} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
		{
			const uint32_t ticks_prepare_to_start =
				MAX(hdr->ticks_active_to_start,
				    hdr->ticks_prepare_to_start);

			ticks_slot_abs_curr = ticks_prepare_to_start;
		}
#endif

		if (ticker_id >= TICKER_ID_CONN_BASE) {
			struct ll_conn *conn;

			conn = ll_conn_get(ticker_id - TICKER_ID_CONN_BASE);
			if (!is_select && conn->lll.role) {
				continue;
			}

			if (is_select &&
			    (conn->lll.interval != conn_interval)) {
				ticks_slot_abs_curr += ticks_slot_abs;
			} else if (is_select) {
				is_conn_interval = true;
			}

			if (conn->lll.role) {
				ticks_slot_margin =
					HAL_TICKER_US_TO_TICKS(EVENT_TIES_US);
				ticks_slot_abs_curr += ticks_slot_margin;
			}
		}

		if ((!is_select || is_after_conn_interval) &&
		    (*ticks_to_offset_next < ticks_to_expire_normal)) {
			if (ticks_to_expire_prev <
			    *ticks_to_offset_next) {
				ticks_to_expire_prev =
					*ticks_to_offset_next;
			}

			while ((offset_index < *offset_max) &&
			       (ticker_ticks_diff_get(
						ticks_to_expire_normal,
						ticks_to_expire_prev) >=
				(ticks_slot_abs_prev + ticks_slot_abs +
				 ticks_slot_margin))) {
				offset = DIV_ROUND_UP(
					(ticks_to_expire_prev +
					 ticks_slot_abs_prev),
					 HAL_TICKER_US_TO_TICKS(
						CONN_INT_UNIT_US));
				while (offset >= conn_interval) {
					offset -= conn_interval;
				}

				sys_put_le16(offset,
					     (win_offset +
					      (sizeof(uint16_t) *
					       offset_index)));
				offset_index++;

				ticks_to_expire_prev +=
					HAL_TICKER_US_TO_TICKS(
						CONN_INT_UNIT_US);
			}

			*ticks_to_offset_next = ticks_to_expire_prev;
		}

		if (is_conn_interval) {
			is_after_conn_interval = true;
		}

		ticks_slot_abs_curr += ticks_slot;

		ticks_to_expire_prev = ticks_to_expire_normal;
		ticks_slot_abs_prev = ticks_slot_abs_curr;
	} while (offset_index < *offset_max);

	if (ticker_id == TICKER_NULL) {
		if (ticks_to_expire_prev < *ticks_to_offset_next) {
			ticks_to_expire_prev = *ticks_to_offset_next;
		}

		while (offset_index < *offset_max) {
			offset = DIV_ROUND_UP(
				(ticks_to_expire_prev + ticks_slot_abs_prev),
				 HAL_TICKER_US_TO_TICKS(CONN_INT_UNIT_US));
			if (offset >= conn_interval) {
				if (!is_select || !is_after_conn_interval) {
					break;
				}

				while (offset >= conn_interval) {
					offset -= conn_interval;
				}
			}

			sys_put_le16(offset, (win_offset + (sizeof(uint16_t) *
							    offset_index)));
			offset_index++;

			ticks_to_expire_prev += HAL_TICKER_US_TO_TICKS(
							CONN_INT_UNIT_US);
		}

		*ticks_to_offset_next = ticks_to_expire_prev;
	}

	*offset_max = offset_index;
}
#endif /* CONFIG_BT_LL_SW_LLCP_LEGACY */
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

		ticks_to_expire_normal = ticks_to_expire;

#if defined(CONFIG_BT_CTLR_LOW_LAT)
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
		if (hdr->ticks_prepare_to_start & XON_BITMASK) {
			const uint32_t ticks_prepare_to_start =
				MAX(hdr->ticks_active_to_start,
				    hdr->ticks_preempt_to_start);

			ticks_slot_abs_curr = hdr->ticks_prepare_to_start &
					      ~XON_BITMASK;
			ticks_to_expire_normal -= ticks_slot_abs_curr -
						  ticks_prepare_to_start;
		} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
		{
			const uint32_t ticks_prepare_to_start =
				MAX(hdr->ticks_active_to_start,
				    hdr->ticks_prepare_to_start);

			ticks_slot_abs_curr = ticks_prepare_to_start;
		}
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
		if (conn && !conn->lll.role) {
			if (IS_ENABLED(CONFIG_BT_CTLR_CENTRAL_RESERVE_MAX)) {
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

			if (*ticks_slot < HAL_TICKER_US_TO_TICKS(CONFIG_BT_CTLR_CENTRAL_SPACING)) {
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
