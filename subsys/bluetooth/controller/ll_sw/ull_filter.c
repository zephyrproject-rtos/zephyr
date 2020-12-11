/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>
#include <soc.h>
#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "pdu.h"
#include "ll.h"

#include "lll.h"
#include "lll_adv.h"
#include "lll_scan.h"
#include "lll_conn.h"
#include "lll_filter.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_filter.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_scan_internal.h"

#define ADDR_TYPE_ANON 0xFF

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_filter
#include "common/log.h"
#include "hal/debug.h"

/* Hardware whitelist */
static struct lll_filter wl_filter;
uint8_t wl_anon;

#define IRK_SIZE 16

#if defined(CONFIG_BT_CTLR_PRIVACY)
#include "common/rpa.h"

/* Whitelist peer list */
static struct lll_whitelist wl[WL_SIZE];

static uint8_t rl_enable;

static struct lll_resolvelist rl[CONFIG_BT_CTLR_RL_SIZE];

#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
/* Cache of known unknown peer RPAs */
static uint8_t newest_prpa;
static struct prpa_cache_dev {
	uint8_t      taken:1;
	bt_addr_t rpa;
} prpa_cache[CONFIG_BT_CTLR_RPA_CACHE_SIZE];

struct prpa_resolve_work {
	struct k_work prpa_work;
	bt_addr_t     rpa;
	resolve_callback_t cb;
};

struct target_resolve_work {
	struct k_work target_work;
	bt_addr_t rpa;
	uint8_t      idx;
	resolve_callback_t cb;
};
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */

static uint8_t peer_irks[CONFIG_BT_CTLR_RL_SIZE][IRK_SIZE];
static uint8_t peer_irk_rl_ids[CONFIG_BT_CTLR_RL_SIZE];
static uint8_t peer_irk_count;

static bt_addr_t local_rpas[CONFIG_BT_CTLR_RL_SIZE];

#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
static struct prpa_resolve_work resolve_work;
static struct target_resolve_work t_work;

BUILD_ASSERT(ARRAY_SIZE(prpa_cache) < FILTER_IDX_NONE);
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */
BUILD_ASSERT(ARRAY_SIZE(wl) < FILTER_IDX_NONE);
BUILD_ASSERT(ARRAY_SIZE(rl) < FILTER_IDX_NONE);

/* Hardware filter for the resolving list */
static struct lll_filter rl_filter;

#define DEFAULT_RPA_TIMEOUT_MS (900 * 1000)
static uint32_t rpa_timeout_ms;
static int64_t rpa_last_ms;

static struct k_delayed_work rpa_work;

#define LIST_MATCH(list, i, type, addr) (list[i].taken && \
		    (list[i].id_addr_type == (type & 0x1)) && \
		    !memcmp(list[i].id_addr.val, addr, BDADDR_SIZE))

static void wl_clear(void);
static uint8_t wl_find(uint8_t addr_type, uint8_t *addr, uint8_t *free);
static uint32_t wl_add(bt_addr_le_t *id_addr);
static uint32_t wl_remove(bt_addr_le_t *id_addr);
static void wl_update(void);

static void rl_clear(void);
static void rl_update(void);
static int rl_access_check(bool check_ar);

#if defined(CONFIG_BT_BROADCASTER)
static void rpa_adv_refresh(struct ll_adv_set *adv);
#endif
static void rpa_timeout(struct k_work *work);
static void rpa_refresh_start(void);
static void rpa_refresh_stop(void);
#else /* !CONFIG_BT_CTLR_PRIVACY */
static uint32_t filter_add(struct lll_filter *filter, uint8_t addr_type,
			uint8_t *bdaddr);
static uint32_t filter_remove(struct lll_filter *filter, uint8_t addr_type,
			   uint8_t *bdaddr);
#endif /* !CONFIG_BT_CTLR_PRIVACY */

static void filter_insert(struct lll_filter *filter, int index, uint8_t addr_type,
			   uint8_t *bdaddr);
static void filter_clear(struct lll_filter *filter);

#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
static void prpa_cache_clear(void);
static uint8_t prpa_cache_find(bt_addr_t *prpa_cache_addr);
static void prpa_cache_add(bt_addr_t *prpa_cache_addr);
static uint8_t prpa_cache_try_resolve(bt_addr_t *rpa);
static void prpa_cache_resolve(struct k_work *work);
static void target_resolve(struct k_work *work);
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */

uint8_t ll_wl_size_get(void)
{
	return WL_SIZE;
}

uint8_t ll_wl_clear(void)
{
#if defined(CONFIG_BT_BROADCASTER)
	if (ull_adv_filter_pol_get(0)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	if (ull_scan_filter_pol_get(0) & 0x1) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	wl_clear();
#else
	filter_clear(&wl_filter);
#endif /* CONFIG_BT_CTLR_PRIVACY */

	wl_anon = 0U;

	return 0;
}

uint8_t ll_wl_add(bt_addr_le_t *addr)
{
#if defined(CONFIG_BT_BROADCASTER)
	if (ull_adv_filter_pol_get(0)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	if (ull_scan_filter_pol_get(0) & 0x1) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_OBSERVER */

	if (addr->type == ADDR_TYPE_ANON) {
		wl_anon = 1U;
		return 0;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	return wl_add(addr);
#else
	return filter_add(&wl_filter, addr->type, addr->a.val);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

uint8_t ll_wl_remove(bt_addr_le_t *addr)
{
#if defined(CONFIG_BT_BROADCASTER)
	if (ull_adv_filter_pol_get(0)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	if (ull_scan_filter_pol_get(0) & 0x1) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}
#endif /* CONFIG_BT_OBSERVER */

	if (addr->type == ADDR_TYPE_ANON) {
		wl_anon = 0U;
		return 0;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	return wl_remove(addr);
#else
	return filter_remove(&wl_filter, addr->type, addr->a.val);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

#if defined(CONFIG_BT_CTLR_PRIVACY)
void ll_rl_id_addr_get(uint8_t rl_idx, uint8_t *id_addr_type, uint8_t *id_addr)
{
	LL_ASSERT(rl_idx < CONFIG_BT_CTLR_RL_SIZE);
	LL_ASSERT(rl[rl_idx].taken);

	*id_addr_type = rl[rl_idx].id_addr_type;
	memcpy(id_addr, rl[rl_idx].id_addr.val, BDADDR_SIZE);
}

uint8_t ll_rl_size_get(void)
{
	return CONFIG_BT_CTLR_RL_SIZE;
}

uint8_t ll_rl_clear(void)
{
	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	rl_clear();

	return 0;
}

uint8_t ll_rl_add(bt_addr_le_t *id_addr, const uint8_t pirk[IRK_SIZE],
	       const uint8_t lirk[IRK_SIZE])
{
	uint8_t i, j;

	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	i = ull_filter_rl_find(id_addr->type, id_addr->a.val, &j);

	/* Duplicate check */
	if (i < ARRAY_SIZE(rl)) {
		return BT_HCI_ERR_INVALID_PARAM;
	} else if (j >= ARRAY_SIZE(rl)) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Device not found but empty slot found */
	i = j;

	bt_addr_copy(&rl[i].id_addr, &id_addr->a);
	rl[i].id_addr_type = id_addr->type & 0x1;
	rl[i].pirk = mem_nz((uint8_t *)pirk, IRK_SIZE);
	rl[i].lirk = mem_nz((uint8_t *)lirk, IRK_SIZE);
	if (rl[i].pirk) {
		/* cross-reference */
		rl[i].pirk_idx = peer_irk_count;
		peer_irk_rl_ids[peer_irk_count] = i;
		/* AAR requires big-endian IRKs */
		sys_memcpy_swap(peer_irks[peer_irk_count++], pirk, IRK_SIZE);
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
		/* a new key was added, invalidate the known/unknown list */
		prpa_cache_clear();
#endif
	}
	if (rl[i].lirk) {
		memcpy(rl[i].local_irk, lirk, IRK_SIZE);
		rl[i].local_rpa = NULL;
	}
	memset(rl[i].curr_rpa.val, 0x00, sizeof(rl[i].curr_rpa));
	rl[i].rpas_ready = 0U;
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
	memset(rl[i].target_rpa.val, 0x00, sizeof(rl[i].target_rpa));
#endif
	/* Default to Network Privacy */
	rl[i].dev = 0U;
	/* Add reference to  a whitelist entry */
	j = wl_find(id_addr->type, id_addr->a.val, NULL);
	if (j < ARRAY_SIZE(wl)) {
		wl[j].rl_idx = i;
		rl[i].wl = 1U;
	} else {
		rl[i].wl = 0U;
	}
	rl[i].taken = 1U;

	return 0;
}

uint8_t ll_rl_remove(bt_addr_le_t *id_addr)
{
	uint8_t i;

	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* find the device and mark it as empty */
	i = ull_filter_rl_find(id_addr->type, id_addr->a.val, NULL);
	if (i < ARRAY_SIZE(rl)) {
		uint8_t j, k;

		if (rl[i].pirk) {
			/* Swap with last item */
			uint8_t pi = rl[i].pirk_idx, pj = peer_irk_count - 1;

			if (pj && pi != pj) {
				memcpy(peer_irks[pi], peer_irks[pj], IRK_SIZE);
				for (k = 0U;
				     k < CONFIG_BT_CTLR_RL_SIZE;
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
		j = wl_find(id_addr->type, id_addr->a.val, NULL);
		if (j < ARRAY_SIZE(wl)) {
			wl[j].rl_idx = FILTER_IDX_NONE;
		}
		rl[i].taken = 0U;
		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
}

void ll_rl_crpa_set(uint8_t id_addr_type, uint8_t *id_addr, uint8_t rl_idx, uint8_t *crpa)
{
	if ((crpa[5] & 0xc0) == 0x40) {

		if (id_addr) {
			/* find the device and return its RPA */
			rl_idx = ull_filter_rl_find(id_addr_type, id_addr, NULL);
		}

		if (rl_idx < ARRAY_SIZE(rl) && rl[rl_idx].taken) {
			memcpy(rl[rl_idx].curr_rpa.val, crpa,
			       sizeof(bt_addr_t));
		}
	}
}

uint8_t ll_rl_crpa_get(bt_addr_le_t *id_addr, bt_addr_t *crpa)
{
	uint8_t i;

	/* find the device and return its RPA */
	i = ull_filter_rl_find(id_addr->type, id_addr->a.val, NULL);
	if (i < ARRAY_SIZE(rl) &&
	    mem_nz(rl[i].curr_rpa.val, sizeof(rl[i].curr_rpa.val))) {
		bt_addr_copy(crpa, &rl[i].curr_rpa);
		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
}

uint8_t ll_rl_lrpa_get(bt_addr_le_t *id_addr, bt_addr_t *lrpa)
{
	uint8_t i;

	/* find the device and return the local RPA */
	i = ull_filter_rl_find(id_addr->type, id_addr->a.val, NULL);
	if (i < ARRAY_SIZE(rl)) {
		bt_addr_copy(lrpa, rl[i].local_rpa);
		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
}

uint8_t ll_rl_enable(uint8_t enable)
{
	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	switch (enable) {
	case BT_HCI_ADDR_RES_DISABLE:
		rl_enable = 0U;
		break;
	case BT_HCI_ADDR_RES_ENABLE:
		rl_enable = 1U;
		break;
	default:
		return BT_HCI_ERR_INVALID_PARAM;
	}

	return 0;
}

void ll_rl_timeout_set(uint16_t timeout)
{
	rpa_timeout_ms = timeout * 1000U;
}

uint8_t ll_priv_mode_set(bt_addr_le_t *id_addr, uint8_t mode)
{
	uint8_t i;

	if (!rl_access_check(false)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* find the device and mark it as empty */
	i = ull_filter_rl_find(id_addr->type, id_addr->a.val, NULL);
	if (i < ARRAY_SIZE(rl)) {
		switch (mode) {
		case BT_HCI_LE_PRIVACY_MODE_NETWORK:
			rl[i].dev = 0U;
			break;
		case BT_HCI_LE_PRIVACY_MODE_DEVICE:
			rl[i].dev = 1U;
			break;
		default:
			return BT_HCI_ERR_INVALID_PARAM;
		}
	} else {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	return 0;
}

void ull_filter_adv_scan_state_cb(uint8_t bm)
{
	if (bm) {
		rpa_refresh_start();
	} else {
		rpa_refresh_stop();
	}
}

void ull_filter_adv_update(uint8_t adv_fp)
{
	/* Clear before populating filter */
	filter_clear(&wl_filter);

	/* enabling advertising */
	if (adv_fp &&
	    (!IS_ENABLED(CONFIG_BT_OBSERVER) ||
	     !(ull_scan_filter_pol_get(0) & 0x1))) {
		/* whitelist not in use, update whitelist */
		wl_update();
	}

	/* Clear before populating rl filter */
	filter_clear(&rl_filter);

	if (rl_enable &&
	    (!IS_ENABLED(CONFIG_BT_OBSERVER) || !ull_scan_is_enabled(0))) {
		/* rl not in use, update resolving list LUT */
		rl_update();
	}
}

void ull_filter_scan_update(uint8_t scan_fp)
{
	/* Clear before populating filter */
	filter_clear(&wl_filter);

	/* enabling advertising */
	if ((scan_fp & 0x1) &&
	    (!IS_ENABLED(CONFIG_BT_BROADCASTER) ||
	     !ull_adv_filter_pol_get(0))) {
		/* whitelist not in use, update whitelist */
		wl_update();
	}

	/* Clear before populating rl filter */
	filter_clear(&rl_filter);

	if (rl_enable &&
	    (!IS_ENABLED(CONFIG_BT_BROADCASTER) || !ull_adv_is_enabled(0))) {
		/* rl not in use, update resolving list LUT */
		rl_update();
	}
}

void ull_filter_rpa_update(bool timeout)
{
	uint8_t i;
	int err;
	int64_t now = k_uptime_get();
	bool all = timeout || (rpa_last_ms == -1) ||
		   (now - rpa_last_ms >= rpa_timeout_ms);
	BT_DBG("");

	for (i = 0U; i < CONFIG_BT_CTLR_RL_SIZE; i++) {
		if ((rl[i].taken) && (all || !rl[i].rpas_ready)) {

			if (rl[i].pirk) {
				uint8_t irk[IRK_SIZE];

				/* TODO: move this swap to the driver level */
				sys_memcpy_swap(irk, peer_irks[rl[i].pirk_idx],
						IRK_SIZE);
				err = bt_rpa_create(irk, &rl[i].peer_rpa);
				LL_ASSERT(!err);
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
				/* a new key was added,
				 * invalidate the known/unknown peer RPA cache
				 */
				prpa_cache_clear();
#endif
			}

			if (rl[i].lirk) {
				bt_addr_t rpa;

				err = bt_rpa_create(rl[i].local_irk, &rpa);
				LL_ASSERT(!err);
				/* pointer read/write assumed to be atomic
				 * so that if ISR fires the local_rpa pointer
				 * will always point to a valid full RPA
				 */
				rl[i].local_rpa = &rpa;
				bt_addr_copy(&local_rpas[i], &rpa);
				rl[i].local_rpa = &local_rpas[i];
			}

			rl[i].rpas_ready = 1U;
		}
	}

	if (all) {
		rpa_last_ms = now;
	}

	if (timeout) {
#if defined(CONFIG_BT_BROADCASTER)
		struct ll_adv_set *adv;

		/* TODO: foreach adv set */
		adv = ull_adv_is_enabled_get(0);
		if (adv) {
			rpa_adv_refresh(adv);
		}
#endif
	}
}

#if defined(CONFIG_BT_BROADCASTER)
const uint8_t *ull_filter_adva_get(struct ll_adv_set *adv)
{
	uint8_t idx = adv->lll.rl_idx;

	/* AdvA */
	if (idx < ARRAY_SIZE(rl) && rl[idx].lirk) {
		LL_ASSERT(rl[idx].rpas_ready);
		return rl[idx].local_rpa->val;
	}

	return NULL;
}

const uint8_t *ull_filter_tgta_get(struct ll_adv_set *adv)
{
	uint8_t idx = adv->lll.rl_idx;

	/* TargetA */
	if (idx < ARRAY_SIZE(rl) && rl[idx].pirk) {
		return rl[idx].peer_rpa.val;
	}

	return NULL;
}
#endif /* CONFIG_BT_BROADCASTER */

uint8_t ull_filter_rl_find(uint8_t id_addr_type, uint8_t const *const id_addr,
			uint8_t *const free)
{
	uint8_t i;

	if (free) {
		*free = FILTER_IDX_NONE;
	}

	for (i = 0U; i < CONFIG_BT_CTLR_RL_SIZE; i++) {
		if (LIST_MATCH(rl, i, id_addr_type, id_addr)) {
			return i;
		} else if (free && !rl[i].taken && (*free == FILTER_IDX_NONE)) {
			*free = i;
		}
	}

	return FILTER_IDX_NONE;
}
#endif /* CONFIG_BT_CTLR_PRIVACY */

void ull_filter_reset(bool init)
{
	wl_anon = 0U;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	wl_clear();

	rl_enable = 0U;
	rpa_timeout_ms = DEFAULT_RPA_TIMEOUT_MS;
	rpa_last_ms = -1;
	rl_clear();
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
	prpa_cache_clear();
#endif
	if (init) {
		k_delayed_work_init(&rpa_work, rpa_timeout);
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
		k_work_init(&(resolve_work.prpa_work), prpa_cache_resolve);
		k_work_init(&(t_work.target_work), target_resolve);
#endif
	} else {
		k_delayed_work_cancel(&rpa_work);
	}
#else
	filter_clear(&wl_filter);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

#if defined(CONFIG_BT_CTLR_PRIVACY)
bool ull_filter_lll_lrpa_used(uint8_t rl_idx)
{
	return rl_idx < ARRAY_SIZE(rl) && rl[rl_idx].lirk;
}

bt_addr_t *ull_filter_lll_lrpa_get(uint8_t rl_idx)
{
	if ((rl_idx >= ARRAY_SIZE(rl)) || !rl[rl_idx].lirk ||
	    !rl[rl_idx].rpas_ready) {
		return NULL;
	}

	return rl[rl_idx].local_rpa;
}

uint8_t *ull_filter_lll_irks_get(uint8_t *count)
{
	*count = peer_irk_count;
	return (uint8_t *)peer_irks;
}

uint8_t ull_filter_lll_rl_idx(bool whitelist, uint8_t devmatch_id)
{
	uint8_t i;

	if (whitelist) {
		LL_ASSERT(devmatch_id < ARRAY_SIZE(wl));
		LL_ASSERT(wl[devmatch_id].taken);
		i = wl[devmatch_id].rl_idx;
	} else {
		LL_ASSERT(devmatch_id < ARRAY_SIZE(rl));
		i = devmatch_id;
		LL_ASSERT(rl[i].taken);
	}

	return i;
}

uint8_t ull_filter_lll_rl_irk_idx(uint8_t irkmatch_id)
{
	uint8_t i;

	LL_ASSERT(irkmatch_id < peer_irk_count);
	i = peer_irk_rl_ids[irkmatch_id];
	LL_ASSERT(i < CONFIG_BT_CTLR_RL_SIZE);
	LL_ASSERT(rl[i].taken);

	return i;
}

bool ull_filter_lll_irk_whitelisted(uint8_t rl_idx)
{
	if (rl_idx >= ARRAY_SIZE(rl)) {
		return false;
	}

	LL_ASSERT(rl[rl_idx].taken);

	return rl[rl_idx].wl;
}
#endif /* CONFIG_BT_CTLR_PRIVACY */

struct lll_filter *ull_filter_lll_get(bool whitelist)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (whitelist) {
		return &wl_filter;
	}
	return &rl_filter;
#else
	LL_ASSERT(whitelist);
	return &wl_filter;
#endif
}

#if defined(CONFIG_BT_CTLR_PRIVACY)
struct lll_whitelist *ull_filter_lll_whitelist_get(void)
{
	return wl;
}

struct lll_resolvelist *ull_filter_lll_resolvelist_get(void)
{
	return rl;
}

bool ull_filter_lll_rl_idx_allowed(uint8_t irkmatch_ok, uint8_t rl_idx)
{
	/* If AR is disabled or we don't know the device or we matched an IRK
	 * then we're all set.
	 */
	if (!rl_enable || rl_idx >= ARRAY_SIZE(rl) || irkmatch_ok) {
		return true;
	}

	LL_ASSERT(rl_idx < CONFIG_BT_CTLR_RL_SIZE);
	LL_ASSERT(rl[rl_idx].taken);

	return !rl[rl_idx].pirk || rl[rl_idx].dev;
}

bool ull_filter_lll_rl_addr_allowed(uint8_t id_addr_type, uint8_t *id_addr, uint8_t *rl_idx)
{
	uint8_t i, j;

	/* If AR is disabled or we matched an IRK then we're all set. No hw
	 * filters are used in this case.
	 */
	if (!rl_enable || *rl_idx != FILTER_IDX_NONE) {
		return true;
	}

	for (i = 0U; i < CONFIG_BT_CTLR_RL_SIZE; i++) {
		if (rl[i].taken && (rl[i].id_addr_type == id_addr_type)) {
			uint8_t *addr = rl[i].id_addr.val;

			for (j = 0U; j < BDADDR_SIZE; j++) {
				if (addr[j] != id_addr[j]) {
					break;
				}
			}

			if (j == BDADDR_SIZE) {
				*rl_idx = i;
				return !rl[i].pirk || rl[i].dev;
			}
		}
	}

	return true;
}

bool ull_filter_lll_rl_addr_resolve(uint8_t id_addr_type, uint8_t *id_addr, uint8_t rl_idx)
{
	/* Unable to resolve if AR is disabled, no RL entry or no local IRK */
	if (!rl_enable || rl_idx >= ARRAY_SIZE(rl) || !rl[rl_idx].lirk) {
		return false;
	}

	if ((id_addr_type != 0U) && ((id_addr[5] & 0xc0) == 0x40)) {
		return bt_rpa_irk_matches(rl[rl_idx].local_irk,
					  (bt_addr_t *)id_addr);
	}

	return false;
}

bool ull_filter_lll_rl_enabled(void)
{
	return rl_enable;
}

#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
uint8_t ull_filter_deferred_resolve(bt_addr_t *rpa, resolve_callback_t cb)
{
	if (rl_enable) {
		if (!k_work_pending(&(resolve_work.prpa_work))) {
			/* copy input param to work variable */
			memcpy(resolve_work.rpa.val, rpa->val, sizeof(bt_addr_t));
			resolve_work.cb = cb;

			k_work_submit(&(resolve_work.prpa_work));

			return 1;
		}
	}

	return 0;
}

uint8_t ull_filter_deferred_targeta_resolve(bt_addr_t *rpa, uint8_t rl_idx,
					 resolve_callback_t cb)
{
	if (rl_enable) {
		if (!k_work_pending(&(t_work.target_work))) {
			/* copy input param to work variable */
			memcpy(t_work.rpa.val, rpa->val, sizeof(bt_addr_t));
			t_work.cb = cb;
			t_work.idx = rl_idx;

			k_work_submit(&(t_work.target_work));

			return 1;
		}
	}
	return 0;
}
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */

static void wl_clear(void)
{
	for (int i = 0; i < WL_SIZE; i++) {
		uint8_t j = wl[i].rl_idx;

		if (j < ARRAY_SIZE(rl)) {
			rl[j].wl = 0U;
		}
		wl[i].taken = 0U;
	}
}

static uint8_t wl_find(uint8_t addr_type, uint8_t *addr, uint8_t *free)
{
	int i;

	if (free) {
		*free = FILTER_IDX_NONE;
	}

	for (i = 0; i < WL_SIZE; i++) {
		if (LIST_MATCH(wl, i, addr_type, addr)) {
			return i;
		} else if (free && !wl[i].taken && (*free == FILTER_IDX_NONE)) {
			*free = i;
		}
	}

	return FILTER_IDX_NONE;
}

static uint32_t wl_add(bt_addr_le_t *id_addr)
{
	uint8_t i, j;

	i = wl_find(id_addr->type, id_addr->a.val, &j);

	/* Duplicate  check */
	if (i < ARRAY_SIZE(wl)) {
		return 0;
	} else if (j >= ARRAY_SIZE(wl)) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	i = j;

	wl[i].id_addr_type = id_addr->type & 0x1;
	bt_addr_copy(&wl[i].id_addr, &id_addr->a);
	/* Get index to Resolving List if applicable */
	j = ull_filter_rl_find(id_addr->type, id_addr->a.val, NULL);
	if (j < ARRAY_SIZE(rl)) {
		wl[i].rl_idx = j;
		rl[j].wl = 1U;
	} else {
		wl[i].rl_idx = FILTER_IDX_NONE;
	}
	wl[i].taken = 1U;

	return 0;
}

static uint32_t wl_remove(bt_addr_le_t *id_addr)
{
	/* find the device and mark it as empty */
	uint8_t i = wl_find(id_addr->type, id_addr->a.val, NULL);

	if (i < ARRAY_SIZE(wl)) {
		uint8_t j = wl[i].rl_idx;

		if (j < ARRAY_SIZE(rl)) {
			rl[j].wl = 0U;
		}
		wl[i].taken = 0U;
		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
}

static void wl_update(void)
{
	uint8_t i;

	/* Populate filter from wl peers */
	for (i = 0U; i < WL_SIZE; i++) {
		uint8_t j;

		if (!wl[i].taken) {
			continue;
		}

		j = wl[i].rl_idx;

		if (!rl_enable || j >= ARRAY_SIZE(rl) || !rl[j].pirk ||
		    rl[j].dev) {
			filter_insert(&wl_filter, i, wl[i].id_addr_type,
				      wl[i].id_addr.val);
		}
	}
}

static void rl_update(void)
{
	uint8_t i;

	/* Populate filter from rl peers */
	for (i = 0U; i < CONFIG_BT_CTLR_RL_SIZE; i++) {
		if (rl[i].taken) {
			filter_insert(&rl_filter, i, rl[i].id_addr_type,
				      rl[i].id_addr.val);
		}
	}
}

#if defined(CONFIG_BT_BROADCASTER)
static void rpa_adv_refresh(struct ll_adv_set *adv)
{
	struct pdu_adv *prev;
	struct pdu_adv *pdu;
	uint8_t idx;

	if (adv->own_addr_type != BT_ADDR_LE_PUBLIC_ID &&
	    adv->own_addr_type != BT_ADDR_LE_RANDOM_ID) {
		return;
	}

	if (adv->lll.rl_idx >= ARRAY_SIZE(rl)) {
		return;
	}

	prev = lll_adv_data_peek(&adv->lll);
	pdu = lll_adv_data_alloc(&adv->lll, &idx);

	memcpy(pdu, prev, PDU_AC_LL_HEADER_SIZE + prev->len);
	ull_adv_pdu_update_addrs(adv, pdu);

	lll_adv_data_enqueue(&adv->lll, idx);
}
#endif /* CONFIG_BT_BROADCASTER */

static void rl_clear(void)
{
	for (uint8_t i = 0; i < CONFIG_BT_CTLR_RL_SIZE; i++) {
		rl[i].taken = 0U;
	}

	peer_irk_count = 0U;
}

static int rl_access_check(bool check_ar)
{
	if (check_ar) {
		/* If address resolution is disabled, allow immediately */
		if (!rl_enable) {
			return -1;
		}
	}

	return ((IS_ENABLED(CONFIG_BT_BROADCASTER) && ull_adv_is_enabled(0)) ||
		(IS_ENABLED(CONFIG_BT_OBSERVER) && ull_scan_is_enabled(0)))
		? 0 : 1;
}

static void rpa_timeout(struct k_work *work)
{
	ull_filter_rpa_update(true);
	k_delayed_work_submit(&rpa_work, K_MSEC(rpa_timeout_ms));
}

static void rpa_refresh_start(void)
{
	BT_DBG("");
	k_delayed_work_submit(&rpa_work, K_MSEC(rpa_timeout_ms));
}

static void rpa_refresh_stop(void)
{
	k_delayed_work_cancel(&rpa_work);
}

#else /* !CONFIG_BT_CTLR_PRIVACY */

static uint32_t filter_add(struct lll_filter *filter, uint8_t addr_type,
			uint8_t *bdaddr)
{
	int index;

	if (filter->enable_bitmask == LLL_FILTER_BITMASK_ALL) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	for (index = 0;
	     (filter->enable_bitmask & BIT(index));
	     index++) {
	}

	filter_insert(filter, index, addr_type, bdaddr);
	return 0;
}

static uint32_t filter_remove(struct lll_filter *filter, uint8_t addr_type,
			   uint8_t *bdaddr)
{
	int index;

	if (!filter->enable_bitmask) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	index = WL_SIZE;
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
#endif /* !CONFIG_BT_CTLR_PRIVACY */

static void filter_insert(struct lll_filter *filter, int index, uint8_t addr_type,
			   uint8_t *bdaddr)
{
	filter->enable_bitmask |= BIT(index);
	filter->addr_type_bitmask |= ((addr_type & 0x01) << index);
	memcpy(&filter->bdaddr[index][0], bdaddr, BDADDR_SIZE);
}

static void filter_clear(struct lll_filter *filter)
{
	filter->enable_bitmask = 0;
	filter->addr_type_bitmask = 0;
}

#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
static void target_resolve(struct k_work *work)
{
	uint8_t j, idx;
	bt_addr_t *search_rpa;
	struct target_resolve_work *twork;
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, 0, NULL};

	twork = CONTAINER_OF(work, struct target_resolve_work, target_work);
	idx = twork->idx;
	search_rpa = &(twork->rpa);

	if (rl[idx].taken && !bt_addr_cmp(&(rl[idx].target_rpa), search_rpa)) {
		j = idx;
	} else {
		/* No match - so not in list Need to see if we can resolve */
		if (bt_rpa_irk_matches(rl[idx].local_irk, search_rpa)) {
			/* Could resolve, store RPA */
			memcpy(rl[idx].target_rpa.val, search_rpa->val,
			       sizeof(bt_addr_t));
			j = idx;
		} else {
			/* Could not resolve, and not in table */
			j = FILTER_IDX_NONE;
		}
	}

	/* Kick the callback in LLL (using the mayfly, tailchain it)
	 * Pass param FILTER_IDX_NONE if RPA can not be resolved,
	 * or index in cache if it can be resolved
	 */
	if (twork->cb) {
		mfy.fp = twork->cb;
		mfy.param = (void *) ((unsigned int) j);
		mayfly_enqueue(TICKER_USER_ID_THREAD,
			       TICKER_USER_ID_LLL, 1, &mfy);
	}
}

static uint8_t prpa_cache_try_resolve(bt_addr_t *rpa)
{
	uint8_t pi;
	uint8_t lpirk[IRK_SIZE];

	for (uint8_t i = 0U; i < CONFIG_BT_CTLR_RL_SIZE; i++) {
		if (rl[i].taken && rl[i].pirk) {
			pi = rl[i].pirk_idx;
			sys_memcpy_swap(lpirk, peer_irks[pi], IRK_SIZE);
			if (bt_rpa_irk_matches(lpirk, rpa)) {
				return i;
			}
		}
	}

	return FILTER_IDX_NONE;
}

static void prpa_cache_resolve(struct k_work *work)
{
	uint8_t i, j;
	bt_addr_t *search_rpa;
	struct prpa_resolve_work *rwork;
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, 0, NULL};

	rwork = CONTAINER_OF(work, struct prpa_resolve_work, prpa_work);
	search_rpa = &(rwork->rpa);

	i = prpa_cache_find(search_rpa);

	if (i == FILTER_IDX_NONE) {
		/* No match - so not in known unknown list
		 * Need to see if we can resolve
		 */
		j = prpa_cache_try_resolve(search_rpa);

		if (j == FILTER_IDX_NONE) {
			/* No match - thus cannot resolve, we have an unknown
			 * so insert in known unkonown list
			 */
			prpa_cache_add(search_rpa);
		} else {
			/* Address could be resolved, so update current RPA
			 * in list
			 */
			memcpy(rl[j].curr_rpa.val, search_rpa->val,
			       sizeof(bt_addr_t));
		}

	} else {
		/* Found a known unknown - do nothing */
		j = FILTER_IDX_NONE;
	}

	/* Kick the callback in LLL (using the mayfly, tailchain it)
	 * Pass param FILTER_IDX_NONE if RPA can not be resolved,
	 * or index in cache if it can be resolved
	 */
	if (rwork->cb) {
		mfy.fp = rwork->cb;
		mfy.param = (void *) ((unsigned int) j);
		mayfly_enqueue(TICKER_USER_ID_THREAD,
			       TICKER_USER_ID_LLL, 1, &mfy);
	}
}

static void prpa_cache_clear(void)
{
	/* Note the first element will not be in use before wrap around
	 * is reached.
	 * The first element in actual use will be at index 1.
	 * There is no element waisted with this implementation, as
	 * element 0 will eventually be allocated.
	 */
	newest_prpa = 0U;

	for (uint8_t i = 0; i < CONFIG_BT_CTLR_RPA_CACHE_SIZE; i++) {
		prpa_cache[i].taken = 0U;
	}
}

static void prpa_cache_add(bt_addr_t *rpa)
{
	newest_prpa = (newest_prpa + 1) % CONFIG_BT_CTLR_RPA_CACHE_SIZE;

	memcpy(prpa_cache[newest_prpa].rpa.val, rpa->val, sizeof(bt_addr_t));
	prpa_cache[newest_prpa].taken = 1U;
}

static uint8_t prpa_cache_find(bt_addr_t *rpa)
{
	for (uint8_t i = 0; i < CONFIG_BT_CTLR_RPA_CACHE_SIZE; i++) {
		if (prpa_cache[i].taken &&
		    !bt_addr_cmp(&(prpa_cache[i].rpa), rpa)) {
			return i;
		}
	}
	return FILTER_IDX_NONE;
}
#endif /* !CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */
