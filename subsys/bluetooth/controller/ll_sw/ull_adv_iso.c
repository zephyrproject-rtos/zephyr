/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_adv_iso
#include "common/log.h"
#include "hal/debug.h"
#include "hal/cpu.h"
#include "util/util.h"
#include "util/memq.h"

#include "pdu.h"
#include "ll.h"
#include "lll.h"

#include "lll_adv.h"
#include "lll_scan.h"

#include "ull_adv_types.h"
#include "ull_adv_internal.h"

static struct ll_adv_iso ll_adv_iso[CONFIG_BT_CTLR_ADV_SET];

uint8_t ll_adv_iso_by_hci_handle_get(uint8_t hci_handle, uint8_t *handle)
{
	struct ll_adv_iso *adv_iso;
	uint8_t idx;

	adv_iso =  &ll_adv_iso[0];

	for (idx = 0U; idx < CONFIG_BT_CTLR_ADV_SET; idx++, adv_iso++) {
		if (adv_iso->is_created &&
		    (adv_iso->hci_handle == hci_handle)) {
			*handle = idx;
			return 0;
		}
	}

	return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
}

uint8_t ll_adv_iso_by_hci_handle_new(uint8_t hci_handle, uint8_t *handle)
{
	struct ll_adv_iso *adv_iso, *adv_iso_empty;
	uint8_t idx;

	adv_iso = &ll_adv_iso[0];
	adv_iso_empty = NULL;

	for (idx = 0U; idx < CONFIG_BT_CTLR_ADV_SET; idx++, adv_iso++) {
		if (adv_iso->is_created) {
			if (adv_iso->hci_handle == hci_handle) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}
		} else if (!adv_iso_empty) {
			adv_iso_empty = adv_iso;
			*handle = idx;
		}
	}

	if (adv_iso_empty) {
		memset(adv_iso_empty, 0, sizeof(*adv_iso_empty));
		adv_iso_empty->hci_handle = hci_handle;
		return 0;
	}

	return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
}

static inline struct ll_adv_iso *ull_adv_iso_get(uint8_t handle)
{
	if (handle >= CONFIG_BT_CTLR_ADV_SET) {
		return NULL;
	}

	return &ll_adv_iso[handle];
}

uint8_t ll_big_create(uint8_t big_handle, uint8_t adv_handle, uint8_t num_bis,
		      uint32_t sdu_interval, uint16_t max_sdu,
		      uint16_t max_latency, uint8_t rtn, uint8_t phy,
		      uint8_t packing, uint8_t framing, uint8_t encryption,
		      uint8_t *bcode, void **rx)
{
	struct ll_adv_iso *adv_iso;
	struct ll_adv_set *adv;
	struct node_rx_pdu *node_rx;

	adv_iso = ull_adv_iso_get(big_handle);

	if (!adv_iso) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	adv = ull_adv_is_created_get(adv_handle);

	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	} else if (!adv->lll.sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	} else if (adv->lll.sync->adv_iso) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK)) {
		if (num_bis == 0 || num_bis > 0x1F) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (sdu_interval < 0x000100 || sdu_interval > 0x0FFFFF) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (max_sdu < 0x0001 || max_sdu > 0x0FFF) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (max_latency > 0x0FA0) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (rtn > 0x0F) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (phy > (BT_HCI_LE_EXT_SCAN_PHY_1M |
			   BT_HCI_LE_EXT_SCAN_PHY_2M |
			   BT_HCI_LE_EXT_SCAN_PHY_CODED)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (packing > 1) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (framing > 1) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (encryption > 1) {
			return BT_HCI_ERR_INVALID_PARAM;
		}
	}

	/* TODO: Allow more than 1 BIS in a BIG */
	if (num_bis != 1) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}
	/* TODO: For now we can just use the unique BIG handle as the BIS
	 * handle until we support multiple BIS
	 */
	adv_iso->bis_handle = big_handle;

	adv_iso->num_bis = num_bis;
	adv_iso->sdu_interval = sdu_interval;
	adv_iso->max_sdu = max_sdu;
	adv_iso->max_latency = max_latency;
	adv_iso->rtn = rtn;
	adv_iso->phy = phy;
	adv_iso->packing = packing;
	adv_iso->framing = framing;
	adv_iso->encryption = encryption;
	memcpy(adv_iso->bcode, bcode, sizeof(adv_iso->bcode));

	/* TODO: Add ACAD to AUX_SYNC_IND */

	/* TODO: start sending BIS empty data packet for each BIS */

	/* Prepare BIG complete event */
	node_rx = (void *)&adv_iso->node_rx_complete;
	node_rx->hdr.type = NODE_RX_TYPE_BIG_COMPLETE;
	node_rx->hdr.handle = big_handle;
	node_rx->hdr.rx_ftr.param = adv_iso;

	*rx = node_rx;

	return 0;
}

uint8_t ll_big_test_create(uint8_t big_handle, uint8_t adv_handle,
			   uint8_t num_bis, uint32_t sdu_interval,
			   uint16_t iso_interval, uint8_t nse, uint16_t max_sdu,
			   uint16_t max_pdu, uint8_t phy, uint8_t packing,
			   uint8_t framing, uint8_t bn, uint8_t irc,
			   uint8_t pto, uint8_t encryption, uint8_t *bcode)
{
	/* TODO: Implement */
	ARG_UNUSED(big_handle);
	ARG_UNUSED(adv_handle);
	ARG_UNUSED(num_bis);
	ARG_UNUSED(sdu_interval);
	ARG_UNUSED(iso_interval);
	ARG_UNUSED(nse);
	ARG_UNUSED(max_sdu);
	ARG_UNUSED(max_pdu);
	ARG_UNUSED(phy);
	ARG_UNUSED(packing);
	ARG_UNUSED(framing);
	ARG_UNUSED(bn);
	ARG_UNUSED(irc);
	ARG_UNUSED(pto);
	ARG_UNUSED(encryption);
	ARG_UNUSED(bcode);

	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_big_terminate(uint8_t big_handle, uint8_t reason)
{
	struct ll_adv_iso *adv_iso;

	adv_iso = ull_adv_iso_get(big_handle);

	if (!adv_iso) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* TODO: Implement */
	ARG_UNUSED(reason);

	return BT_HCI_ERR_CMD_DISALLOWED;
}
