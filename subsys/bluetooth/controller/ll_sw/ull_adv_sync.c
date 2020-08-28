/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <soc.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"
#include "ll.h"
#include "lll.h"
#include "lll_vendor.h"
#include "lll_adv.h"
#include "lll_adv_sync.h"
#include "lll_adv_internal.h"

#include "ull_adv_types.h"

#include "ull_internal.h"
#include "ull_chan_internal.h"
#include "ull_adv_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_adv_sync
#include "common/log.h"
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_adv_sync_set *sync_acquire(void);
static inline void sync_release(struct ll_adv_sync_set *sync);
static inline uint16_t sync_handle_get(struct ll_adv_sync_set *sync);
static inline uint8_t sync_stop(struct ll_adv_sync_set *sync);
static void mfy_sync_offset_get(void *param);
static inline struct pdu_adv_sync_info *sync_info_get(struct pdu_adv *pdu);
static inline void sync_info_offset_fill(struct pdu_adv_sync_info *si,
					 uint32_t ticks_offset,
					 uint32_t start_us);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param);
static void ticker_op_cb(uint32_t status, void *param);

static struct ll_adv_sync_set ll_adv_sync_pool[CONFIG_BT_CTLR_ADV_SYNC_SET];
static void *adv_sync_free;

uint8_t ll_adv_sync_param_set(uint8_t handle, uint16_t interval, uint16_t flags)
{
	struct pdu_adv_hdr *ter_hdr, ter_hdr_prev;
	struct pdu_adv_com_ext_adv *ter_com_hdr;
	uint8_t *ter_dptr_prev, *ter_dptr;
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct pdu_adv *ter_pdu;
	struct ll_adv_set *adv;
	uint8_t ter_len;

	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		struct lll_adv *lll;
		int err;

		sync = sync_acquire();
		if (!sync) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		lll = &adv->lll;
		lll_sync = &sync->lll;
		lll->sync = lll_sync;
		lll_sync->adv = lll;

		/* NOTE: ull_hdr_init(&sync->ull); is done on start */
		lll_hdr_init(lll_sync, sync);

		err = util_aa_le32(lll_sync->access_addr);
		LL_ASSERT(!err);

		lll_csrand_get(lll_sync->crc_init, sizeof(lll_sync->crc_init));

		lll_sync->latency_prepare = 0;
		lll_sync->latency_event = 0;
		lll_sync->event_counter = 0;

		lll_sync->data_chan_count =
			ull_chan_map_get(lll_sync->data_chan_map);
		lll_sync->data_chan_id = 0;

		sync->is_enabled = 0U;
		sync->is_started = 0U;
	} else {
		sync = (void *)HDR_LLL2EVT(lll_sync);
	}

	sync->interval = interval;

	ter_pdu = lll_adv_sync_data_peek(lll_sync);
	ter_pdu->type = PDU_ADV_TYPE_AUX_SYNC_IND;
	ter_pdu->rfu = 0U;
	ter_pdu->chan_sel = 0U;
	ter_pdu->tx_addr = 0U;
	ter_pdu->rx_addr = 0U;

	ter_com_hdr = (void *)&ter_pdu->adv_ext_ind;
	ter_hdr = (void *)ter_com_hdr->ext_hdr_adi_adv_data;
	ter_dptr = (uint8_t *)ter_hdr + sizeof(*ter_hdr);
	ter_hdr_prev = *ter_hdr;
	*(uint8_t *)ter_hdr = 0U;
	ter_dptr_prev = ter_dptr;

	/* Non-connectable and Non-scannable adv mode */
	ter_com_hdr->adv_mode = 0U;

	/* No AdvA */
	/* No TargetA */

	/* TODO: CTEInfo */

	/* No ADI */

	/* TODO: AuxPtr */

	/* No SyncInfo */

	/* TODO: TxPower */
	if (flags & BT_HCI_LE_ADV_PROP_TX_POWER) {
		/* TODO: add/remove Tx Power in AUX_SYNC_IND PDU */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* TODO: ACAD */

	/* TODO: AdvData */

	/* Calc tertiary PDU len */
	ter_len = ull_adv_aux_hdr_len_get(ter_com_hdr, ter_dptr);
	ull_adv_aux_hdr_len_fill(ter_com_hdr, ter_len);

	ter_pdu->len = ter_len;

	return 0;
}

uint8_t ll_adv_sync_ad_data_set(uint8_t handle, uint8_t op, uint8_t len,
				uint8_t const *const data)
{
	/* TODO */
	return BT_HCI_ERR_CMD_DISALLOWED;
}

uint8_t ll_adv_sync_enable(uint8_t handle, uint8_t enable)
{
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct ll_adv_set *adv;

	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	sync = (void *)HDR_LLL2EVT(lll_sync);

	if (!enable) {
		uint8_t err;

		if (!sync->is_enabled) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		if (!sync->is_started) {
			sync->is_enabled = 0U;

			return 0;
		}

		/* Remove sync_info from auxiliary PDU */
		err = ull_adv_aux_hdr_set_clear(adv, 0,
						ULL_ADV_PDU_HDR_FIELD_SYNC_INFO,
						NULL, NULL);
		if (err) {
			return err;
		}

		err = sync_stop(sync);
		if (err) {
			return err;
		}

		sync->is_started = 0U;
		sync->is_enabled = 0U;

		return 0;
	}

	/* TODO: Check for periodic data being complete */

	/* TODO: Check packet too long */

	if (sync->is_enabled) {
		/* TODO: Enabling an already enabled advertising changes its
		 * random address.
		 */
	} else {
		sync->is_enabled = 1U;
	}

	if (adv->is_enabled && !sync->is_started) {
		volatile uint32_t ret_cb = TICKER_STATUS_BUSY;
		uint32_t ticks_anc_sync;
		uint8_t err;

		/* FIXME: Find absolute ticks until after auxliary PDU on air
		 *        to place the periodic advertising PDU.
		 */
		ticks_anc_sync = ticker_ticks_now_get();

		/* Add sync_info into auxiliary PDU */
		err = ull_adv_aux_hdr_set_clear(adv,
						ULL_ADV_PDU_HDR_FIELD_SYNC_INFO,
						0, NULL, NULL);
		if (err) {
			return err;
		}

		ull_hdr_init(&sync->ull);

		err = ull_adv_sync_start(sync, ticks_anc_sync, &ret_cb);
		if (err) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		sync->is_started = 1U;
	}

	return 0;
}

int ull_adv_sync_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_adv_sync_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

uint16_t ull_adv_sync_lll_handle_get(struct lll_adv_sync *lll)
{
	return sync_handle_get((void *)lll->hdr.parent);
}

uint32_t ull_adv_sync_start(struct ll_adv_sync_set *sync, uint32_t ticks_anchor,
			 uint32_t volatile *ret_cb)
{
	uint32_t slot_us = EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;
	uint32_t ticks_slot_overhead;
	uint32_t interval_us;
	uint8_t sync_handle;
	uint32_t ret;

	/* TODO: Calc AUX_SYNC_IND slot_us */
	slot_us += 1000;

	/* TODO: active_to_start feature port */
	sync->evt.ticks_active_to_start = 0;
	sync->evt.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	sync->evt.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	sync->evt.ticks_slot = HAL_TICKER_US_TO_TICKS(slot_us);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(sync->evt.ticks_active_to_start,
					  sync->evt.ticks_xtal_to_start);
	} else {
		ticks_slot_overhead = 0;
	}

	interval_us = (uint64_t)sync->interval * 1250U;

	sync_handle = sync_handle_get(sync);

	*ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_SYNC_BASE + sync_handle),
			   ticks_anchor, 0,
			   HAL_TICKER_US_TO_TICKS(interval_us),
			   HAL_TICKER_REMAINDER(interval_us), TICKER_NULL_LAZY,
			   (sync->evt.ticks_slot + ticks_slot_overhead),
			   ticker_cb, sync,
			   ull_ticker_status_give, (void *)ret_cb);

	return ret;
}

void ull_adv_sync_offset_get(struct ll_adv_set *adv)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mfy_sync_offset_get};
	uint32_t ret;

	mfy.param = adv;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1,
			     &mfy);
	LL_ASSERT(!ret);
}

static int init_reset(void)
{
	/* Initialize adv sync pool. */
	mem_init(ll_adv_sync_pool, sizeof(struct ll_adv_sync_set),
		 sizeof(ll_adv_sync_pool) / sizeof(struct ll_adv_sync_set),
		 &adv_sync_free);

	return 0;
}

static inline struct ll_adv_sync_set *sync_acquire(void)
{
	return mem_acquire(&adv_sync_free);
}

static inline void sync_release(struct ll_adv_sync_set *sync)
{
	mem_release(sync, &adv_sync_free);
}

static inline uint16_t sync_handle_get(struct ll_adv_sync_set *sync)
{
	return mem_index_get(sync, ll_adv_sync_pool,
			     sizeof(struct ll_adv_sync_set));
}

static inline uint8_t sync_stop(struct ll_adv_sync_set *sync)
{
	uint32_t volatile ret_cb;
	uint8_t sync_handle;
	void *mark;
	uint32_t ret;

	mark = ull_disable_mark(sync);
	LL_ASSERT(mark == sync);

	sync_handle = sync_handle_get(sync);

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			  TICKER_ID_ADV_SYNC_BASE + sync_handle,
			  ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret) {
		mark = ull_disable_mark(sync);
		LL_ASSERT(mark == sync);

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ret = ull_disable(&sync->lll);
	LL_ASSERT(!ret);

	mark = ull_disable_unmark(sync);
	LL_ASSERT(mark == sync);

	return 0;
}

static void mfy_sync_offset_get(void *param)
{
	struct ll_adv_set *adv = param;
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct pdu_adv_sync_info *si;
	uint32_t ticks_to_expire;
	uint32_t ticks_current;
	struct pdu_adv *pdu;
	uint8_t ticker_id;
	uint8_t retry;
	uint8_t id;

	lll_sync = adv->lll.sync;
	sync = (void *)HDR_LLL2EVT(lll_sync);
	ticker_id = TICKER_ID_ADV_SYNC_BASE + sync_handle_get(sync);

	id = TICKER_NULL;
	ticks_to_expire = 0U;
	ticks_current = 0U;
	retry = 4U;
	do {
		uint32_t volatile ret_cb;
		uint32_t ticks_previous;
		uint32_t ret;

		ticks_previous = ticks_current;

		ret_cb = TICKER_STATUS_BUSY;
		ret = ticker_next_slot_get(TICKER_INSTANCE_ID_CTLR,
					   TICKER_USER_ID_ULL_LOW,
					   &id,
					   &ticks_current, &ticks_to_expire,
					   ticker_op_cb, (void *)&ret_cb);
		if (ret == TICKER_STATUS_BUSY) {
			while (ret_cb == TICKER_STATUS_BUSY) {
				ticker_job_sched(TICKER_INSTANCE_ID_CTLR,
						 TICKER_USER_ID_ULL_LOW);
			}
		}

		LL_ASSERT(ret_cb == TICKER_STATUS_SUCCESS);

		LL_ASSERT((ticks_current == ticks_previous) || retry--);

		LL_ASSERT(id != TICKER_NULL);
	} while (id != ticker_id);

	/* NOTE: as remainder not used in scheduling primary PDU
	 * packet timer starts transmission after 1 tick hence the +1.
	 */
	lll_sync->ticks_offset = ticks_to_expire + 1;

	pdu = lll_adv_aux_data_curr_get(adv->lll.aux);
	si = sync_info_get(pdu);
	sync_info_offset_fill(si, ticks_to_expire, 0);
	si->evt_cntr = lll_sync->event_counter + lll_sync->latency_prepare;
}

static inline struct pdu_adv_sync_info *sync_info_get(struct pdu_adv *pdu)
{
	struct pdu_adv_com_ext_adv *p;
	struct pdu_adv_hdr *h;
	uint8_t *ptr;

	p = (void *)&pdu->adv_ext_ind;
	h = (void *)p->ext_hdr_adi_adv_data;
	ptr = (uint8_t *)h + sizeof(*h);

	if (h->adv_addr) {
		ptr += BDADDR_SIZE;
	}

	if (h->adi) {
		ptr += sizeof(struct pdu_adv_adi);
	}

	if (h->aux_ptr) {
		ptr += sizeof(struct pdu_adv_aux_ptr);
	}

	return (void *)ptr;
}

static inline void sync_info_offset_fill(struct pdu_adv_sync_info *si,
					 uint32_t ticks_offset,
					 uint32_t start_us)
{
	uint32_t offs;

	offs = HAL_TICKER_TICKS_TO_US(ticks_offset) - start_us;
	if (si->offs_units) {
		si->offs = offs / SYNC_PKT_OFFS_UNIT_300_US;
	} else {
		si->offs = offs / SYNC_PKT_OFFS_UNIT_30_US;
	}
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_adv_sync_prepare};
	static struct lll_prepare_param p;
	struct ll_adv_sync_set *sync = param;
	struct lll_adv_sync *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_A(1);

	lll = &sync->lll;

	/* Increment prepare reference count */
	ref = ull_ref_inc(&sync->ull);
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
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}
