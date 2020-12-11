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
#include "hal/ccm.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/mem.h"
#include "util/mfifo.h"
#include "ticker/ticker.h"

#include "pdu.h"
#include "ll.h"
#include "lll.h"

#include "lll_vendor.h"
#include "lll_adv.h"
#include "lll_conn.h"

#include "ull_internal.h"
#include "ull_adv_types.h"
#include "ull_adv_internal.h"

static struct ll_adv_iso ll_adv_iso[CONFIG_BT_CTLR_ADV_SET];
static void *adv_iso_free;

static uint32_t ull_adv_iso_start(struct ll_adv_iso *adv_iso,
				  uint32_t ticks_anchor);
static inline struct ll_adv_iso *ull_adv_iso_get(uint8_t handle);
static int init_reset(void);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param);
static void tx_lll_flush(void *param);
static void ticker_op_stop_cb(uint32_t status, void *param);

uint8_t ll_big_create(uint8_t big_handle, uint8_t adv_handle, uint8_t num_bis,
		      uint32_t sdu_interval, uint16_t max_sdu,
		      uint16_t max_latency, uint8_t rtn, uint8_t phy,
		      uint8_t packing, uint8_t framing, uint8_t encryption,
		      uint8_t *bcode)
{
	struct ll_adv_iso *adv_iso;
	struct ll_adv_set *adv;
	struct node_rx_pdu *node_rx;

	adv_iso = ull_adv_iso_get(big_handle);

	if (!adv_iso || adv_iso->is_created) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	adv = ull_adv_is_created_get(adv_handle);

	/* Does not identify a periodic advertising train or
	 * the periodic advertising trains is already associated
	 * with another BIG.
	 */
	if (!adv || !adv->lll.sync || adv->lll.sync->adv_iso) {
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
	ull_adv_iso_start(adv_iso, 0 /* TODO: Calc ticks_anchor */);

	/* Prepare BIG complete event */
	/* TODO: Implement custom node_rx struct for optimization */
	node_rx = (void *)&adv_iso->node_rx_complete;
	node_rx->hdr.type = NODE_RX_TYPE_BIG_COMPLETE;
	node_rx->hdr.handle = big_handle;
	node_rx->hdr.rx_ftr.param = adv_iso;

	adv_iso->is_created = true;

	return BT_HCI_ERR_SUCCESS;
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
	struct node_rx_pdu *node_rx;
	uint32_t ret;

	adv_iso = ull_adv_iso_get(big_handle);

	if (!adv_iso) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* TODO: Terminate all BIS data paths */

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			  TICKER_ID_ADV_ISO_BASE + adv_iso->bis_handle,
			  ticker_op_stop_cb, adv_iso);

	adv_iso->is_created = 0U;

	/* Prepare BIG terminate event */
	node_rx = (void *)&adv_iso->node_rx_terminate;
	node_rx->hdr.type = NODE_RX_TYPE_BIG_TERMINATE;
	node_rx->hdr.handle = big_handle;
	node_rx->hdr.rx_ftr.param = adv_iso;
	*((uint8_t *)node_rx->pdu) = reason;

	return BT_HCI_ERR_SUCCESS;
}

int ull_adv_iso_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_adv_iso_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING)
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
#endif /* CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING */

static uint32_t ull_adv_iso_start(struct ll_adv_iso *adv_iso,
				  uint32_t ticks_anchor)
{
	uint32_t ticks_slot_overhead;
	uint32_t volatile ret_cb;
	uint32_t iso_interval_us;
	uint32_t slot_us;
	uint32_t ret;

	ull_hdr_init(&adv_iso->ull);

	/* TODO: Calc slot_us */
	slot_us = EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;
	slot_us += 1000;

	adv_iso->evt.ticks_active_to_start = 0;
	adv_iso->evt.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	adv_iso->evt.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	adv_iso->evt.ticks_slot = HAL_TICKER_US_TO_TICKS(slot_us);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(adv_iso->evt.ticks_active_to_start,
					  adv_iso->evt.ticks_xtal_to_start);
	} else {
		ticks_slot_overhead = 0;
	}

	/* TODO: Calculate ISO interval */
	/* iso_interval shall be at least SDU interval,
	 * or integer multiple of SDU interval for unframed PDUs
	 */
	iso_interval_us = adv_iso->sdu_interval;

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			   (TICKER_ID_ADV_ISO_BASE + adv_iso->bis_handle),
			   ticks_anchor, 0,
			   HAL_TICKER_US_TO_TICKS(iso_interval_us),
			   HAL_TICKER_REMAINDER(iso_interval_us),
			   TICKER_NULL_LAZY,
			   (ll_adv_iso->evt.ticks_slot + ticks_slot_overhead),
			   ticker_cb, ll_adv_iso,
			   ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);

	return 0;
}

static inline struct ll_adv_iso *ull_adv_iso_get(uint8_t handle)
{
	if (handle >= CONFIG_BT_CTLR_ADV_SET) {
		return NULL;
	}

	return &ll_adv_iso[handle];
}

static int init_reset(void)
{
	/* Initialize pool. */
	mem_init(ll_adv_iso, sizeof(struct ll_adv_iso),
		 sizeof(ll_adv_iso) / sizeof(struct ll_adv_iso), &adv_iso_free);

	return 0;
}


static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param)
{
	/* TODO: LLL support for ADV ISO */
#if 0
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_adv_iso_prepare};
	static struct lll_prepare_param p;
	struct ll_adv_iso *adv_iso = param;
	struct lll_adv_iso *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_A(1);

	lll = &adv_iso->lll;

	/* Increment prepare reference count */
	ref = ull_ref_inc(&adv_iso->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.param = lll;
	mfy.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
			     TICKER_USER_ID_LLL, 0, &mfy);
	LL_ASSERT(!ret);

	DEBUG_RADIO_PREPARE_A(1);
#endif
}

static void tx_lll_flush(void *param)
{
	/* TODO: LLL support for ADV ISO */
	/* TODO: Send terminate complete event to host */
#if 0
	/* TODO: Flush TX */
	struct ll_adv_iso *lll = param;
#endif
}

static void ticker_op_stop_cb(uint32_t status, void *param)
{
	uint32_t retval;
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, tx_lll_flush};

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	mfy.param = param;

	/* Flush pending tx PDUs in LLL (using a mayfly) */
	retval = mayfly_enqueue(TICKER_USER_ID_ULL_LOW, TICKER_USER_ID_LLL, 0,
				&mfy);
	LL_ASSERT(!retval);
}
