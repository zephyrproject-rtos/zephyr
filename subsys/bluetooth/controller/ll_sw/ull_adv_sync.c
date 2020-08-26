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
		struct pdu_adv_com_ext_adv *pri_com_hdr, *pri_com_hdr_prev;
		struct pdu_adv_com_ext_adv *sec_com_hdr, *sec_com_hdr_prev;
		uint8_t pri_len, pri_len_prev, sec_len, sec_len_prev;
		struct pdu_adv_hdr *pri_hdr, pri_hdr_prev;
		struct pdu_adv_hdr *sec_hdr, sec_hdr_prev;
		struct pdu_adv *pri_pdu, *pri_pdu_prev;
		struct pdu_adv *sec_pdu_prev, *sec_pdu;
		uint8_t *pri_dptr, *pri_dptr_prev;
		uint8_t *sec_dptr, *sec_dptr_prev;
		struct pdu_adv_sync_info *si;
		struct lll_adv_aux *lll_aux;
		uint8_t pri_idx, sec_idx, ad_len;
		struct lll_adv *lll;
		uint8_t is_aux_new;
		int err;

		sync = sync_acquire();
		if (!sync) {
			return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
		}

		lll = &adv->lll;
		lll_aux = lll->aux;
		if (!lll_aux) {
			struct ll_adv_aux_set *aux;

			aux = ull_adv_aux_acquire(lll);
			if (!aux) {
				return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
			}

			lll_aux = &aux->lll;

			is_aux_new = 1U;
		} else {
			is_aux_new = 0U;
		}

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

		/* Get reference to previous primary PDU data */
		pri_pdu_prev = lll_adv_data_peek(lll);
		pri_com_hdr_prev = (void *)&pri_pdu_prev->adv_ext_ind;
		pri_hdr = (void *)pri_com_hdr_prev->ext_hdr_adi_adv_data;
		pri_hdr_prev = *pri_hdr;
		pri_dptr_prev = (uint8_t *)pri_hdr + sizeof(*pri_hdr);

		/* Get reference to new primary PDU data buffer */
		pri_pdu = lll_adv_data_alloc(lll, &pri_idx);
		pri_pdu->type = pri_pdu_prev->type;
		pri_pdu->rfu = 0U;
		pri_pdu->chan_sel = 0U;
		pri_com_hdr = (void *)&pri_pdu->adv_ext_ind;
		pri_com_hdr->adv_mode = pri_com_hdr_prev->adv_mode;
		pri_hdr = (void *)pri_com_hdr->ext_hdr_adi_adv_data;
		pri_dptr = (uint8_t *)pri_hdr + sizeof(*pri_hdr);
		*(uint8_t *)pri_hdr = 0U;

		/* Get reference to previous secondary PDU data */
		sec_pdu_prev = lll_adv_aux_data_peek(lll_aux);
		sec_com_hdr_prev = (void *)&sec_pdu_prev->adv_ext_ind;
		sec_hdr = (void *)sec_com_hdr_prev->ext_hdr_adi_adv_data;
		if (!is_aux_new) {
			sec_hdr_prev = *sec_hdr;
		} else {
			/* Initialize only those fields used to copy into new PDU
			 * buffer.
			 */
			sec_pdu_prev->tx_addr = 0U;
			sec_pdu_prev->rx_addr = 0U;
			sec_pdu_prev->len = offsetof(struct pdu_adv_com_ext_adv,
						     ext_hdr_adi_adv_data);
			*(uint8_t *)&sec_hdr_prev = 0U;
		}
		sec_dptr_prev = (uint8_t *)sec_hdr + sizeof(*sec_hdr);

		/* Get reference to new secondary PDU data buffer */
		sec_pdu = lll_adv_aux_data_alloc(lll_aux, &sec_idx);
		sec_pdu->type = pri_pdu->type;
		sec_pdu->rfu = 0U;
		sec_pdu->chan_sel = 0U;

		sec_pdu->tx_addr = sec_pdu_prev->tx_addr;
		sec_pdu->rx_addr = sec_pdu_prev->rx_addr;

		sec_com_hdr = (void *)&sec_pdu->adv_ext_ind;
		sec_com_hdr->adv_mode = pri_com_hdr->adv_mode;
		sec_hdr = (void *)sec_com_hdr->ext_hdr_adi_adv_data;
		sec_dptr = (uint8_t *)sec_hdr + sizeof(*sec_hdr);
		*(uint8_t *)sec_hdr = 0U;

		/* AdvA flag */
		/* NOTE: as we will use auxiliary packet, we remove AdvA in
		 * primary channel. i.e. Do nothing to add AdvA in the primary
		 * PDU.
		 */
		if (pri_hdr_prev.adv_addr) {
			pri_dptr_prev += BDADDR_SIZE;

			/* Prepare to add AdvA in secondary PDU */
			sec_hdr->adv_addr = 1;

			/* NOTE: AdvA is filled at enable */
			sec_pdu->tx_addr = pri_pdu->tx_addr;
		}
		pri_pdu->tx_addr = 0U;
		pri_pdu->rx_addr = 0U;

		if (sec_hdr_prev.adv_addr) {
			sec_dptr_prev += BDADDR_SIZE;
			sec_hdr->adv_addr = 1;
		}
		if (sec_hdr->adv_addr) {
			sec_dptr += BDADDR_SIZE;
		}

		/* No TargetA in primary and secondary channel for undirected */
		/* No CTEInfo flag in primary and secondary channel PDU */

		/* ADI flag */
		if (pri_hdr_prev.adi) {
			pri_dptr_prev += sizeof(struct pdu_adv_adi);
		}
		pri_hdr->adi = 1;
		pri_dptr += sizeof(struct pdu_adv_adi);
		if (sec_hdr_prev.adi) {
			sec_dptr_prev += sizeof(struct pdu_adv_adi);
		}
		sec_hdr->adi = 1;
		sec_dptr += sizeof(struct pdu_adv_adi);

		/* AuxPtr flag */
		if (pri_hdr_prev.aux_ptr) {
			pri_dptr_prev += sizeof(struct pdu_adv_aux_ptr);
		}
		pri_hdr->aux_ptr = 1;
		pri_dptr += sizeof(struct pdu_adv_aux_ptr);
		if (sec_hdr_prev.aux_ptr) {
			sec_dptr_prev += sizeof(struct pdu_adv_aux_ptr);

			sec_hdr->aux_ptr = 1;
			sec_dptr += sizeof(struct pdu_adv_aux_ptr);
		}

		/* No SyncInfo flag in primary channel PDU */
		/* Add SyncInfo flag in secondary channel PDU */
		sec_hdr->sync_info = 1;
		sec_dptr += sizeof(*si);

		/* Tx Power flag */
		if (pri_hdr_prev.tx_pwr) {
			pri_dptr_prev++;

			/* C1, Tx Power is optional on the LE 1M PHY, and
			 * reserved for future use on the LE Coded PHY.
			 */
			if (lll->phy_p != PHY_CODED) {
				pri_hdr->tx_pwr = 1;
				pri_dptr++;
			} else {
				sec_hdr->tx_pwr = 1;
			}
		}
		if (sec_hdr_prev.tx_pwr) {
			sec_dptr_prev++;

			sec_hdr->tx_pwr = 1;
		}
		if (sec_hdr->tx_pwr) {
			sec_dptr++;
		}

		/* TODO: ACAD place holder */

		/* Calc primary PDU len */
		pri_len_prev = pri_dptr_prev - (uint8_t *)pri_com_hdr_prev;
		pri_len = pri_dptr - (uint8_t *)pri_com_hdr;
		pri_com_hdr->ext_hdr_len = pri_len -
					   offsetof(struct pdu_adv_com_ext_adv,
						    ext_hdr_adi_adv_data);

		/* set the primary PDU len */
		pri_pdu->len = pri_len;

		/* Calc previous secondary PDU len */
		sec_len_prev = sec_dptr_prev - (uint8_t *)sec_com_hdr_prev;
		if (sec_len_prev <= (offsetof(struct pdu_adv_com_ext_adv,
					      ext_hdr_adi_adv_data) +
				     sizeof(sec_hdr_prev))) {
			sec_len_prev = offsetof(struct pdu_adv_com_ext_adv,
						ext_hdr_adi_adv_data);
		}

		/* Did we parse beyond PDU length? */
		if (sec_len_prev > sec_pdu_prev->len) {
			/* we should not encounter invalid length */
			/* FIXME: release allocations */
			return BT_HCI_ERR_UNSPECIFIED;
		}

		/* Calc current secondary PDU len */
		sec_len = sec_dptr - (uint8_t *)sec_com_hdr;
		if (sec_len > (offsetof(struct pdu_adv_com_ext_adv,
					ext_hdr_adi_adv_data) +
			       sizeof(*sec_hdr))) {
			sec_com_hdr->ext_hdr_len =
				sec_len - offsetof(struct pdu_adv_com_ext_adv,
						   ext_hdr_adi_adv_data);
		} else {
			sec_com_hdr->ext_hdr_len = 0;
			sec_len = offsetof(struct pdu_adv_com_ext_adv,
					   ext_hdr_adi_adv_data);
		}

		/* Calc the previous AD data length in auxiliary PDU */
		ad_len = sec_pdu_prev->len - sec_len_prev;

		/* set the secondary PDU len */
		sec_pdu->len = sec_len + ad_len;

		/* Check AdvData overflow */
		if (sec_pdu->len > CONFIG_BT_CTLR_ADV_DATA_LEN_MAX) {
			/* FIXME: release allocations */
			return BT_HCI_ERR_PACKET_TOO_LONG;
		}

		/* Fill AdvData in secondary PDU */
		memcpy(sec_dptr, sec_dptr_prev, ad_len);

		/* Start filling primary PDU payload based on flags */

		/* No AdvData in primary channel PDU */

		/* No ACAD in primary channel PDU */

		/* Tx Power */
		if (pri_hdr->tx_pwr) {
			*--pri_dptr = *--pri_dptr_prev;
		} else if (sec_hdr->tx_pwr) {
			*--sec_dptr = *--sec_dptr_prev;
		}

		/* No SyncInfo in primary channel PDU */
		/* Fill SyncInfo in secondary channel PDU */
		sec_dptr -= sizeof(*si);
		si = (void *)sec_dptr;
		si->offs = 0U; /* NOTE: Filled by secondary prepare */
		si->offs_units = 0U;
		si->interval = sys_cpu_to_le16(interval);
		memcpy(si->sca_chm, lll_sync->data_chan_map,
		       sizeof(si->sca_chm));
		memcpy(&si->aa, lll_sync->access_addr, sizeof(si->aa));
		memcpy(si->crc_init, lll_sync->crc_init, sizeof(si->crc_init));

		si->evt_cntr = 0U; /* TODO: Implementation defined */

		/* AuxPtr */
		if (pri_hdr_prev.aux_ptr) {
			pri_dptr_prev -= sizeof(struct pdu_adv_aux_ptr);
		}
		{
			struct pdu_adv_aux_ptr *aux_ptr;

			pri_dptr -= sizeof(struct pdu_adv_aux_ptr);

			/* NOTE: Aux Offset will be set in advertiser LLL event
			 */
			aux_ptr = (void *)pri_dptr;

			/* FIXME: implementation defined */
			aux_ptr->chan_idx = 0U;
			aux_ptr->ca = 0U;
			aux_ptr->offs_units = 0U;

			aux_ptr->phy = find_lsb_set(lll->phy_s) - 1;
		}

		/* TODO: reduce duplicate code if below remains similar to
		 * primary PDU
		 */
		if (sec_hdr_prev.aux_ptr) {
			struct pdu_adv_aux_ptr *aux_ptr;

			sec_dptr_prev -= sizeof(struct pdu_adv_aux_ptr);
			sec_dptr -= sizeof(struct pdu_adv_aux_ptr);

			/* NOTE: Aux Offset will be set in advertiser LLL event
			 */
			aux_ptr = (void *)sec_dptr;

			/* FIXME: implementation defined */
			aux_ptr->chan_idx = 0U;
			aux_ptr->ca = 0U;
			aux_ptr->offs_units = 0U;

			aux_ptr->phy = find_lsb_set(lll->phy_s) - 1;
		}

		/* ADI */
		{
			struct pdu_adv_adi *pri_adi, *sec_adi;
			uint16_t did = UINT16_MAX;

			pri_dptr -= sizeof(struct pdu_adv_adi);
			sec_dptr -= sizeof(struct pdu_adv_adi);

			pri_adi = (void *)pri_dptr;
			sec_adi = (void *)sec_dptr;

			if (pri_hdr_prev.adi) {
				struct pdu_adv_adi *pri_adi_prev;

				pri_dptr_prev -= sizeof(struct pdu_adv_adi);
				sec_dptr_prev -= sizeof(struct pdu_adv_adi);

				/* NOTE: memcpy shall handle overlapping buffers
				 */
				memcpy(pri_dptr, pri_dptr_prev,
				       sizeof(struct pdu_adv_adi));
				memcpy(sec_dptr, sec_dptr_prev,
				       sizeof(struct pdu_adv_adi));

				pri_adi_prev = (void *)pri_dptr_prev;
				did = sys_le16_to_cpu(pri_adi_prev->did);
			} else {
				pri_adi->sid = adv->sid;
				sec_adi->sid = adv->sid;
			}

			did++;

			pri_adi->did = sys_cpu_to_le16(did);
			sec_adi->did = sys_cpu_to_le16(did);
		}

		/* No CTEInfo field in primary channel PDU */

		/* No TargetA non-conn non-scan advertising  */

		/* No AdvA in primary channel due to AuxPtr being added */

		/* NOTE: AdvA in aux channel is also filled at enable and RPA
		 * timeout
		 */
		if (sec_hdr->adv_addr) {
			void *bdaddr;

			if (sec_hdr_prev.adv_addr) {
				sec_dptr_prev -= BDADDR_SIZE;
				bdaddr = sec_dptr_prev;
			} else {
				pri_dptr_prev -= BDADDR_SIZE;
				bdaddr = pri_dptr_prev;
			}

			sec_dptr -= BDADDR_SIZE;

			memcpy(sec_dptr, bdaddr, BDADDR_SIZE);
		}

		lll_adv_aux_data_enqueue(lll_aux, sec_idx);
		lll_adv_data_enqueue(lll, pri_idx);
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
	ter_len = ter_dptr - (uint8_t *)ter_com_hdr;
	if (ter_len >
	    (offsetof(struct pdu_adv_com_ext_adv, ext_hdr_adi_adv_data) +
	     sizeof(*ter_hdr))) {
		ter_com_hdr->ext_hdr_len = ter_len -
					   offsetof(struct pdu_adv_com_ext_adv,
						    ext_hdr_adi_adv_data);
		ter_pdu->len = ter_len;
	} else {
		ter_com_hdr->ext_hdr_len = 0U;
		ter_pdu->len = offsetof(struct pdu_adv_com_ext_adv,
				    ext_hdr_adi_adv_data);
	}

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

		/* TODO: remove sync_info from auxiliary PDU */

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
