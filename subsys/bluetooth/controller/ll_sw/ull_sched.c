/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <sys/byteorder.h>

#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll_scan.h"
#include "lll_conn.h"

#include "ull_scan_types.h"
#include "ull_conn_types.h"

#include "ull_internal.h"
#include "ull_conn_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_sched
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void win_offset_calc(struct ll_conn *conn_curr, uint8_t is_select,
			    uint32_t *ticks_to_offset_next,
			    uint16_t conn_interval, uint8_t *offset_max,
			    uint8_t *win_offset);
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */
static void after_mstr_offset_get(uint16_t conn_interval, uint32_t ticks_slot,
				  uint32_t ticks_anchor,
				  uint32_t *win_offset_us);
static void ticker_op_cb(uint32_t status, void *param);

void ull_sched_after_mstr_slot_get(uint8_t user_id, uint32_t ticks_slot_abs,
				   uint32_t *ticks_anchor, uint32_t *us_offset)
{
	uint32_t ticks_to_expire_prev;
	uint32_t ticks_slot_abs_prev;
	uint32_t ticks_to_expire;
	uint8_t ticker_id_prev;
	uint8_t ticker_id;

	ticks_slot_abs += HAL_TICKER_US_TO_TICKS(EVENT_JITTER_US << 3);

	ticker_id = ticker_id_prev = 0xff;
	ticks_to_expire = ticks_to_expire_prev = *us_offset = 0U;
	ticks_slot_abs_prev = 0U;
	while (1) {
		uint32_t volatile ret_cb;
		struct ll_conn *conn;
		uint32_t ret;

		ret_cb = TICKER_STATUS_BUSY;
		ret = ticker_next_slot_get(TICKER_INSTANCE_ID_CTLR, user_id,
					   &ticker_id, ticks_anchor,
					   &ticks_to_expire, ticker_op_cb,
					   (void *)&ret_cb);
		if (ret == TICKER_STATUS_BUSY) {
			while (ret_cb == TICKER_STATUS_BUSY) {
				ticker_job_sched(TICKER_INSTANCE_ID_CTLR,
						 user_id);
			}
		}

		LL_ASSERT(ret_cb == TICKER_STATUS_SUCCESS);

		if (ticker_id == 0xff) {
			break;
		}

		if ((ticker_id < TICKER_ID_CONN_BASE) ||
		    (ticker_id > TICKER_ID_CONN_LAST)) {
			continue;
		}

		conn = ll_conn_get(ticker_id - TICKER_ID_CONN_BASE);
		if (conn && !conn->lll.role) {
			uint32_t ticks_to_expire_normal = ticks_to_expire;
			uint32_t ticks_slot_abs_curr = 0;
#if defined(CONFIG_BT_CTLR_LOW_LAT)
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
			if (conn->evt.ticks_xtal_to_start & XON_BITMASK) {
				uint32_t ticks_prepare_to_start =
					MAX(conn->evt.ticks_active_to_start,
					    conn->evt.ticks_preempt_to_start);

				ticks_slot_abs_curr =
					conn->evt.ticks_xtal_to_start &
					~XON_BITMASK;
				ticks_to_expire_normal -=
					ticks_slot_abs_curr -
					ticks_prepare_to_start;
			} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
			{
				uint32_t ticks_prepare_to_start =
					MAX(conn->evt.ticks_active_to_start,
					    conn->evt.ticks_xtal_to_start);

				ticks_slot_abs_curr = ticks_prepare_to_start;
			}
#endif

			ticks_slot_abs_curr += conn->evt.ticks_slot;

			if ((ticker_id_prev != 0xff) &&
			    (ticker_ticks_diff_get(ticks_to_expire_normal,
						   ticks_to_expire_prev) >
			     (ticks_slot_abs_prev + ticks_slot_abs))) {
				break;
			}

			ticker_id_prev = ticker_id;
			ticks_to_expire_prev = ticks_to_expire_normal;
			ticks_slot_abs_prev = ticks_slot_abs_curr;
		}
	}

	if (ticker_id_prev != 0xff) {
		*us_offset = HAL_TICKER_TICKS_TO_US(ticks_to_expire_prev +
						    ticks_slot_abs_prev) +
						    (EVENT_JITTER_US << 3);
	}
}

#if defined(CONFIG_BT_CENTRAL)
void ull_sched_mfy_after_mstr_offset_get(void *param)
{
	struct lll_prepare_param *p = param;
	struct lll_scan *lll = p->param;
	struct evt_hdr *conn_evt = HDR_LLL2EVT(lll->conn);
	uint32_t ticks_slot_overhead;

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(conn_evt->ticks_active_to_start,
					  conn_evt->ticks_xtal_to_start);
	} else {
		ticks_slot_overhead = 0U;
	}

	after_mstr_offset_get(lll->conn->interval,
			      (ticks_slot_overhead + conn_evt->ticks_slot),
			      p->ticks_at_expire, &lll->conn_win_offset_us);
}
#endif /* CONFIG_BT_CENTRAL */

void ull_sched_mfy_win_offset_use(void *param)
{
	struct ll_conn *conn = param;
	uint32_t ticks_slot_overhead;
	uint16_t win_offset;

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(conn->evt.ticks_active_to_start,
					  conn->evt.ticks_xtal_to_start);
	} else {
		ticks_slot_overhead = 0U;
	}

	after_mstr_offset_get(conn->lll.interval,
			      (ticks_slot_overhead + conn->evt.ticks_slot),
			      conn->llcp.conn_upd.ticks_anchor,
			      &conn->llcp_cu.win_offset_us);

	win_offset = conn->llcp_cu.win_offset_us / CONN_INT_UNIT_US;

	sys_put_le16(win_offset, (void *)conn->llcp.conn_upd.pdu_win_offset);

	/* move to offset calculated state */
	conn->llcp_cu.state = LLCP_CUI_STATE_OFFS_RDY;
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
void ull_sched_mfy_free_win_offset_calc(void *param)
{
	uint32_t ticks_to_offset_default = 0U;
	uint32_t *ticks_to_offset_next;
	struct ll_conn *conn = param;
	uint8_t offset_max = 6U;

	ticks_to_offset_next = &ticks_to_offset_default;

#if defined(CONFIG_BT_PERIPHERAL)
	if (conn->lll.role) {
		conn->llcp_conn_param.ticks_to_offset_next =
			conn->slave.ticks_to_offset;

		ticks_to_offset_next =
			&conn->llcp_conn_param.ticks_to_offset_next;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	win_offset_calc(conn, 0, ticks_to_offset_next,
			conn->llcp_conn_param.interval_max, &offset_max,
			(void *)conn->llcp_conn_param.pdu_win_offset0);

	/* move to offset calculated state */
	conn->llcp_conn_param.state = LLCP_CPR_STATE_OFFS_RDY;
}

void ull_sched_mfy_win_offset_select(void *param)
{
#define OFFSET_S_MAX 6
#define OFFSET_M_MAX 6
	uint16_t win_offset_m[OFFSET_M_MAX] = {0, };
	uint8_t offset_m_max = OFFSET_M_MAX;
	struct ll_conn *conn = param;
	uint8_t offset_index_s = 0U;
	uint8_t has_offset_s = 0U;
	uint32_t ticks_to_offset;
	uint16_t win_offset_s;

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
#undef OFFSET_S_MAX
#undef OFFSET_M_MAX
}

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
	if (conn_curr->evt.ticks_xtal_to_start & XON_BITMASK) {
		uint32_t ticks_prepare_to_start =
			MAX(conn_curr->evt.ticks_active_to_start,
			    conn_curr->evt.ticks_preempt_to_start);

		ticks_slot_abs = conn_curr->evt.ticks_xtal_to_start &
				 ~XON_BITMASK;
		ticks_prepare_reduced = ticks_slot_abs - ticks_prepare_to_start;
	} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	{
		uint32_t ticks_prepare_to_start =
			MAX(conn_curr->evt.ticks_active_to_start,
			    conn_curr->evt.ticks_xtal_to_start);

		ticks_slot_abs = ticks_prepare_to_start;
	}
#endif

	ticks_slot_abs += conn_curr->evt.ticks_slot;

	if (conn_curr->lll.role) {
		ticks_slot_abs += HAL_TICKER_US_TO_TICKS(EVENT_TIES_US);
	}

	ticker_id = ticker_id_prev = ticker_id_other = 0xff;
	ticks_to_expire = ticks_to_expire_prev = ticks_anchor =
		ticks_anchor_prev = offset_index = offset = 0U;
	ticks_slot_abs_prev = 0U;
	do {
		uint32_t volatile ret_cb;
		struct ll_conn *conn;
		uint32_t ret;

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

		LL_ASSERT(ret_cb == TICKER_STATUS_SUCCESS);

		if (ticker_id == 0xff) {
			break;
		}

		/* ticks_anchor shall not change during this loop */
		if ((ticker_id_prev != 0xff) &&
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
		if (ticker_id_other != 0xff) {
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
			if (conn->evt.ticks_xtal_to_start & XON_BITMASK) {
				uint32_t ticks_prepare_to_start =
					MAX(conn->evt.ticks_active_to_start,
					    conn->evt.ticks_preempt_to_start);

				ticks_slot_abs_curr =
					conn->evt.ticks_xtal_to_start &
					~XON_BITMASK;
				ticks_to_expire_normal -=
					ticks_slot_abs_curr -
					ticks_prepare_to_start;
			} else
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
			{
				uint32_t ticks_prepare_to_start =
					MAX(conn->evt.ticks_active_to_start,
					    conn->evt.ticks_xtal_to_start);

				ticks_slot_abs_curr = ticks_prepare_to_start;
			}
#endif

			ticks_slot_abs_curr += conn->evt.ticks_slot +
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

	if (ticker_id == 0xff) {
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
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

static void after_mstr_offset_get(uint16_t conn_interval, uint32_t ticks_slot,
				  uint32_t ticks_anchor,
				  uint32_t *win_offset_us)
{
	uint32_t ticks_anchor_offset = ticks_anchor;

	ull_sched_after_mstr_slot_get(TICKER_USER_ID_ULL_LOW, ticks_slot,
				      &ticks_anchor_offset, win_offset_us);

	if (!*win_offset_us) {
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

	if ((*win_offset_us & BIT(31)) == 0) {
		uint32_t conn_interval_us = conn_interval * CONN_INT_UNIT_US;

		while (*win_offset_us > conn_interval_us) {
			*win_offset_us -= conn_interval_us;
		}
	}
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}
