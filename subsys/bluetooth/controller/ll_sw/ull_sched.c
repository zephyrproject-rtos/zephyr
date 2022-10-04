/*
 * Copyright (c) 2018-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>

#include <zephyr/sys/byteorder.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"
#include "util/mem.h"

#include "ticker/ticker.h"

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

#if !defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
#include "ull_tx_queue.h"
#endif

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_conn_types.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_conn_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_sched
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
static void win_offset_calc(struct ll_conn *conn_curr, uint8_t is_select,
			    uint32_t *ticks_to_offset_next,
			    uint16_t conn_interval, uint8_t *offset_max,
			    uint8_t *win_offset);
#endif
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CONN)
static void after_cen_offset_get(uint16_t conn_interval, uint32_t ticks_slot,
				 uint32_t ticks_anchor,
				 uint32_t *win_offset_us);
#endif /* CONFIG_BT_CONN */

typedef struct ull_hdr *(*ull_hdr_get_func)(uint8_t ticker_id,
					    uint32_t *ticks_slot);
static uint8_t after_match_slot_get(uint8_t user_id, uint32_t ticks_slot_abs,
				    ticker_op_match_func ticker_match_op_cb,
				    ull_hdr_get_func ull_hdr_get_cb,
				    uint32_t *ticks_anchor,
				    uint32_t *ticks_to_expire_match,
				    uint32_t *ticks_slot_match);
static void ticker_op_cb(uint32_t status, void *param);
static bool ticker_match_op_cb(uint8_t ticker_id, uint32_t ticks_slot,
			       uint32_t ticks_to_expire, void *op_context);
static struct ull_hdr *ull_hdr_get_cb(uint8_t ticker_id, uint32_t *ticks_slot);

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_BROADCASTER)
int ull_sched_adv_aux_sync_free_slot_get(uint8_t user_id,
					 uint32_t ticks_slot_abs,
					 uint32_t *ticks_anchor)
{
	uint32_t ticks_to_expire;
	uint32_t ticks_slot;
	uint8_t ticker_id;

	ticker_id = after_match_slot_get(user_id, ticks_slot_abs,
					 ticker_match_op_cb, ull_hdr_get_cb,
					 ticks_anchor, &ticks_to_expire,
					 &ticks_slot);
	if (ticker_id != TICKER_NULL) {
		if (false) {

		} else if (IN_RANGE(ticker_id, TICKER_ID_ADV_AUX_BASE,
				    TICKER_ID_ADV_AUX_LAST)) {
			const struct ll_adv_aux_set *aux;

			aux = (void *)ull_hdr_get_cb(ticker_id, &ticks_slot);

			*ticks_anchor += ticks_to_expire;
			*ticks_anchor += ticks_slot;

			if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
				*ticks_anchor +=
					MAX(aux->ull.ticks_active_to_start,
					    aux->ull.ticks_prepare_to_start);
			}

			return 0;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
		} else if (IN_RANGE(ticker_id, TICKER_ID_ADV_SYNC_BASE,
				    TICKER_ID_ADV_SYNC_LAST)) {
			const struct ll_adv_sync_set *sync;

			sync = (void *)ull_hdr_get_cb(ticker_id, &ticks_slot);

			*ticks_anchor += ticks_to_expire;
			*ticks_anchor += ticks_slot;

			if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
				*ticks_anchor +=
					MAX(sync->ull.ticks_active_to_start,
					    sync->ull.ticks_prepare_to_start);
			}

			return 0;
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

#if defined(CONFIG_BT_CONN)
		} else if (IN_RANGE(ticker_id, TICKER_ID_CONN_BASE,
				    TICKER_ID_CONN_LAST)) {
			*ticks_anchor += ticks_to_expire;
			*ticks_anchor += ticks_slot;

			return 0;
#endif /* CONFIG_BT_CONN */

		}
	}

	return -ECHILD;
}
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_CONN)
int ull_sched_after_cen_slot_get(uint8_t user_id, uint32_t ticks_slot_abs,
				 uint32_t *ticks_anchor, uint32_t *us_offset)
{
	uint32_t ticks_to_expire;
	uint32_t ticks_slot;
	uint8_t ticker_id;

	ticker_id = after_match_slot_get(user_id, ticks_slot_abs,
					 ticker_match_op_cb, ull_hdr_get_cb,
					 ticks_anchor, &ticks_to_expire,
					 &ticks_slot);
	if (ticker_id != TICKER_NULL) {
		*us_offset = HAL_TICKER_TICKS_TO_US(ticks_to_expire +
						    ticks_slot) +
						    (EVENT_JITTER_US << 3);
		return 0;
	}

	return -ECHILD;
}

#if defined(CONFIG_BT_CENTRAL)
void ull_sched_mfy_after_cen_offset_get(void *param)
{
	struct lll_prepare_param *p = param;
	struct lll_scan *lll = p->param;
	uint32_t ticks_slot_overhead;
	struct ll_conn *conn;

	conn = HDR_LLL2ULL(lll->conn);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(conn->ull.ticks_active_to_start,
					  conn->ull.ticks_prepare_to_start);
	} else {
		ticks_slot_overhead = 0U;
	}

	after_cen_offset_get(lll->conn->interval,
			     (ticks_slot_overhead + conn->ull.ticks_slot),
			     p->ticks_at_expire, &lll->conn_win_offset_us);
}
#endif /* CONFIG_BT_CENTRAL */

void ull_sched_mfy_win_offset_use(void *param)
{
	struct ll_conn *conn = param;
	uint32_t ticks_slot_overhead;

	/*
	 * TODO: update when updating the connection update procedure
	 */
#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
	uint16_t win_offset;
#endif

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(conn->ull.ticks_active_to_start,
					  conn->ull.ticks_prepare_to_start);
	} else {
		ticks_slot_overhead = 0U;
	}

	/*
	 * TODO: update when updating the connection update procedure
	 */
#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
	after_cen_offset_get(conn->lll.interval,
			     (ticks_slot_overhead + conn->ull.ticks_slot),
			     conn->llcp.conn_upd.ticks_anchor,
			     &conn->llcp_cu.win_offset_us);

	win_offset = conn->llcp_cu.win_offset_us / CONN_INT_UNIT_US;

	sys_put_le16(win_offset, (void *)conn->llcp.conn_upd.pdu_win_offset);

	/* move to offset calculated state */
	conn->llcp_cu.state = LLCP_CUI_STATE_OFFS_RDY;
#endif
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
#endif

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
#define OFFSET_S_MAX 6
#define OFFSET_M_MAX 6

#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
	uint16_t win_offset_m[OFFSET_M_MAX] = {0, };
	uint8_t offset_m_max = OFFSET_M_MAX;
	struct ll_conn *conn = param;
	uint8_t offset_index_s = 0U;
	uint8_t has_offset_s = 0U;
	uint16_t win_offset_s;
	uint32_t ticks_to_offset;
#endif

	/*
	 * TODO: update when updating the connection update procedure
	 */
#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)

	ticks_to_offset = HAL_TICKER_US_TO_TICKS(conn->llcp_conn_param.offset0 *
						 CONN_INT_UNIT_US);

	win_offset_calc(conn, 1, &ticks_to_offset,
			conn->llcp_conn_param.interval_max, &offset_m_max,
			(void *)win_offset_m);

	while (offset_index_s < OFFSET_S_MAX) {
		uint8_t offset_index_m = 0U;

		win_offset_s =
			sys_get_le16((uint8_t *)&conn->llcp_conn_param.offset0 +
				     (sizeof(uint16_t) * offset_index_s));

		while (offset_index_m < offset_m_max) {
			if (win_offset_s != 0xffff) {
				if (win_offset_s ==
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
		/* move to offset calculated state */
		conn->llcp_cu.state = LLCP_CUI_STATE_OFFS_RDY;
	} else if (!has_offset_s) {
		conn->llcp_cu.win_offset_us = win_offset_m[0] *
					      CONN_INT_UNIT_US;
		sys_put_le16(win_offset_m[0],
			     (void *)conn->llcp.conn_upd.pdu_win_offset);
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
#endif /* CONFIG_BT_LL_SW_LLCP_LEGACY */


#undef OFFSET_S_MAX
#undef OFFSET_M_MAX
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
	uint8_t ticker_id_other;
	uint8_t ticker_id_prev;
	uint32_t ticks_anchor;
	uint8_t offset_index;
	uint8_t ticker_id;
	uint16_t offset;

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

	ticker_id = ticker_id_prev = ticker_id_other = TICKER_NULL;
	ticks_to_expire = ticks_to_expire_prev = ticks_anchor =
		ticks_anchor_prev = offset_index = offset = 0U;
	ticks_slot_abs_prev = 0U;
	do {
		uint32_t volatile ret_cb;
		struct ll_conn *conn;
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

		/* consider advertiser time as available. Any other time used by
		 * tickers declared outside the controller is also available.
		 */
#if defined(CONFIG_BT_BROADCASTER)
		if ((ticker_id < TICKER_ID_ADV_BASE) ||
		    (ticker_id > TICKER_ID_CONN_LAST))
#else /* !CONFIG_BT_BROADCASTER */
		if ((ticker_id < TICKER_ID_SCAN_BASE) ||
		    (ticker_id > TICKER_ID_CONN_LAST))
#endif /* !CONFIG_BT_BROADCASTER */
		{
			continue;
		}

		if (ticker_id < TICKER_ID_CONN_BASE) {
			/* non conn role found which could have preempted a
			 * conn role, hence do not consider this free space
			 * and any further as free slot for offset,
			 */
			ticker_id_other = ticker_id;
			continue;
		}

		/* TODO: handle scanner; for now we exit with as much we
		 * where able to fill (offsets).
		 */
		if (ticker_id_other != TICKER_NULL) {
			break;
		}

		conn = ll_conn_get(ticker_id - TICKER_ID_CONN_BASE);
		if ((conn != conn_curr) && (is_select || !conn->lll.role)) {
			uint32_t ticks_to_expire_normal =
				ticks_to_expire + ticks_prepare_reduced;
			uint32_t ticks_slot_margin = 0U;
			uint32_t ticks_slot_abs_curr = 0U;
#if defined(CONFIG_BT_CTLR_LOW_LAT)
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
			if (conn->ull.ticks_prepare_to_start & XON_BITMASK) {
				uint32_t ticks_prepare_to_start =
					MAX(conn->ull.ticks_active_to_start,
					    conn->ull.ticks_preempt_to_start);

				ticks_slot_abs_curr =
					conn->ull.ticks_prepare_to_start &
					~XON_BITMASK;
				ticks_to_expire_normal -=
					ticks_slot_abs_curr -
					ticks_prepare_to_start;
			} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
			{
				uint32_t ticks_prepare_to_start =
					MAX(conn->ull.ticks_active_to_start,
					    conn->ull.ticks_prepare_to_start);

				ticks_slot_abs_curr = ticks_prepare_to_start;
			}
#endif

			ticks_slot_abs_curr += conn->ull.ticks_slot +
				HAL_TICKER_US_TO_TICKS(CONN_INT_UNIT_US);

			if (conn->lll.role) {
				ticks_slot_margin =
					HAL_TICKER_US_TO_TICKS(EVENT_TIES_US);
				ticks_slot_abs_curr += ticks_slot_margin;
			}

			if (*ticks_to_offset_next < ticks_to_expire_normal) {
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
					offset = (ticks_to_expire_prev +
						  ticks_slot_abs_prev) /
						 HAL_TICKER_US_TO_TICKS(
							CONN_INT_UNIT_US);
					if (offset >= conn_interval) {
						ticks_to_expire_prev = 0U;

						break;
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

				if (offset >= conn_interval) {
					break;
				}
			}

			ticks_anchor_prev = ticks_anchor;
			ticker_id_prev = ticker_id;
			ticks_to_expire_prev = ticks_to_expire_normal;
			ticks_slot_abs_prev = ticks_slot_abs_curr;
		}
	} while (offset_index < *offset_max);

	if (ticker_id == TICKER_NULL) {
		if (ticks_to_expire_prev < *ticks_to_offset_next) {
			ticks_to_expire_prev = *ticks_to_offset_next;
		}

		while (offset_index < *offset_max) {
			offset = (ticks_to_expire_prev + ticks_slot_abs_prev) /
				 HAL_TICKER_US_TO_TICKS(CONN_INT_UNIT_US);
			if (offset >= conn_interval) {
				ticks_to_expire_prev = 0U;

				break;
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
#endif /* CONFIG_BT_CONN */

static uint8_t after_match_slot_get(uint8_t user_id, uint32_t ticks_slot_abs,
				    ticker_op_match_func ticker_match_op_cb,
				    ull_hdr_get_func ull_hdr_get_cb,
				    uint32_t *ticks_anchor,
				    uint32_t *ticks_to_expire_match,
				    uint32_t *ticks_slot_match)
{
	uint32_t ticks_to_expire_prev;
	uint32_t ticks_slot_abs_prev;
	uint32_t ticks_anchor_prev;
	uint32_t ticks_to_expire;
	uint8_t ticker_id_prev;
	uint8_t ticker_id;
	uint8_t retry;

	/* As we may place the event between two other events, ensure there is
	 * space for 3 times +/- 16 us jitter. I.e. with 32KHz sleep clock,
	 * ~30.517 us after previous event, ~30.517 us before and after current
	 * event, and an ~30.517 us before next event. Hence 8 time of ceil
	 * value 16 us (30.517 / 2).
	 */
	ticks_slot_abs += HAL_TICKER_US_TO_TICKS(EVENT_JITTER_US << 3);

	/* There is a possibility that ticker nodes expire during iterations in
	 * this function causing the reference ticks_anchor returned for the
	 * found ticker to change. In this case the iterations have to be
	 * restarted with the new reference ticks_anchor value.
	 * Simultaneous continuous scanning on 1M and Coded PHY, alongwith
	 * directed advertising and one other state/role could expire in quick
	 * succession, hence have a retry count of 4.
	 */
	retry = 4U;

	/* Initialize variable required for iterations to find a free slot */
	ticker_id = ticker_id_prev = TICKER_NULL;
	ticks_anchor_prev = 0U;
	ticks_to_expire = ticks_to_expire_prev = 0U;
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
					       &ticks_to_expire,
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

		hdr = ull_hdr_get_cb(ticker_id, &ticks_slot);
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
		ticks_slot_abs_prev = ticks_slot_abs_curr;
	}

	if (ticker_id_prev != TICKER_NULL) {
		*ticks_to_expire_match = ticks_to_expire_prev;
		*ticks_slot_match = ticks_slot_abs_prev;
	}

	return ticker_id_prev;
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}

static bool ticker_match_op_cb(uint8_t ticker_id, uint32_t ticks_slot,
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
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_CONN)
	       IN_RANGE(ticker_id, TICKER_ID_CONN_BASE,
			TICKER_ID_CONN_LAST) ||
#endif /* CONFIG_BT_CONN */

	       false;
}

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
				*ticks_slot = HAL_TICKER_US_TO_TICKS(time_us);
			} else {
				*ticks_slot = aux->ull.ticks_slot;
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
				*ticks_slot = HAL_TICKER_US_TO_TICKS(time_us);
			} else {
				*ticks_slot = sync->ull.ticks_slot;
			}

			return &sync->ull;
		}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_CONN)
	} else if (IN_RANGE(ticker_id, TICKER_ID_CONN_BASE,
			    TICKER_ID_CONN_LAST)) {
		struct ll_conn *conn;

		conn = ll_conn_get(ticker_id - TICKER_ID_CONN_BASE);
		if (conn && !conn->lll.role) {
			*ticks_slot = conn->ull.ticks_slot;

			return &conn->ull;
		}
#endif /* CONFIG_BT_CONN */

	}

	return NULL;
}
