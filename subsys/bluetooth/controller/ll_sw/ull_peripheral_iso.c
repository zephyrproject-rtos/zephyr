/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/sys/byteorder.h>

#include "util/memq.h"
#include "util/mayfly.h"

#include "hal/ccm.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "isoal.h"
#include "ull_iso_types.h"

#if !defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
#include "ull_tx_queue.h"
#endif

#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"
#include "ull_internal.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"

#include "ull_conn_internal.h"
#include "ull_conn_iso_internal.h"
#include "lll_peripheral_iso.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_peripheral_iso
#include "common/log.h"
#include "hal/debug.h"

static struct ll_conn *ll_cis_get_acl_awaiting_reply(uint16_t handle, uint8_t *error)
{
	struct ll_conn *acl_conn = NULL;

	if (!IS_CIS_HANDLE(handle) || ll_conn_iso_stream_get(handle)->group == NULL) {
		BT_ERR("Unknown CIS handle %u", handle);
		*error = BT_HCI_ERR_UNKNOWN_CONN_ID;
		return NULL;
	}

	for (int h = 0; h < CONFIG_BT_MAX_CONN; h++) {
		struct ll_conn *conn = ll_conn_get(h);
#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
		uint16_t cis_handle = conn->llcp_cis.cis_handle;
#else
		uint16_t cis_handle = ull_cp_cc_ongoing_handle(conn);
#endif

		if (handle == cis_handle) {
			/* ACL connection found */
			acl_conn = conn;
			break;
		}
	}

	if (!acl_conn) {
		BT_ERR("No connection found for handle %u", handle);
		*error = BT_HCI_ERR_CMD_DISALLOWED;
		return NULL;
	}

	if (acl_conn->lll.role == BT_CONN_ROLE_CENTRAL) {
		BT_ERR("Not allowed for central");
		*error = BT_HCI_ERR_CMD_DISALLOWED;
		return NULL;
	}

#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
	if (acl_conn->llcp_cis.state != LLCP_CIS_STATE_RSP_WAIT) {
#else
	if (!ull_cp_cc_awaiting_reply(acl_conn)) {
#endif
		BT_ERR("Not allowed in current procedure state");
		*error = BT_HCI_ERR_CMD_DISALLOWED;
		return NULL;
	}

	return acl_conn;
}

uint8_t ll_cis_accept(uint16_t handle)
{
	uint8_t status = BT_HCI_ERR_SUCCESS;
	struct ll_conn *acl_conn = ll_cis_get_acl_awaiting_reply(handle, &status);

	if (acl_conn) {
		/* Accept request */
#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
		acl_conn->llcp_cis.req++;
#else
		ull_cp_cc_accept(acl_conn);
#endif
	}

	return status;
}

uint8_t ll_cis_reject(uint16_t handle, uint8_t reason)
{
	uint8_t status = BT_HCI_ERR_SUCCESS;

#if defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
	status = BT_HCI_ERR_CMD_DISALLOWED;
#else
	struct ll_conn *acl_conn = ll_cis_get_acl_awaiting_reply(handle, &status);

	if (acl_conn) {
		/* Accept request */
		ull_cp_cc_reject(acl_conn, reason);
	}
#endif

	return status;
}

int ull_peripheral_iso_init(void)
{
	return 0;
}

int ull_peripheral_iso_reset(void)
{
	return 0;
}

uint8_t ull_peripheral_iso_acquire(struct ll_conn *acl,
				   struct pdu_data_llctrl_cis_req *req,
				   uint16_t *cis_handle)
{
	struct ll_conn_iso_group *cig;
	struct ll_conn_iso_stream *cis;
	uint16_t handle;

	/* Get CIG by id */
	cig = ll_conn_iso_group_get_by_id(req->cig_id);
	if (!cig) {
		/* CIG does not exist - create it */
		cig = ll_conn_iso_group_acquire();
		if (!cig) {
			/* No space for new CIG */
			return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
		}

		memset(&cig->lll, 0, sizeof(cig->lll));

		cig->cig_id = req->cig_id;
		cig->lll.handle = LLL_HANDLE_INVALID;
		cig->lll.role = acl->lll.role;
		cig->lll.resume_cis = LLL_HANDLE_INVALID;

		ull_hdr_init(&cig->ull);
		lll_hdr_init(&cig->lll, cig);
	}

	if (cig->lll.num_cis == CONFIG_BT_CTLR_CONN_ISO_STREAMS_PER_GROUP) {
		/* No space in CIG for new CIS */
		return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
	}

	for (handle = LL_CIS_HANDLE_BASE; handle <= LAST_VALID_CIS_HANDLE; handle++) {
		cis = ll_iso_stream_connected_get(handle);
		if (cis && cis->group && cis->cis_id == req->cis_id) {
			/* CIS ID already in use */
			return BT_HCI_ERR_INVALID_LL_PARAM;
		}
	}

	/* Acquire new CIS */
	cis = ll_conn_iso_stream_acquire();
	if (cis == NULL) {
		/* No space for new CIS */
		return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
	}

	cig->iso_interval = sys_le16_to_cpu(req->iso_interval);
	/* Read 20-bit SDU intervals (mask away RFU bits) */
	cig->c_sdu_interval = sys_get_le24(req->c_sdu_interval) & 0x0FFFFF;
	cig->p_sdu_interval = sys_get_le24(req->p_sdu_interval) & 0x0FFFFF;

	cis->cis_id = req->cis_id;
	cis->framed = (req->c_max_sdu_packed[1] & BIT(7)) >> 7;
	cis->established = 0;
	cis->group = cig;
	cis->teardown = 0;
	cis->released_cb = NULL;
	cis->c_max_sdu = (uint16_t)(req->c_max_sdu_packed[1] & 0x0F) << 8 |
				    req->c_max_sdu_packed[0];
	cis->p_max_sdu = (uint16_t)(req->p_max_sdu[1] & 0x0F) << 8 |
				    req->p_max_sdu[0];

	cis->lll.handle = 0xFFFF;
	cis->lll.acl_handle = acl->lll.handle;
	cis->lll.sub_interval = sys_get_le24(req->sub_interval);
	cis->lll.num_subevents = req->nse;
	cis->lll.next_subevent = 0;
	cis->lll.sn = 0;
	cis->lll.nesn = 0;
	cis->lll.cie = 0;
	cis->lll.flushed = 0;
	cis->lll.datapath_ready_rx = 0;

	cis->lll.rx.phy = req->c_phy;
	cis->lll.rx.burst_number = req->c_bn;
	cis->lll.rx.flush_timeout = req->c_ft;
	cis->lll.rx.max_octets = sys_le16_to_cpu(req->c_max_pdu);
	cis->lll.rx.payload_number = 0;

	cis->lll.tx.phy = req->p_phy;
	cis->lll.tx.burst_number = req->p_bn;
	cis->lll.tx.flush_timeout = req->p_ft;
	cis->lll.tx.max_octets = sys_le16_to_cpu(req->p_max_pdu);
	cis->lll.tx.payload_number = 0;

	if (!cis->lll.link_tx_free) {
		cis->lll.link_tx_free = &cis->lll.link_tx;
	}

	memq_init(cis->lll.link_tx_free, &cis->lll.memq_tx.head,
		  &cis->lll.memq_tx.tail);
	cis->lll.link_tx_free = NULL;

	*cis_handle = ll_conn_iso_stream_handle_get(cis);
	cig->lll.num_cis++;

	return 0;
}

uint8_t ull_peripheral_iso_setup(struct pdu_data_llctrl_cis_ind *ind,
				 uint8_t cig_id, uint16_t cis_handle)
{
	struct ll_conn_iso_stream *cis = NULL;
	struct ll_conn_iso_group *cig;

	/* Get CIG by id */
	cig = ll_conn_iso_group_get_by_id(cig_id);
	if (!cig) {
		return BT_HCI_ERR_UNSPECIFIED;
	}

	cig->lll.handle = ll_conn_iso_group_handle_get(cig);
	cig->sync_delay = sys_get_le24(ind->cig_sync_delay);

	cis = ll_conn_iso_stream_get(cis_handle);
	if (!cis) {
		return BT_HCI_ERR_UNSPECIFIED;
	}

	cis->sync_delay = sys_get_le24(ind->cis_sync_delay);
	cis->offset = sys_get_le24(ind->cis_offset);
	cis->lll.event_count = -1;
	memcpy(cis->lll.access_addr, ind->aa, sizeof(ind->aa));

	return 0;
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = { 0, 0, &link, NULL,
				     lll_peripheral_iso_prepare };
	static struct lll_prepare_param p;
	struct ll_conn_iso_group *cig;
	struct ll_conn_iso_stream *cis;
	uint64_t leading_event_count;
	uint16_t handle_iter;
	uint32_t err;
	uint8_t ref;

	cig = param;
	leading_event_count = 0;

	/* Check if stopping ticker (on disconnection, race with ticker expiry)
	 */
	if (unlikely(cig->lll.handle == 0xFFFF)) {
		return;
	}

	handle_iter = UINT16_MAX;

	/* Increment CIS event counters */
	for (int i = 0; i < cig->lll.num_cis; i++)  {
		cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);
		LL_ASSERT(cis);

		/* New CIS may become available by creation prior to the CIG
		 * event in which it has event_count == 0. Don't increment
		 * event count until its handle is validated in
		 * ull_peripheral_iso_start, which means that its ACL instant
		 * has been reached, and offset calculated.
		 */
		if (cis->lll.handle != 0xFFFF) {
			cis->lll.event_count++;


			leading_event_count = MAX(leading_event_count,
						cis->lll.event_count);

			ull_iso_lll_event_prepare(cis->lll.handle, cis->lll.event_count);
		}

		/* Latch datapath validity entering event */
		cis->lll.datapath_ready_rx = cis->hdr.datapath_out != NULL;
	}

	/* Update the CIG reference point for this event. Event 0 for the
	 * leading CIS in the CIG would have had it's reference point set in
	 * ull_peripheral_iso_start(). The reference point should only be
	 * updated from event 1 onwards. Although the cig reference point set
	 * this way is not accurate, it is the best possible until the anchor
	 * point for the leading CIS is available for this event.
	 */
	if (leading_event_count > 0) {
		cig->cig_ref_point += (cig->iso_interval * CONN_INT_UNIT_US);
	}

	/* Increment prepare reference count */
	ref = ull_ref_inc(&cig->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.param = &cig->lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	err = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!err);

	/* Handle ISO Transmit Test for this CIG */
	ull_conn_iso_transmit_test_cig_interval(cig->lll.handle, ticks_at_expire);
}

static void ticker_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

void ull_peripheral_iso_start(struct ll_conn *acl, uint32_t ticks_at_expire,
			      uint16_t cis_handle)
{
	struct ll_conn_iso_group *cig;
	struct ll_conn_iso_stream *cis;
	uint32_t acl_to_cig_ref_point;
	uint32_t cis_offs_to_cig_ref;
	uint32_t ready_delay_us;
	uint32_t ticks_interval;
	uint32_t ticker_status;
	int32_t cig_offset_us;
	uint8_t ticker_id;

	cis = ll_conn_iso_stream_get(cis_handle);
	cig = cis->group;

	cis_offs_to_cig_ref = cig->sync_delay - cis->sync_delay;

	cis->lll.offset = cis_offs_to_cig_ref;
	cis->lll.handle = cis_handle;

	/* Check if another CIS was already started and CIG ticker is
	 * running. If so, we just return with updated offset and
	 * validated handle.
	 */
	if (cig->started) {
		/* We're done */
		return;
	}

	ticker_id = TICKER_ID_CONN_ISO_BASE +
		    ll_conn_iso_group_handle_get(cig);
	ticks_interval = HAL_TICKER_US_TO_TICKS(cig->iso_interval *
						CONN_INT_UNIT_US);

	/* Establish the CIG reference point by adjusting ACL-to-CIS offset
	 * (cis->offset) by the difference between CIG- and CIS sync delays.
	 */
	acl_to_cig_ref_point = cis->offset - cis_offs_to_cig_ref;

#if defined(CONFIG_BT_CTLR_PHY)
	ready_delay_us = lll_radio_rx_ready_delay_get(acl->lll.phy_rx, 1);
#else
	ready_delay_us = lll_radio_rx_ready_delay_get(0, 0);
#endif

	/* Calculate initial ticker offset - we're one ACL interval early */
	cig_offset_us  = acl_to_cig_ref_point;
	cig_offset_us += (acl->lll.interval * CONN_INT_UNIT_US);
	cig_offset_us -= EVENT_OVERHEAD_START_US;
	cig_offset_us -= EVENT_TICKER_RES_MARGIN_US;
	cig_offset_us -= EVENT_JITTER_US;
	cig_offset_us -= ready_delay_us;

	/* Make sure we have time to service first subevent. TODO: Improve
	 * by skipping <n> interval(s) and incrementing event_count.
	 */
	LL_ASSERT(cig_offset_us > 0);

	/* Calculate the CIG reference point of first CIG event. This
	 * calculation is inaccurate. However it is the best estimate available
	 * until the first anchor point for the leading CIS is available.
	 */
	cig->cig_ref_point = HAL_TICKER_TICKS_TO_US(ticks_at_expire);
	cig->cig_ref_point += acl_to_cig_ref_point;
	cig->cig_ref_point += (acl->lll.interval * CONN_INT_UNIT_US);

	/* Start CIS peripheral CIG ticker */
	ticker_status = ticker_start(TICKER_INSTANCE_ID_CTLR,
				     TICKER_USER_ID_ULL_HIGH,
				     ticker_id,
				     ticks_at_expire,
				     HAL_TICKER_US_TO_TICKS(cig_offset_us),
				     ticks_interval,
				     HAL_TICKER_REMAINDER(ticks_interval),
				     TICKER_NULL_LAZY,
				     0,
				     ticker_cb, cig,
				     ticker_op_cb, NULL);

	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

	cig->started = 1;
}
