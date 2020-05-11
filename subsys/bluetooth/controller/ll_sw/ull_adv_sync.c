/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
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
#include "lll_adv_sync.h"
#include "lll_adv_internal.h"

#include "ull_adv_types.h"

#include "ull_internal.h"
#include "ull_chan_internal.h"
#include "ull_adv_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_adv_sync
#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static int init_reset(void);
static inline struct ll_adv_sync_set *sync_acquire(void);
static inline void sync_release(struct ll_adv_sync_set *sync);
static inline uint16_t sync_handle_get(struct ll_adv_sync_set *sync);
static inline uint8_t sync_stop(struct ll_adv_sync_set *sync);
static void mfy_sync_offset_get(void *param);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
		      uint16_t lazy, void *param);
static void ticker_op_cb(uint32_t status, void *param);

static struct ll_adv_sync_set ll_adv_sync_pool[CONFIG_BT_CTLR_ADV_SYNC_SET];
static void *adv_sync_free;

uint8_t ll_adv_sync_param_set(uint8_t handle, uint16_t interval, uint16_t flags)
{
	struct lll_adv_sync *lll_sync;
	struct pdu_adv_com_ext_adv *t;
	struct ll_adv_sync_set *sync;
	struct ext_adv_hdr *ht, _ht;
	struct ll_adv_set *adv;
	struct pdu_adv *pdu;
	uint8_t *_pt, *pt;
	uint8_t ter_len;

	adv = ull_adv_is_created_get(handle);
	if (!adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		struct pdu_adv_com_ext_adv *p, *_p, *s, *_s;
		uint8_t pri_len, _pri_len, sec_len, _sec_len;
		struct pdu_adv *_pri, *pri, *_sec, *sec;
		struct ext_adv_hdr *hp, _hp, *hs, _hs;
		struct ext_adv_sync_info *si;
		struct lll_adv_aux *lll_aux;
		uint8_t *_pp, *pp, *ps, *_ps;
		uint8_t ip, is, ad_len;
		struct lll_adv *lll;
		int err;

		sync = sync_acquire();
		if (!sync) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		lll = &adv->lll;
		lll_aux = lll->aux;
		if (!lll_aux) {
		}

		lll_sync = &sync->lll;
		lll->sync = lll_sync;
		lll_sync->adv = lll;

		/* NOTE: ull_hdr_init(&sync->ull); is done on start */
		lll_hdr_init(lll_sync, sync);

		err = util_aa_le32(lll_sync->access_addr);
		LL_ASSERT(!err);

		util_rand(lll_sync->crc_init, sizeof(lll_sync->crc_init));

		lll_sync->latency_prepare = 0;
		lll_sync->latency_event = 0;
		lll_sync->event_counter = 0;

		lll_sync->data_chan_count =
			ull_chan_map_get(lll_sync->data_chan_map);
		lll_sync->data_chan_id = 0;

		/* Get reference to previous primary PDU data */
		_pri = lll_adv_data_peek(lll);
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
		/* NOTE: as we will use auxiliary packet, we remove AdvA in
		 * primary channel. i.e. Do nothing to add AdvA in the primary
		 * PDU.
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
		/* Add SyncInfo flag in secondary channel PDU */
		hs->sync_info = 1;
		ps += sizeof(*si);

		/* Tx Power flag */
		if (_hp.tx_pwr) {
			_pp++;

			/* C1, Tx Power is optional on the LE 1M PHY, and
			 * reserved for future use on the LE Coded PHY.
			 */
			if (adv->lll.phy_p != BIT(2)) {
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

		/* TODO: ACAD place holder */

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
		ad_len = _sec->len - _sec_len;

		/* set the secondary PDU len */
		sec->len = sec_len + ad_len;

		/* Fill AdvData in secondary PDU */
		memcpy(ps, _ps, ad_len);

		/* Start filling primary PDU payload based on flags */

		/* No AdvData in primary channel PDU */

		/* No ACAD in primary channel PDU */

		/* Tx Power */
		if (hp->tx_pwr) {
			*--pp = *--_pp;
		} else if (hs->tx_pwr) {
			*--ps = *--_ps;
		}

		/* No SyncInfo in primary channel PDU */
		/* Fill SyncInfo in secondary channel PDU */
		ps -= sizeof(*si);
		si = (void *)ps;
		si->offs = 0; /* NOTE: Filled by secondary prepare */
		si->offs_units = 0;
		si->interval = interval;
		memcpy(si->sca_chm, lll_sync->data_chan_map,
		       sizeof(si->sca_chm));
		memcpy(&si->aa, lll_sync->access_addr, sizeof(si->aa));
		memcpy(si->crc_init, lll_sync->crc_init, sizeof(si->crc_init));

		si->evt_cntr = 0; /* TODO: Implementation defined */

		/* AuxPtr */
		if (_hp.aux_ptr) {
			_pp -= sizeof(struct ext_adv_aux_ptr);
		}
		{
			struct ext_adv_aux_ptr *aux;

			pp -= sizeof(struct ext_adv_aux_ptr);

			/* NOTE: Aux Offset will be set in advertiser LLL event
			 */
			aux = (void *)pp;
			aux->chan_idx = 0; /* FIXME: implementation defined */
			aux->ca = 0; /* FIXME: implementation defined */
			aux->offs_units = 0; /* FIXME: implementation defined */
			aux->phy = find_lsb_set(adv->lll.phy_s) - 1;
		}

		/* TODO: reduce duplicate code if below remains similar to
		 * primary PDU
		 */
		if (_hs.aux_ptr) {
			struct ext_adv_aux_ptr *aux;

			_ps -= sizeof(struct ext_adv_aux_ptr);
			ps -= sizeof(struct ext_adv_aux_ptr);

			/* NOTE: Aux Offset will be set in advertiser LLL event
			 */
			aux = (void *)ps;
			aux->chan_idx = 0; /* FIXME: implementation defined */
			aux->ca = 0; /* FIXME: implementation defined */
			aux->offs_units = 0; /* FIXME: implementation defined */
			aux->phy = find_lsb_set(adv->lll.phy_s) - 1;
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

				/* NOTE: memcpy shall handle overlapping buffers
				 */
				memcpy(pp, _pp, sizeof(struct ext_adv_adi));
				memcpy(ps, _ps, sizeof(struct ext_adv_adi));

				_adi = (void *)_pp;
				did = _adi->did;
			} else {
				ap->sid = adv->sid;
				as->sid = adv->sid;
			}

			did++;

			ap->did = did;
			as->did = did;
		}

		/* No CTEInfo field in primary channel PDU */

		/* No TargetA non-conn non-scan advertising  */

		/* No AdvA in primary channel due to AuxPtr being added */

		/* NOTE: AdvA in aux channel is also filled at enable and RPA
		 * timeout
		 */
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
	} else {
		sync = (void *)HDR_LLL2EVT(lll_sync);
	}

	sync->interval = interval;

	pdu = lll_adv_sync_data_peek(lll_sync);
	pdu->type = PDU_ADV_TYPE_AUX_SYNC_IND;
	pdu->rfu = 0U;
	pdu->chan_sel = 0U;
	pdu->tx_addr = 0U;
	pdu->rx_addr = 0U;

	t = (void *)&pdu->adv_ext_ind;
	ht = (void *)t->ext_hdr_adi_adv_data;
	pt = (uint8_t *)ht + sizeof(*ht);
	*(uint8_t *)&_ht = *(uint8_t *)ht;
	*(uint8_t *)ht = 0;
	_pt = pt;

	/* Non-connectable and Non-scannable adv mode */
	t->adv_mode = 0;

	/* No AdvA */
	/* No TargetA */

	/* TODO: CTEInfo */

	/* No ADI */

	/* TODO: AuxPtr */

	/* No SyncInfo */

	/* TODO: TxPower */
	if (flags & BIT(6)) {
		/* TODO: add/remove Tx Power in AUX_SYNC_IND PDU */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* TODO: ACAD */

	/* TODO: AdvData */

	/* Calc primary PDU len */
	ter_len = pt - (uint8_t *)t;
	if (ter_len >
	    (offsetof(struct pdu_adv_com_ext_adv, ext_hdr_adi_adv_data) +
	     sizeof(*ht))) {
		t->ext_hdr_len = ter_len - offsetof(struct pdu_adv_com_ext_adv,
						    ext_hdr_adi_adv_data);
		pdu->len = ter_len;
	} else {
		t->ext_hdr_len = 0;
		pdu->len = offsetof(struct pdu_adv_com_ext_adv,
				    ext_hdr_adi_adv_data);
	}

	return 0;
}

uint8_t ll_adv_sync_ad_data_set(uint8_t handle, uint8_t op, uint8_t frag_pref,
				uint8_t len, uint8_t const *const data)
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
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	lll_sync = adv->lll.sync;
	if (!lll_sync) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	sync = (void *)HDR_LLL2EVT(lll_sync);

	if (!enable) {
		uint8_t err;

		err = sync_stop(sync);
		if (err) {
			return err;
		}

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

	if (!sync->is_started) {
		/* TODO: */
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
	volatile uint32_t ret_cb = TICKER_STATUS_BUSY;
	uint8_t sync_handle;
	void *mark;
	uint32_t ret;

	mark = ull_disable_mark(sync);
	LL_ASSERT(mark == sync);

	sync_handle = sync_handle_get(sync);

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

	sync->is_started = 0U;

	return 0;
}

static void mfy_sync_offset_get(void *param)
{
	struct ll_adv_set *adv = param;
	struct ll_adv_sync_set *sync;
	uint32_t ticks_to_expire;
	uint32_t ticks_current;
	struct pdu_adv *pdu;
	uint8_t ticker_id;
	uint8_t retry;
	uint8_t id;

	sync = (void *)HDR_LLL2EVT(adv->lll.sync);
	ticker_id = TICKER_ID_ADV_SYNC_BASE + sync_handle_get(sync);

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
	sync->lll.ticks_offset = ticks_to_expire + 1;

	pdu = lll_adv_aux_data_curr_get(adv->lll.aux);
	lll_adv_sync_offset_fill(ticks_to_expire, 0, pdu);
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
