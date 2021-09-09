/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <bluetooth/buf.h>
#include <sys/byteorder.h>

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

#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"
#include "ull_internal.h"

#include "ull_conn_internal.h"
#include "ull_conn_iso_internal.h"
#include "lll_peripheral_iso.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_peripheral_iso
#include "common/log.h"
#include "hal/debug.h"

uint8_t ll_cis_accept(uint16_t handle)
{
	struct ll_conn *acl_conn = NULL;

	for (int h = 0; h < CONFIG_BT_MAX_CONN; h++) {
		struct ll_conn *conn = ll_conn_get(h);

		if (handle == conn->llcp_cis.cis_handle) {
			/* ACL connection found */
			acl_conn = conn;
			break;
		}
	}

	if (!acl_conn) {
		BT_ERR("No connection found for handle %u", handle);
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if (acl_conn->lll.role == BT_CONN_ROLE_MASTER) {
		BT_ERR("Not allowed for central");
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	acl_conn->llcp_cis.req++;

	return 0;
}

uint8_t ll_cis_reject(uint16_t handle, uint8_t reason)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(reason);

	return BT_HCI_ERR_CMD_DISALLOWED;
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
		cig->lll.handle = 0xFFFF;
		cig->lll.role = acl->lll.role;

		ull_hdr_init(&cig->ull);
		lll_hdr_init(&cig->lll, cig);
	}

	if (cig->lll.num_cis == CONFIG_BT_CTLR_CONN_ISO_STREAMS_PER_GROUP) {
		/* No space in CIG for new CIS */
		return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
	}

	/* Acquire new CIS */
	cis = ll_conn_iso_stream_acquire();
	if (cis == NULL) {
		/* No space for new CIS */
		return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
	}

	cig->iso_interval   = sys_le16_to_cpu(req->iso_interval);
	cig->c_sdu_interval = sys_get_le24(req->c_sdu_interval);
	cig->p_sdu_interval = sys_get_le24(req->p_sdu_interval);

	cis->cis_id = req->cis_id;
	cis->established = 0;
	cis->group = cig;
	cis->teardown = 0;
	cis->released_cb = NULL;

	cis->lll.handle = 0xFFFF;
	cis->lll.acl_handle = acl->lll.handle;
	cis->lll.sub_interval = sys_get_le24(req->sub_interval);
	cis->lll.num_subevents = req->nse;
	cis->lll.next_subevent = 0;
	cis->lll.sn = 0;
	cis->lll.nesn = 0;
	cis->lll.cie = 0;

	cis->lll.rx.phy = req->c_phy;
	cis->lll.rx.burst_number = req->c_bn;
	cis->lll.rx.flush_timeout = req->c_ft;
	cis->lll.rx.max_octets = sys_le16_to_cpu(req->c_max_pdu);

	cis->lll.tx.phy = req->p_phy;
	cis->lll.tx.burst_number = req->p_bn;
	cis->lll.tx.flush_timeout = req->p_ft;
	cis->lll.tx.max_octets = sys_le16_to_cpu(req->p_max_pdu);

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

static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, uint8_t force, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = { 0, 0, &link, NULL,
				     lll_peripheral_iso_prepare };
	static struct lll_prepare_param p;
	struct ll_conn_iso_group *cig;
	struct ll_conn_iso_stream *cis;
	uint16_t handle_iter;
	uint32_t err;
	uint8_t ref;

	cig = param;

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
		}
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
}

static void ticker_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

void ull_peripheral_iso_start(struct ll_conn *acl, uint32_t ticks_at_expire)
{
	struct ll_conn_iso_group *cig;
	struct ll_conn_iso_stream *cis;
	uint32_t acl_to_cig_ref_point;
	uint32_t cis_offs_to_cig_ref;
	uint32_t ready_delay_us;
	uint32_t ticks_interval;
	uint32_t ticker_status;
	int32_t cig_offset_us;
	uint16_t handle_iter;
	uint16_t cis_handle;
	uint8_t ticker_id;

	cis_handle = acl->llcp_cis.cis_handle;

	cig = ll_conn_iso_group_get_by_id(acl->llcp_cis.cig_id);
	cis = ll_conn_iso_stream_get(cis_handle);

	cis_offs_to_cig_ref = cig->sync_delay - cis->sync_delay;
	handle_iter = UINT16_MAX;

	cis->lll.offset = cis_offs_to_cig_ref;
	cis->lll.handle = cis_handle;

	/* Check if another CIS was already started and CIG ticker is
	 * running. If so, we just return with updated offset and
	 * validated handle.
	 */
	for (int i = 0; i < cig->lll.num_cis; i++) {
		struct ll_conn_iso_stream *other_cis;

		other_cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);
		LL_ASSERT(other_cis);

		if (other_cis == cis || other_cis->lll.handle == 0xFFFF) {
			/* Same CIS or not valid - skip */
			continue;
		}

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

	cig_offset_us  = acl_to_cig_ref_point;
	cig_offset_us -= EVENT_OVERHEAD_START_US;
	cig_offset_us -= EVENT_TICKER_RES_MARGIN_US;
	cig_offset_us -= EVENT_JITTER_US;
	cig_offset_us -= ready_delay_us;

	/* Make sure we have time to service first subevent. TODO: Improve
	 * by skipping <n> interval(s) and incrementing event_count.
	 */
	LL_ASSERT(cig_offset_us > 0);

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
}
