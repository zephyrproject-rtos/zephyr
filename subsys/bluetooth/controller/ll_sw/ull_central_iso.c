/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "util/memq.h"

#include "hal/ccm.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"
#include "ull_conn_types.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"

#include "ull_internal.h"
#include "ull_conn_iso_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_central_iso
#include "common/log.h"
#include "hal/debug.h"

uint8_t ll_cig_parameters_open(uint8_t cig_id,
			       uint32_t c_interval, uint32_t p_interval,
			       uint8_t sca, uint8_t packing, uint8_t framing,
			       uint16_t c_latency, uint16_t p_latency,
			       uint8_t num_cis)
{
	struct ll_conn_iso_group *cig;

	/* TODO: Verify parameters */

	if (num_cis >= CONFIG_BT_CTLR_CONN_ISO_STREAMS_PER_GROUP) {
		return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
	}

	cig = ll_conn_iso_group_get_by_id(cig_id);
	if (!cig) {
		cig = ll_conn_iso_group_acquire();
		if (!cig) {
			return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
		}

		cig->cig_id = cig_id;

		ull_hdr_init(&cig->ull);
		lll_hdr_init(&cig->lll, cig);
	} else {
		/* TODO: check if it is allowed to update the CIG parameters */
	}

	cig->c_sdu_interval = c_interval;
	cig->p_sdu_interval = p_interval;
	cig->c_latency = c_latency;
	cig->p_latency = p_latency;
	cig->started = 0U;

	cig->lll.handle = LLL_HANDLE_INVALID;
	cig->lll.role = BT_HCI_ROLE_CENTRAL;
	cig->lll.wc_sca = sca;
	cig->lll.packing = packing;
	cig->lll.framing = framing;
	cig->lll.num_cis = num_cis;

	/* TODO: Calculate ISO interval */

	return 0U;
}

uint8_t ll_cis_parameters_set(uint8_t cig_id, uint8_t cis_id,
			      uint16_t c_sdu, uint16_t p_sdu,
			      uint8_t c_phy, uint8_t p_phy,
			      uint8_t c_rtn, uint8_t p_rtn,
			      uint16_t *cis_handle)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;

	/* Get reference to CIG instance */
	cig = ll_conn_iso_group_get_by_id(cig_id);
	if (!cig) {
		/* TODO: Fix the return error code */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Get reference to existing CIS instance, if any */
	cis = NULL;
	for (uint16_t handle = LL_CIS_HANDLE_BASE; handle <= LL_CIS_HANDLE_LAST;
	     handle++) {
		struct ll_conn_iso_stream *iter_cis;

		iter_cis = ll_conn_iso_stream_get(handle);
		if (iter_cis && (iter_cis->group == cig) &&
		    (iter_cis->cis_id == cis_id)) {
			cis = iter_cis;

			break;
		}
	}

	/* Acquire new CIS */
	if (!cis) {
		cis = ll_conn_iso_stream_acquire();
		if (!cis) {
			return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
		}
	}

	cis->cis_id = cis_id;
	cis->group = cig;

	cis->c_max_sdu = c_sdu;
	cis->p_max_sdu = p_sdu;

	cis->established = 0U;
	cis->teardown = 0U;
	cis->released_cb = NULL;

	cis->lll.handle = LLL_HANDLE_INVALID;

#if WIP
	cis->lll.acl_handle = acl->lll.handle;
	cis->lll.sub_interval = sys_get_le24(req->sub_interval);
	cis->lll.num_subevents = req->nse;
	cis->lll.next_subevent = 0U;
#endif

	ARG_UNUSED(c_rtn);
	ARG_UNUSED(p_rtn);

	cis->lll.sn = 0U;
	cis->lll.nesn = 0U;
	cis->lll.cie = 0U;
	cis->lll.flushed = 0U;
	cis->lll.datapath_ready_rx = 0U;

	cis->lll.rx.phy = p_phy;

#if WIP
	cis->lll.rx.burst_number = req->c_bn;
	cis->lll.rx.flush_timeout = req->c_ft;
	cis->lll.rx.max_octets = sys_le16_to_cpu(req->c_max_pdu);
	cis->lll.rx.payload_number = 0;
#endif

	cis->lll.tx.phy = c_phy;

#if WIP
	cis->lll.tx.burst_number = req->p_bn;
	cis->lll.tx.flush_timeout = req->p_ft;
	cis->lll.tx.max_octets = sys_le16_to_cpu(req->p_max_pdu);
	cis->lll.tx.payload_number = 0;
#endif

	if (!cis->lll.link_tx_free) {
		cis->lll.link_tx_free = &cis->lll.link_tx;
	}

	memq_init(cis->lll.link_tx_free, &cis->lll.memq_tx.head,
		  &cis->lll.memq_tx.tail);
	cis->lll.link_tx_free = NULL;

	*cis_handle = ll_conn_iso_stream_handle_get(cis);

	return 0U;
}

uint8_t ll_cig_parameters_test_open(uint8_t cig_id,
				    uint32_t c_interval,
				    uint32_t p_interval,
				    uint8_t  c_ft,
				    uint8_t  p_ft,
				    uint16_t iso_interval,
				    uint8_t  sca,
				    uint8_t  packing,
				    uint8_t  framing,
				    uint8_t  num_cis)
{
	ARG_UNUSED(cig_id);
	ARG_UNUSED(c_interval);
	ARG_UNUSED(p_interval);
	ARG_UNUSED(c_ft);
	ARG_UNUSED(p_ft);
	ARG_UNUSED(iso_interval);
	ARG_UNUSED(sca);
	ARG_UNUSED(packing);
	ARG_UNUSED(framing);
	ARG_UNUSED(num_cis);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cis_parameters_test_set(uint8_t  cis_id,
				   uint16_t c_sdu, uint16_t p_sdu,
				   uint16_t c_pdu, uint16_t p_pdu,
				   uint8_t c_phy, uint8_t p_phy,
				   uint8_t c_bn, uint8_t p_bn,
				   uint16_t *handle)
{
	ARG_UNUSED(cis_id);
	ARG_UNUSED(c_sdu);
	ARG_UNUSED(p_sdu);
	ARG_UNUSED(c_pdu);
	ARG_UNUSED(p_pdu);
	ARG_UNUSED(c_phy);
	ARG_UNUSED(p_phy);
	ARG_UNUSED(c_bn);
	ARG_UNUSED(p_bn);
	ARG_UNUSED(handle);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cig_parameters_commit(uint8_t cig_id)
{
	ARG_UNUSED(cig_id);

	/* TODO: Is there anything to be done here? */

	return 0U;
}

uint8_t ll_cig_remove(uint8_t cig_id)
{
	ARG_UNUSED(cig_id);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_cis_create_check(uint16_t cis_handle, uint16_t acl_handle)
{
	ARG_UNUSED(cis_handle);
	ARG_UNUSED(acl_handle);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

void ll_cis_create(uint16_t cis_handle, uint16_t acl_handle)
{
	ARG_UNUSED(cis_handle);
	ARG_UNUSED(acl_handle);
}

int ull_central_iso_init(void)
{
	return 0;
}

int ull_central_iso_reset(void)
{
	return 0;
}
