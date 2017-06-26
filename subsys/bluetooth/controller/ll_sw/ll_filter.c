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

/* Hardware whitelist */
static struct ll_filter wl;
u8_t wl_anon;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PRIVACY)
#include "common/rpa.h"

#define IDX_NONE 0xF

/* Whitelist peer list */
static struct {
	u8_t      taken:1;
	u8_t      rl_idx:4;
	u8_t      id_addr_type:1;
	bt_addr_t id_addr;
} wl_peers[WL_SIZE];

static u8_t rl_enable;
static struct rl_dev {
	u8_t      taken:1;
	u8_t      rpas_ready:1;
	u8_t      pirk:1;
	u8_t      pirk_idx:3;
	u8_t      lirk:1;
	u8_t      dev:1;
	u8_t      wl:1;

	u8_t      id_addr_type:1;
	bt_addr_t id_addr;

	u8_t      local_irk[16];
	bt_addr_t peer_rpa;
	bt_addr_t local_rpa;

} rl[CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE];

static u8_t peer_irks[CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE][16];
static u8_t peer_irk_rl_ids[CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE];
static u8_t peer_irk_count;

/* Hardware filter for the resolving list */
static struct ll_filter rl_filter;

#define DEFAULT_RPA_TIMEOUT_MS (900 * 1000)
u32_t rpa_timeout_ms;
s64_t rpa_last_ms;

struct k_delayed_work rpa_work;

#define LIST_MATCH(list, i, type, addr) (list[i].taken && \
		    (list[i].id_addr_type == (type & 0x1)) && \
		    !memcmp(list[i].id_addr.val, addr, BDADDR_SIZE))

static void wl_peers_clear(void)
{
	for (int i = 0; i < WL_SIZE; i++) {
		wl_peers[i].taken = 0;
	}
}

static int wl_peers_find(u8_t addr_type, u8_t *addr)
{
	int i;

	for (i = 0; i < WL_SIZE; i++) {
		if (LIST_MATCH(wl_peers, i, addr_type, addr)) {
			return i;
		}
	}

	return -1;
}

static u32_t wl_peers_add(bt_addr_le_t *id_addr)
{
	int i = wl_peers_find(id_addr->type, id_addr->a.val);

	if (i >= 0) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	for (i = 0; i < WL_SIZE; i++) {
		if (!wl_peers[i].taken) {
			int j;

			wl_peers[i].id_addr_type = id_addr->type & 0x1;
			bt_addr_copy(&wl_peers[i].id_addr, &id_addr->a);
			/* Get index to Resolving List if applicable */
			j = ll_rl_find(id_addr->type, id_addr->a.val);
			if (j >= 0) {
				wl_peers[i].rl_idx = j;
				rl[j].wl = 1;
			} else {
				wl_peers[i].rl_idx = IDX_NONE;
			}
			wl_peers[i].taken = 1;
			return 0;
		}
	}

	return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
}

static u32_t wl_peers_remove(bt_addr_le_t *id_addr)
{
	/* find the device and mark it as empty */
	int i = wl_peers_find(id_addr->type, id_addr->a.val);

	if (i >= 0) {
		int j = wl_peers[i].rl_idx;
		if (j != IDX_NONE) {
			rl[j].wl = 0;
		}
		wl_peers[i].taken = 0;
		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
}

#endif /* CONFIG_BLUETOOTH_CONTROLLER_PRIVACY */

static void filter_clear(struct ll_filter *filter)
{
	filter->enable_bitmask = 0;
	filter->addr_type_bitmask = 0;
}

static void filter_insert(struct ll_filter *filter, int index, u8_t addr_type,
			   u8_t *bdaddr)
{
	filter->enable_bitmask |= BIT(index);
	filter->addr_type_bitmask |= ((addr_type & 0x01) << index);
	memcpy(&filter->bdaddr[index][0], bdaddr, BDADDR_SIZE);
}

#if !defined(CONFIG_BLUETOOTH_CONTROLLER_PRIVACY)
static u32_t filter_add(struct ll_filter *filter, u8_t addr_type, u8_t *bdaddr)
{
	int index;

	if (filter->enable_bitmask == 0xFF) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	for (index = 0;
	     (filter->enable_bitmask & BIT(index));
	     index++) {
	}

	filter_insert(filter, index, addr_type, bdaddr);
	return 0;
}

static u32_t filter_remove(struct ll_filter *filter, u8_t addr_type,
			   u8_t *bdaddr)
{
	int index;

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
#endif

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

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PRIVACY)
	wl_peers_clear();
#else
	filter_clear(&wl);
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PRIVACY */
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

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PRIVACY)
	return wl_peers_add(addr);
#else
	return filter_add(&wl, addr->type, addr->a.val);
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PRIVACY */
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

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PRIVACY)
	return wl_peers_remove(addr);
#else
	return filter_remove(&wl, addr->type, addr->a.val);
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PRIVACY */
}

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PRIVACY)

static void filter_wl_update(void)
{
	int i;

	/* Populate filter from wl peers */
	filter_clear(&wl);

	for (i = 0; i < WL_SIZE; i++) {
		int j = wl_peers[i].rl_idx;

		if (!rl_enable || j == IDX_NONE || !rl[j].pirk || rl[j].dev) {
			filter_insert(&wl, i, wl_peers[i].id_addr_type,
				      wl_peers[i].id_addr.val);
		}
	}
}

static void filter_rl_update(void)
{
	int i;

	/* No whitelist: populate filter from rl peers */
	filter_clear(&rl_filter);

	for (i = 0; i < CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE; i++) {
		if (!rl[i].pirk || rl[i].dev) {
			filter_insert(&rl_filter, i, rl[i].id_addr_type,
				      rl[i].id_addr.val);
		}
	}
}

void ll_filters_adv_update(u8_t adv_fp)
{
	/* enabling advertising */
	if (adv_fp && !(radio_scan_filter_pol_get() & 0x1)) {
		/* whitelist not in use, update whitelist */
		filter_wl_update();
	}

	if (rl_enable && !radio_scan_is_enabled()) {
		/* rl not in use, update resolving list LUT */
		filter_rl_update();
	}
}

void ll_filters_scan_update(u8_t scan_fp)
{
	/* enabling advertising */
	if ((scan_fp & 0x1) && !radio_adv_filter_pol_get()) {
		/* whitelist not in use, update whitelist */
		filter_wl_update();
	}

	if (rl_enable && !radio_adv_is_enabled()) {
		/* rl not in use, update resolving list LUT */
		filter_rl_update();
	}
}

int ll_rl_find(u8_t id_addr_type, u8_t *id_addr)
{
	int i, free = -IDX_NONE;

	for (i = 0; i < CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE; i++) {
		if (LIST_MATCH(rl, i, id_addr_type, id_addr)) {
			return i;
		} else if (!rl[i].taken && free == -IDX_NONE) {
			free = -i;
		}
	}

	return free;
}

bool ctrl_rl_enabled(void)
{
	return rl_enable;
}

#if defined(CONFIG_BLUETOOTH_BROADCASTER)
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

	if (ll_adv->own_addr_type != BT_ADDR_LE_PUBLIC_ID &&
	    ll_adv->own_addr_type != BT_ADDR_LE_RANDOM_ID) {
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


	idx = ll_rl_find(ll_adv->id_addr_type, ll_adv->id_addr);
	LL_ASSERT(idx >= 0);
	ll_rl_pdu_adv_update(idx, pdu);

	memcpy(&pdu->payload.adv_ind.data[0], &prev->payload.adv_ind.data[0],
	       prev->len - BDADDR_SIZE);
	pdu->len = prev->len;

	/* commit the update so controller picks it. */
	radio_adv_data->last = last;
}
#endif

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
#if defined(CONFIG_BLUETOOTH_BROADCASTER)
		if (radio_adv_is_enabled()) {
			rpa_adv_refresh();
		}
#endif
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
	int i, j;

	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	i = ll_rl_find(id_addr->type, id_addr->a.val);
	if (i >= 0) {
		return BT_HCI_ERR_INVALID_PARAM;
	} else if (i == -IDX_NONE) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Device not found but empty slot found */
	i = -i;

	bt_addr_copy(&rl[i].id_addr, &id_addr->a);
	rl[i].id_addr_type = id_addr->type & 0x1;
	rl[i].pirk = mem_nz((u8_t *)pirk, 16);
	rl[i].lirk = mem_nz((u8_t *)lirk, 16);
	if (rl[i].pirk) {
		/* cross-reference */
		rl[i].pirk_idx = peer_irk_count;
		peer_irk_rl_ids[peer_irk_count] = i;
		memcpy(peer_irks[peer_irk_count++], pirk, 16);
	}
	if (rl[i].lirk) {
		memcpy(rl[i].local_irk, lirk, 16);
	}
	rl[i].rpas_ready = 0;
	/* Default to Network Privacy */
	rl[i].dev = 0;
	/* Add reference to  a whitelist entry */
	j = wl_peers_find(id_addr->type, id_addr->a.val);
	if (j >= 0) {
		wl_peers[j].rl_idx = i;
		rl[i].wl = 1;
	} else {
		rl[i].wl = 0;
	}
	rl[i].taken = 1;

	return 0;
}

u32_t ll_rl_remove(bt_addr_le_t *id_addr)
{
	int i;

	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* find the device and mark it as empty */
	i = ll_rl_find(id_addr->type, id_addr->a.val);
	if (i >= 0) {
		int j, k;

		if (rl[i].pirk) {
			/* Swap with last item */
			int pi = rl[i].pirk_idx, pj = peer_irk_count - 1;

			if (pj && pi != pj) {
				memcpy(peer_irks[pi], peer_irks[pj], 16);
				for (k = 0;
				     k < CONFIG_BLUETOOTH_CONTROLLER_RL_SIZE;
				     k++) {

					if (rl[k].taken && rl[k].pirk &&
					    rl[k].pirk_idx == pj) {
						rl[k].pirk_idx = pi;
						peer_irk_rl_ids[pi] = k;
						break;
					}
				}
			}
			peer_irk_count--;
		}

		/* Check if referenced by a whitelist entry */
		j = wl_peers_find(id_addr->type, id_addr->a.val);
		if (j >= 0) {
			wl_peers[j].rl_idx = IDX_NONE;
		}
		rl[i].taken = 0;
		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
}

u32_t ll_rl_prpa_get(bt_addr_le_t *id_addr, bt_addr_t *prpa)
{
	int i;

	/* find the device and return its RPA */
	i = ll_rl_find(id_addr->type, id_addr->a.val);
	if (i >= 0) {
		bt_addr_copy(prpa, &rl[i].peer_rpa);
		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;

}

u32_t ll_rl_lrpa_get(bt_addr_le_t *id_addr, bt_addr_t *lrpa)
{
	int i;

	/* find the device and return the local RPA */
	i = ll_rl_find(id_addr->type, id_addr->a.val);
	if (i >= 0) {
		bt_addr_copy(lrpa, &rl[i].local_rpa);
		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
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

u32_t ll_priv_mode_set(bt_addr_le_t *id_addr, u8_t mode)
{
	int i;

	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* find the device and mark it as empty */
	i = ll_rl_find(id_addr->type, id_addr->a.val);
	if (i >= 0) {
		switch (mode) {
		case BT_HCI_LE_PRIVACY_MODE_NETWORK:
			rl[i].dev = 0;
			break;
		case BT_HCI_LE_PRIVACY_MODE_DEVICE:
			rl[i].dev = 1;
			break;
		default:
			return BT_HCI_ERR_INVALID_PARAM;
		}
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
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
