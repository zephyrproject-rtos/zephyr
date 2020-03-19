/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <sys/byteorder.h>
#include <bluetooth/hci.h>

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
#include "lll_adv_aux.h"
#include "lll_adv_internal.h"

#include "ull_adv_types.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_adv_aux
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
static inline struct ll_adv_aux_set *aux_acquire(void);
static inline void aux_release(struct ll_adv_aux_set *aux);
static inline uint16_t aux_handle_get(struct ll_adv_aux_set *aux);
static void mfy_aux_offset_get(void *param);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param);
static void ticker_op_cb(uint32_t status, void *param);

static struct ll_adv_aux_set ll_adv_aux_pool[CONFIG_BT_CTLR_ADV_AUX_SET];
static void *adv_aux_free;
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

uint8_t ll_adv_aux_random_addr_set(uint8_t handle, uint8_t *addr)
{
	/* TODO: store in adv set instance */
	return 0;
}

uint8_t *ll_adv_aux_random_addr_get(uint8_t handle, uint8_t *addr)
{
	/* TODO: copy adv set instance addr into addr and/or return reference */
	return NULL;
}

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
uint8_t ll_adv_aux_ad_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref, uint8_t len,
			    uint8_t *data)
{
	struct pdu_adv_com_ext_adv *p, *_p, *s, *_s;
	uint8_t pri_len, _pri_len, sec_len, _sec_len;
	struct pdu_adv *_pri, *pri, *_sec, *sec;
	struct ext_adv_hdr *hp, _hp, *hs, _hs;
	struct lll_adv_aux *lll_aux;
	struct ll_adv_aux_set *aux;
	uint8_t *_pp, *pp, *ps, *_ps;
	struct ll_adv_set *adv;
	struct lll_adv *lll;
	uint8_t ip, is;

	/* op param definitions:
	 * 0x00 - Intermediate fragment of fragmented extended advertising data
	 * 0x01 - First fragment of fragmented extended advertising data
	 * 0x02 - Last fragemnt of fragemented extended advertising data
	 * 0x03 - Complete extended advertising data
	 * 0x04 - Unchanged data (just update the advertising data)
	 * All other values, Reserved for future use
	 */

	/* TODO: handle other op values */
	if ((op != 0x03) && (op != 0x04)) {
		/* FIXME: error code */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Get the advertising set instance */
	adv = ull_adv_set_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lll = &adv->lll;

	lll_aux = lll->aux;
	if (!lll_aux) {
		aux = aux_acquire();
		if (!aux) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		lll_aux = &aux->lll;
		lll->aux = lll_aux;
		lll_aux->adv = lll;

		/* NOTE: ull_hdr_init(&aux->ull); is done on start */
		lll_hdr_init(lll_aux, aux);

		aux->is_started = 0;
	} else {
		aux = (void *)HDR_LLL2EVT(lll_aux);
	}

	/* Do not update data if not extended advertising. */
	_pri = lll_adv_data_peek(lll);
	if (_pri->type != PDU_ADV_TYPE_EXT_IND) {
		/* Advertising Handle has not been created using
		 * Set Extended Advertising Parameter command
		 */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Get reference to previous primary PDU data */
	_p = (void *)&_pri->adv_ext_ind;
	hp = (void *)_p->ext_hdr_adi_adv_data;
	*(uint8_t *)&_hp = *(uint8_t *)hp;
	_pp = (uint8_t *)hp + sizeof(*hp);

	/* Get reference to new primary PDU data buffer */
	pri = lll_adv_data_alloc(lll, &ip);
	pri->type = _pri->type;
	pri->rfu = 0U;
	pri->chan_sel = 0U;
	p = (void *)&pri->adv_ext_ind;
	p->adv_mode = _p->adv_mode;
	hp = (void *)p->ext_hdr_adi_adv_data;
	pp = (uint8_t *)hp + sizeof(*hp);
	*(uint8_t *)hp = 0U;

	/* Get reference to previous secondary PDU data */
	_sec = lll_adv_aux_data_peek(lll_aux);
	_s = (void *)&_sec->adv_ext_ind;
	hs = (void *)_s->ext_hdr_adi_adv_data;
	*(uint8_t *)&_hs = *(uint8_t *)hs;
	_ps = (uint8_t *)hs + sizeof(*hs);

	/* Get reference to new secondary PDU data buffer */
	sec = lll_adv_aux_data_alloc(lll_aux, &is);
	sec->type = pri->type;
	sec->rfu = 0U;

	if (IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2)) {
		sec->chan_sel = _sec->chan_sel;
	} else {
		sec->chan_sel = 0U;
	}

	sec->tx_addr = _sec->tx_addr;
	sec->rx_addr = _sec->rx_addr;

	s = (void *)&sec->adv_ext_ind;
	s->adv_mode = p->adv_mode;
	hs = (void *)s->ext_hdr_adi_adv_data;
	ps = (uint8_t *)hs + sizeof(*hs);
	*(uint8_t *)hs = 0U;

	/* AdvA flag */
	/* NOTE: as we will use auxiliary packet, we remove AdvA in primary
	 * channel. i.e. Do nothing to add AdvA in the primary PDU.
	 */
	if (_hp.adv_addr) {
		_pp += BDADDR_SIZE;

		/* Prepare to add AdvA in secondary PDU */
		hs->adv_addr = 1;

		/* NOTE: AdvA is filled at enable */
		sec->tx_addr = pri->tx_addr;
	}
	pri->tx_addr = 0U;
	pri->rx_addr = 0U;

	if (_hs.adv_addr) {
		_ps += BDADDR_SIZE;
		hs->adv_addr = 1;
	}
	if (hs->adv_addr) {
		ps += BDADDR_SIZE;
	}

	/* No TargetA in primary and secondary channel for undirected */
	/* No CTEInfo flag in primary and secondary channel PDU */

	/* ADI flag */
	if (_hp.adi) {
		_pp += sizeof(struct ext_adv_adi);
	}
	hp->adi = 1;
	pp += sizeof(struct ext_adv_adi);
	if (_hs.adi) {
		_ps += sizeof(struct ext_adv_adi);
	}
	hs->adi = 1;
	ps += sizeof(struct ext_adv_adi);

	/* AuxPtr flag */
	if (_hp.aux_ptr) {
		_pp += sizeof(struct ext_adv_aux_ptr);
	}
	hp->aux_ptr = 1;
	pp += sizeof(struct ext_adv_aux_ptr);
	if (_hs.aux_ptr) {
		_ps += sizeof(struct ext_adv_aux_ptr);

		hs->aux_ptr = 1;
		ps += sizeof(struct ext_adv_aux_ptr);
	}

	/* No SyncInfo flag in primary channel PDU */
	/* SyncInfo flag in secondary channel PDU */
	if (_hs.sync_info) {
		_ps += sizeof(struct ext_adv_sync_info);

		hs->sync_info = 1;
		ps += sizeof(struct ext_adv_sync_info);
	}

	/* Tx Power flag */
	if (_hp.tx_pwr) {
		_pp++;

		/* C1, Tx Power is optional on the LE 1M PHY, and reserved for
		 * for future use on the LE Coded PHY.
		 */
		if (lll->phy_p != BIT(2)) {
			hp->tx_pwr = 1;
			pp++;
		} else {
			hs->tx_pwr = 1;
		}
	}
	if (_hs.tx_pwr) {
		_ps++;

		hs->tx_pwr = 1;
	}
	if (hs->tx_pwr) {
		ps++;
	}

	/* No ACAD in Primary channel PDU */
	/* TODO: ACAD in Secondary channel PDU */

	/* Calc primary PDU len */
	_pri_len = _pp - (uint8_t *)_p;
	pri_len = pp - (uint8_t *)p;
	p->ext_hdr_len = pri_len - offsetof(struct pdu_adv_com_ext_adv,
					    ext_hdr_adi_adv_data);

	/* set the primary PDU len */
	pri->len = pri_len;

	/* Calc secondary PDU len */
	_sec_len = _ps - (uint8_t *)_s;
	sec_len = ps - (uint8_t *)s;
	s->ext_hdr_len = sec_len - offsetof(struct pdu_adv_com_ext_adv,
					    ext_hdr_adi_adv_data);

	/* TODO: Check AdvData overflow */

	/* Fill AdvData in secondary PDU */
	memcpy(ps, data, len);

	/* set the secondary PDU len */
	sec->len = sec_len + len;

	/* Start filling primary PDU extended header  based on flags */

	/* No AdvData in primary channel PDU */

	/* No ACAD in primary channel PDU */

	/* Tx Power */
	if (hp->tx_pwr) {
		*--pp = *--_pp;
	} else if (hs->tx_pwr) {
		*--ps = *--_ps;
	}

	/* No SyncInfo in primary channel PDU */
	/* SyncInfo in secondary channel PDU */
	if (hs->sync_info) {
		_ps -= sizeof(struct ext_adv_sync_info);
		ps -= sizeof(struct ext_adv_sync_info);

		memcpy(ps, _ps, sizeof(struct ext_adv_sync_info));
	}

	/* AuxPtr */
	if (_hp.aux_ptr) {
		_pp -= sizeof(struct ext_adv_aux_ptr);
	}
	{
		struct ext_adv_aux_ptr *aux;

		pp -= sizeof(struct ext_adv_aux_ptr);

		/* NOTE: Aux Offset will be set in advertiser LLL event */
		aux = (void *)pp;
		aux->chan_idx = 0; /* FIXME: implementation defined */
		aux->ca = 0; /* FIXME: implementation defined */
		aux->offs_units = 0; /* FIXME: implementation defined */
		aux->phy = find_lsb_set(lll->phy_s) - 1;
	}
	if (_hs.aux_ptr) {
		struct ext_adv_aux_ptr *aux;

		_ps -= sizeof(struct ext_adv_aux_ptr);
		ps -= sizeof(struct ext_adv_aux_ptr);

		/* NOTE: Aux Offset will be set in advertiser LLL event */
		aux = (void *)ps;
		aux->chan_idx = 0; /* FIXME: implementation defined */
		aux->ca = 0; /* FIXME: implementation defined */
		aux->offs_units = 0; /* FIXME: implementation defined */
		aux->phy = find_lsb_set(lll->phy_s) - 1;
	}

	/* ADI */
	{
		struct ext_adv_adi *ap, *as;
		uint16_t did = UINT16_MAX;

		pp -= sizeof(struct ext_adv_adi);
		ps -= sizeof(struct ext_adv_adi);

		ap = (void *)pp;
		as = (void *)ps;

		if (_hp.adi) {
			struct ext_adv_adi *_adi;

			_pp -= sizeof(struct ext_adv_adi);
			_ps -= sizeof(struct ext_adv_adi);

			/* NOTE: memcpy shall handle overlapping buffers */
			memcpy(pp, _pp, sizeof(struct ext_adv_adi));
			memcpy(ps, _ps, sizeof(struct ext_adv_adi));

			_adi = (void *)_pp;
			did = _adi->did;
		} else {
			ap->sid = adv->sid;
			as->sid = adv->sid;
		}

		if ((op == 0x04) || len || (_pri_len != pri_len)) {
			did++;
		}

		ap->did = did;
		as->did = did;
	}

	/* No CTEInfo field in primary channel PDU */

	/* NOTE: TargetA, filled at enable and RPA timeout */

	/* No AdvA in primary channel due to AuxPtr being added */

	/* NOTE: AdvA in aux channel is also filled at enable and RPA timeout */
	if (hs->adv_addr) {
		void *bdaddr;

		if (_hs.adv_addr) {
			_ps -= BDADDR_SIZE;
			bdaddr = _ps;
		} else {
			_pp -= BDADDR_SIZE;
			bdaddr = _pp;
		}

		ps -= BDADDR_SIZE;

		memcpy(ps, bdaddr, BDADDR_SIZE);
	}

	lll_adv_aux_data_enqueue(lll_aux, is);
	lll_adv_data_enqueue(lll, ip);

	if (adv->is_enabled && !aux->is_started) {
		volatile uint32_t ret_cb;
		uint32_t ticks_anchor;
		uint32_t ret;

		ull_hdr_init(&aux->ull);
		aux->interval =	adv->interval +
				(HAL_TICKER_TICKS_TO_US(ULL_ADV_RANDOM_DELAY) /
				 625U);

		ticks_anchor = ticker_ticks_now_get();

		ret = ull_adv_aux_start(aux, ticks_anchor, &ret_cb);

		ret = ull_ticker_status_take(ret, &ret_cb);
		if (ret != TICKER_STATUS_SUCCESS) {
			/* FIXME: Use a better error code */
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		aux->is_started = 1;
	}

	return 0;
}

uint8_t ll_adv_aux_sr_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref, uint8_t len,
			    uint8_t *data)
{
	/* TODO: */
	return 0;
}

uint16_t ll_adv_aux_max_data_length_get(void)
{
	/* TODO: return a Kconfig value */
	return 0;
}
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

uint8_t ll_adv_aux_set_count_get(void)
{
	/*  TODO: return a Kconfig value */
	return 0;
}

uint8_t ll_adv_aux_set_remove(uint8_t handle)
{
	/* TODO: reset/release primary channel and Aux channel PDUs */
	return 0;
}

uint8_t ll_adv_aux_set_clear(void)
{
	/* TODO: reset/release all adv set primary channel and  Aux channel
	 * PDUs
	 */
	return 0;
}

int ull_adv_aux_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_adv_aux_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
uint16_t ull_adv_aux_lll_handle_get(struct lll_adv_aux *lll)
{
	return aux_handle_get((void *)lll->hdr.parent);
}

uint32_t ull_adv_aux_start(struct ll_adv_aux_set *aux, uint32_t ticks_anchor,
			uint32_t volatile *ret_cb)
{
	uint32_t slot_us = EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;
	uint32_t ticks_slot_overhead;
	uint8_t aux_handle;
	uint32_t ret;

	/* TODO: Calc AUX_ADV_IND slot_us */
	slot_us += 1000;

	/* TODO: active_to_start feature port */
	aux->evt.ticks_active_to_start = 0;
	aux->evt.ticks_xtal_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	aux->evt.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	aux->evt.ticks_slot = HAL_TICKER_US_TO_TICKS(slot_us);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(aux->evt.ticks_active_to_start,
					  aux->evt.ticks_xtal_to_start);
	} else {
		ticks_slot_overhead = 0;
	}

	aux_handle = aux_handle_get(aux);

	*ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_AUX_BASE + aux_handle),
			   ticks_anchor, 0,
			   HAL_TICKER_US_TO_TICKS((uint64_t)aux->interval *
						  625),
			   TICKER_NULL_REMAINDER, TICKER_NULL_LAZY,
			   (aux->evt.ticks_slot + ticks_slot_overhead),
			   ticker_cb, aux,
			   ull_ticker_status_give, (void *)ret_cb);

	return ret;
}

uint8_t ull_adv_aux_stop(struct ll_adv_aux_set *aux)
{
	volatile uint32_t ret_cb = TICKER_STATUS_BUSY;
	uint8_t aux_handle;
	void *mark;
	uint32_t ret;

	mark = ull_disable_mark(aux);
	LL_ASSERT(mark == aux);

	aux_handle = aux_handle_get(aux);

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			  TICKER_ID_ADV_AUX_BASE + aux_handle,
			  ull_ticker_status_give, (void *)&ret_cb);

	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret) {
		mark = ull_disable_mark(aux);
		LL_ASSERT(mark == aux);

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	ret = ull_disable(&aux->lll);
	LL_ASSERT(!ret);

	mark = ull_disable_unmark(aux);
	LL_ASSERT(mark == aux);

	aux->is_started = 0U;

	return 0;
}

void ull_adv_aux_offset_get(struct ll_adv_set *adv)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mfy_aux_offset_get};
	uint32_t ret;

	mfy.param = adv;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1,
			     &mfy);
	LL_ASSERT(!ret);
}

static int init_reset(void)
{
	/* Initialize adv aux pool. */
	mem_init(ll_adv_aux_pool, sizeof(struct ll_adv_aux_set),
		 sizeof(ll_adv_aux_pool) / sizeof(struct ll_adv_aux_set),
		 &adv_aux_free);

	return 0;
}

static inline struct ll_adv_aux_set *aux_acquire(void)
{
	return mem_acquire(&adv_aux_free);
}

static inline void aux_release(struct ll_adv_aux_set *aux)
{
	mem_release(aux, &adv_aux_free);
}

static inline uint16_t aux_handle_get(struct ll_adv_aux_set *aux)
{
	return mem_index_get(aux, ll_adv_aux_pool,
			     sizeof(struct ll_adv_aux_set));
}

static void mfy_aux_offset_get(void *param)
{
	struct ll_adv_set *adv = param;
	struct ll_adv_aux_set *aux;
	uint32_t ticks_to_expire;
	uint32_t ticks_current;
	struct pdu_adv *pdu;
	uint8_t ticker_id;
	uint8_t retry;
	uint8_t id;

	aux = (void *)HDR_LLL2EVT(adv->lll.aux);
	ticker_id = TICKER_ID_ADV_AUX_BASE + aux_handle_get(aux);

	id = TICKER_NULL;
	ticks_to_expire = 0U;
	ticks_current = 0U;
	retry = 4U;
	do {
		uint32_t volatile ret_cb = TICKER_STATUS_BUSY;
		uint32_t ticks_previous;
		uint32_t ret;

		ticks_previous = ticks_current;

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
	aux->lll.ticks_offset = ticks_to_expire + 1;

	pdu = lll_adv_data_curr_get(&adv->lll);
	lll_adv_aux_offset_fill(ticks_to_expire, 0, pdu);
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_adv_aux_prepare};
	static struct lll_prepare_param p;
	struct ll_adv_aux_set *aux = param;
	struct lll_adv_aux *lll;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_A(1);

	lll = &aux->lll;

	/* Increment prepare reference count */
	ref = ull_ref_inc(&aux->ull);
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

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	struct ll_adv_set *adv;

	adv = (void *)HDR_LLL2EVT(lll->adv);
	if (adv->lll.sync) {
		struct ll_adv_sync_set *sync;

		sync  = (void *)HDR_LLL2EVT(adv->lll.sync);
		if (sync->is_started) {
			ull_adv_sync_offset_get(adv);
		}
	}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
	DEBUG_RADIO_PREPARE_A(1);
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}
#else /* !(CONFIG_BT_CTLR_ADV_AUX_SET > 0) */

static int init_reset(void)
{
	return 0;
}
#endif /* !(CONFIG_BT_CTLR_ADV_AUX_SET > 0) */
