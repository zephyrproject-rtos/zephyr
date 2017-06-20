/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>
#include <bluetooth/hci.h>

#include "util/util.h"
#include "util/mem.h"

#include "pdu.h"
#include "ctrl.h"
#include "ll.h"
#include "ll_adv.h"
#include "ll_filter.h"

#define ADDR_TYPE_ANON 0xFF

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_DEBUG_HCI_DRIVER)
#include "common/log.h"

#include "hal/debug.h"
#include "pdu.h"

static struct ll_filter wl;
u8_t wl_anon;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PRIVACY)
#include "common/rpa.h"

static u8_t rl_enable;
static struct rl_dev {
	u8_t      taken:1;
	u8_t      rpas_ready:1;
	u8_t      pirk:1;
	u8_t      pirk_idx:4;
	u8_t      lirk:1;

	u8_t      peer_id_addr_type:1;
	bt_addr_t peer_id_addr;
	u8_t      local_irk[16];
	bt_addr_t peer_rpa;
	bt_addr_t local_rpa;

} rl[CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE];

static u8_t peer_irks[CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE][16];
static u8_t peer_irk_count;

#define DEFAULT_RPA_TIMEOUT_MS 900 * 1000
u32_t rpa_timeout_ms;
s64_t rpa_last_ms;

struct k_delayed_work rpa_work;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PRIVACY */

static void filter_clear(struct ll_filter *filter)
{
	filter->enable_bitmask = 0;
	filter->addr_type_bitmask = 0;
}

static u32_t filter_add(struct ll_filter *filter, u8_t addr_type, u8_t *bdaddr)
{
	u8_t index;

	if (filter->enable_bitmask == 0xFF) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	for (index = 0;
	     (filter->enable_bitmask & (1 << index));
	     index++) {
	}
	filter->enable_bitmask |= (1 << index);
	filter->addr_type_bitmask |= ((addr_type & 0x01) << index);
	memcpy(&filter->bdaddr[index][0], bdaddr, BDADDR_SIZE);

	return 0;
}

static u32_t filter_remove(struct ll_filter *filter, u8_t addr_type,
			   u8_t *bdaddr)
{
	u8_t index;

	if (!filter->enable_bitmask) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	index = 8;
	while (index--) {
		if ((filter->enable_bitmask & BIT(index)) &&
		    (((filter->addr_type_bitmask >> index) & 0x01) ==
		     (addr_type & 0x01)) &&
		    !memcmp(filter->bdaddr[index], bdaddr, BDADDR_SIZE)) {
			filter->enable_bitmask &= ~BIT(index);
			filter->addr_type_bitmask &= ~BIT(index);
			return 0;
		}
	}

	return BT_HCI_ERR_INVALID_PARAM;
}


struct ll_filter *ctrl_filter_get(void)
{
	return &wl;
}

u32_t ll_wl_size_get(void)
{
	return WL_SIZE;
}

u32_t ll_wl_clear(void)
{
	if (radio_adv_filter_pol_get() || (radio_scan_filter_pol_get() & 0x1)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	filter_clear(&wl);
	wl_anon = 0;

	return 0;
}

u32_t ll_wl_add(bt_addr_le_t *addr)
{

	if (radio_adv_filter_pol_get() || (radio_scan_filter_pol_get() & 0x1)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (addr->type == ADDR_TYPE_ANON) {
		wl_anon = 1;
		return 0;
	}

	return filter_add(&wl, addr->type, addr->a.val);
}

u32_t ll_wl_remove(bt_addr_le_t *addr)
{

	if (radio_adv_filter_pol_get() || (radio_scan_filter_pol_get() & 0x1)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (addr->type == ADDR_TYPE_ANON) {
		wl_anon = 0;
		return 0;
	}

	return filter_remove(&wl, addr->type, addr->a.val);
}

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PRIVACY)

#define RL_MATCH(i, id_addr_type, id_addr) (rl[i].taken && \
		    rl[i].peer_id_addr_type == (id_addr_type & 0x1) && \
		    !memcmp(rl[i].peer_id_addr.val, id_addr, BDADDR_SIZE))


int ll_rl_idx_find(u8_t id_addr_type, u8_t *id_addr)
{
	int i;
	for (i = 0; i < CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE; i++) {
		if (RL_MATCH(i, id_addr_type, id_addr)) {
			return i;
		}
	}

	return -1;
}

bool ctrl_rl_enabled(void)
{
	return rl_enable;
}

void ll_rl_pdu_adv_update(int idx, struct pdu_adv *pdu)
{
	u8_t *adva = pdu->type == PDU_ADV_TYPE_SCAN_RSP ?
				  &pdu->payload.scan_rsp.addr[0] :
				  &pdu->payload.adv_ind.addr[0];

	struct ll_adv_set *ll_adv = ll_adv_set_get();

	/* AdvA */
	if (idx >= 0 && rl[idx].lirk) {
		LL_ASSERT(rl[idx].rpas_ready);
		pdu->tx_addr = 1;
		memcpy(adva, rl[idx].local_rpa.val, BDADDR_SIZE);
	} else {
		pdu->tx_addr = ll_adv->own_addr_type & 0x1;
		ll_addr_get(ll_adv->own_addr_type & 0x1, adva);
	}

	/* TargetA */
	if (pdu->type == PDU_ADV_TYPE_DIRECT_IND) {
		if (idx >= 0 && rl[idx].pirk) {
			pdu->rx_addr = 1;
			memcpy(&pdu->payload.direct_ind.tgt_addr[0],
			       rl[idx].peer_rpa.val, BDADDR_SIZE);
		} else {
			pdu->rx_addr = ll_adv->id_addr_type;
			memcpy(&pdu->payload.direct_ind.tgt_addr[0],
			       ll_adv->id_addr, BDADDR_SIZE);
		}
	}
}

static void rpa_adv_refresh(void)
{
	struct radio_adv_data *radio_adv_data;
	struct ll_adv_set *ll_adv;
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	u8_t last;
	int idx;

	ll_adv = ll_adv_set_get();

	if (ll_adv->own_addr_type < BT_ADDR_LE_PUBLIC_ID) {
		return;
	}

	radio_adv_data = radio_adv_data_get();
	prev = (struct pdu_adv *)&radio_adv_data->data[radio_adv_data->last][0];
	/* use the last index in double buffer, */
	if (radio_adv_data->first == radio_adv_data->last) {
		last = radio_adv_data->last + 1;
		if (last == DOUBLE_BUFFER_SIZE) {
			last = 0;
		}
	} else {
		last = radio_adv_data->last;
	}

	/* update adv pdu fields. */
	pdu = (struct pdu_adv *)&radio_adv_data->data[last][0];
	pdu->type = prev->type;
	pdu->rfu = 0;

	if (IS_ENABLED(CONFIG_BLUETOOTH_CONTROLLER_CHAN_SEL_2)) {
		pdu->chan_sel = prev->chan_sel;
	} else {
		pdu->chan_sel = 0;
	}


	idx = ll_rl_idx_find(ll_adv->id_addr_type, ll_adv->id_addr);
	LL_ASSERT(idx >= 0);
	ll_rl_pdu_adv_update(idx, pdu);

	memcpy(&pdu->payload.adv_ind.data[0], &prev->payload.adv_ind.data[0],
	       prev->len - BDADDR_SIZE);
	pdu->len = prev->len;;

	/* commit the update so controller picks it. */
	radio_adv_data->last = last;
}

static void rl_clear(void)
{
	for (int i = 0; i < CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE; i++) {
		rl[i].taken = 0;
	}
}

static int rl_access_check(bool check_ar)
{
	if (check_ar) {
		/* If address resolution is disabled, allow immediately */
		if (!rl_enable) {
			return -1;
		}
	}

	return (radio_adv_is_enabled() || radio_scan_is_enabled()) ? 0 : 1;
}


void ll_rl_rpa_update(bool timeout)
{
	int i, err;
	s64_t now = k_uptime_get();
	bool all = timeout || (rpa_last_ms == -1) ||
		   (now - rpa_last_ms >= rpa_timeout_ms);
	BT_DBG("");

	for (i = 0; i < CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE; i++) {
		if ((rl[i].taken) && (all || !rl[i].rpas_ready)) {

			if (rl[i].pirk) {
				err = bt_rpa_create(peer_irks[rl[i].pirk_idx],
						    &rl[i].peer_rpa);
				LL_ASSERT(!err);
			}
			if (rl[i].lirk) {
				err = bt_rpa_create(rl[i].local_irk,
						    &rl[i].local_rpa);
				LL_ASSERT(!err);
			}

			rl[i].rpas_ready = 1;
		}
	}

	if (all) {
		rpa_last_ms = now;
	}

	if (timeout) {
		if (radio_adv_is_enabled()) {
			rpa_adv_refresh();
		}
	}
}

static void rpa_timeout(struct k_work *work)
{
	ll_rl_rpa_update(true);
	k_delayed_work_submit(&rpa_work, rpa_timeout_ms);
}

static void rpa_refresh_start(void)
{
	if (!rl_enable) {
		return;
	}

	BT_DBG("");
	k_delayed_work_submit(&rpa_work, rpa_timeout_ms);
}

static void rpa_refresh_stop(void)
{
	if (!rl_enable) {
		return;
	}

	k_delayed_work_cancel(&rpa_work);
}

void ll_adv_scan_state_cb(u8_t bm)
{
	if (bm) {
		rpa_refresh_start();
	} else {
		rpa_refresh_stop();
	}
}


u32_t ll_rl_size_get(void)
{
	return CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE;
}

u32_t ll_rl_clear(void)
{
	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	rl_clear();

	return 0;
}

u32_t ll_rl_add(bt_addr_le_t *id_addr, const u8_t pirk[16],
		const u8_t lirk[16])
{
	int i;

	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* find an empty slot and insert device */
	for (i = 0; i < CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE; i++) {
		if (!rl[i].taken) {
			bt_addr_copy(&rl[i].peer_id_addr,
					&id_addr->a);
			rl[i].peer_id_addr_type = id_addr->type & 0x1;
			rl[i].pirk = mem_nz((u8_t *)pirk, 16);
			rl[i].lirk = mem_nz((u8_t *)lirk, 16);
			if (rl[i].pirk) {
				rl[i].pirk_idx = peer_irk_count;
				memcpy(peer_irks[peer_irk_count++],
				       pirk, 16);
			}
			if (rl[i].lirk) {
				memcpy(rl[i].local_irk, lirk, 16);
			}
			rl[i].rpas_ready = 0;
			rl[i].taken = 1;
			break;
		}
	}
	return (i < CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE) ?
		0x00 : BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;

}

u32_t ll_rl_remove(bt_addr_le_t *id_addr)
{
	int i;

	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* find the device and mark it as empty */
	i = ll_rl_idx_find(id_addr->type, id_addr->a.val);
	if (i >= 0) {
		if (rl[i].pirk) {
			uint8_t idx = rl[i].pirk_idx;
			memmove(peer_irks[idx], peer_irks[idx + 1],
				16 * peer_irk_count--);
		}
		rl[i].taken = 0;
	}

	return (i >= 0) ?  0x00 : BT_HCI_ERR_UNKNOWN_CONN_ID;
}

u32_t ll_rl_prpa_get(bt_addr_le_t *id_addr, bt_addr_t *prpa)
{
	int i;

	/* find the device and return its RPA */
	i = ll_rl_idx_find(id_addr->type, id_addr->a.val);
	if (i >= 0) {
		bt_addr_copy(prpa, &rl[i].peer_rpa);
	}

	return (i >= 0) ?  0x00 : BT_HCI_ERR_UNKNOWN_CONN_ID;

}

u32_t ll_rl_lrpa_get(bt_addr_le_t *id_addr, bt_addr_t *lrpa)
{
	int i;

	/* find the device and return the local RPA */
	i = ll_rl_idx_find(id_addr->type, id_addr->a.val);
	if (i >= 0) {
		bt_addr_copy(lrpa, &rl[i].local_rpa);
	}

	return (i >= 0) ?  0x00 : BT_HCI_ERR_UNKNOWN_CONN_ID;
}

u32_t ll_rl_enable(u8_t enable)
{
	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	switch (enable) {
	case BT_HCI_ADDR_RES_DISABLE:
		rl_enable = 0;
		break;
	case BT_HCI_ADDR_RES_ENABLE:
		rl_enable = 1;
		break;
	default:
		return BT_HCI_ERR_INVALID_PARAM;
	}

	return 0;
}

void ll_rl_timeout_set(u16_t timeout)
{
	rpa_timeout_ms = timeout * 1000;
}
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PRIVACY */

void ll_filter_reset(bool init)
{
	filter_clear(&wl);
	wl_anon = 0;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PRIVACY)
	rl_enable = 0;
	rpa_timeout_ms = DEFAULT_RPA_TIMEOUT_MS;
	rpa_last_ms = -1;
	rl_clear();
	if (init) {
		k_delayed_work_init(&rpa_work, rpa_timeout);
	} else {
		k_delayed_work_cancel(&rpa_work);
	}
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PRIVACY */

}
