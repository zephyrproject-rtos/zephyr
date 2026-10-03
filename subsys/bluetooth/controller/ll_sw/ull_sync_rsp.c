/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci_types.h>

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_clock.h"
#include "lll/lll_vendor.h"
#include "lll_chan.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"

#include "isoal.h"

#include "ull_tx_queue.h"

#include "ull_filter.h"
#include "ull_iso_types.h"
#include "ull_scan_types.h"
#include "ull_sync_types.h"
#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"

#include "ull_internal.h"
#include "ull_scan_internal.h"
#include "ull_sync_internal.h"
#include "ull_conn_internal.h"
#include "ull_conn_iso_internal.h"

#include "ll.h"

#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_RSP)

/**
 * @brief Set subevents to synchronize to for periodic advertising with responses
 *
 * @param handle                Sync handle
 * @param periodic_adv_properties Periodic advertising properties
 * @param num_subevents         Number of subevents to synchronize to
 * @param subevents             Array of subevent indices
 *
 * @return 0 on success, error code otherwise
 */
uint8_t ll_sync_subevent_set(uint16_t handle, uint16_t periodic_adv_properties,
			      uint8_t num_subevents, const uint8_t *subevents)
{
	struct ll_sync_set *sync;

	/* Get sync set by handle */
	sync = ull_sync_is_enabled_get(handle);
	if (!sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Validate parameters */
	if (num_subevents == 0 || num_subevents > BT_HCI_PAWR_SUBEVENT_MAX) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Store subevent selection */
	sync->num_subevents = num_subevents;
	memcpy(sync->subevents, subevents, num_subevents);

	/* Mark LLL as PAwR mode */
	sync->lll.is_rsp = 1U;

	ARG_UNUSED(periodic_adv_properties);

	return BT_HCI_ERR_SUCCESS;
}

/**
 * @brief Set response data for periodic advertising with responses
 *
 * @param handle           Sync handle
 * @param request_event    Event counter of the request
 * @param request_subevent Subevent where request was received
 * @param response_subevent Subevent where response will be sent
 * @param response_slot    Response slot to use
 * @param response_data_len Length of response data
 * @param response_data    Response data to transmit
 *
 * @return 0 on success, error code otherwise
 */
uint8_t ll_sync_response_data_set(uint16_t handle, uint16_t request_event,
				   uint8_t request_subevent, uint8_t response_subevent,
				   uint8_t response_slot, uint8_t response_data_len,
				   const uint8_t *response_data)
{
	struct ll_sync_set *sync;

	/* Get sync set by handle */
	sync = ull_sync_is_enabled_get(handle);
	if (!sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Validate data length */
	if (response_data_len > CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Validate that we have data if length is non-zero */
	if (response_data_len > 0 && response_data == NULL) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* Store response data for transmission
	 * Note: In a complete implementation, we'd check timing constraints
	 * to ensure the response can be sent in the specified subevent/slot
	 */
	sync->rsp_data.request_event = request_event;
	sync->rsp_data.request_subevent = request_subevent;
	sync->rsp_data.response_subevent = response_subevent;
	sync->rsp_data.response_slot = response_slot;
	sync->rsp_data.len = response_data_len;
	sync->rsp_data.is_pending = 1U;

	if (response_data_len > 0) {
		memcpy(sync->rsp_data.data, response_data, response_data_len);
	}

	return BT_HCI_ERR_SUCCESS;
}

#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_RSP */
