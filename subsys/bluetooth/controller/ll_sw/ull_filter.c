/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>

#include "hal/cpu.h"
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_filter.h"

#include "ll_sw/ull_tx_queue.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_conn_types.h"
#include "ull_filter.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_scan_internal.h"
#include "ull_conn_internal.h"

#include "ll.h"

#include "hal/debug.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_ctlr_ull_filter);

#define ADDR_TYPE_ANON 0xFF

#if defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
/* Hardware Filter Accept List */
static struct lll_filter fal_filter;

#if defined(CONFIG_BT_CTLR_PRIVACY)
#include "common/rpa.h"

/* Filter Accept List peer list */
static struct lll_fal fal[CONFIG_BT_CTLR_FAL_SIZE];

/* Resolving list */
static struct lll_resolve_list rl[CONFIG_BT_CTLR_RL_SIZE];
static uint8_t rl_enable;

#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
/* Cache of known unknown peer RPAs */
static uint8_t newest_prpa;
static struct lll_prpa_cache prpa_cache[CONFIG_BT_CTLR_RPA_CACHE_SIZE];

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
BUILD_ASSERT(ARRAY_SIZE(fal) < FILTER_IDX_NONE);
BUILD_ASSERT(ARRAY_SIZE(rl) < FILTER_IDX_NONE);

/* Hardware filter for the resolving list */
static struct lll_filter rl_filter;

#define DEFAULT_RPA_TIMEOUT_MS (900 * 1000)
static uint32_t rpa_timeout_ms;
static int64_t rpa_last_ms;

static struct k_work_delayable rpa_work;

#define LIST_MATCH(list, i, type, addr) (list[i].taken && \
		    (list[i].id_addr_type == (type & 0x1)) && \
		    !memcmp(list[i].id_addr.val, addr, BDADDR_SIZE))

static void fal_clear(void);
static uint8_t fal_find(uint8_t addr_type, const uint8_t *const addr,
			uint8_t *const free_idx);
static uint32_t fal_add(bt_addr_le_t *id_addr);
static uint32_t fal_remove(bt_addr_le_t *id_addr);
static void fal_update(void);

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

static uint32_t filter_find(const struct lll_filter *const filter,
			    uint8_t addr_type, const uint8_t *const bdaddr);
static void filter_insert(struct lll_filter *const filter, int index,
			  uint8_t addr_type, const uint8_t *const bdaddr);
static void filter_clear(struct lll_filter *filter);

#if defined(CONFIG_BT_CTLR_PRIVACY) && \
	defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
static void conn_rpa_update(uint8_t rl_idx);
#endif /* CONFIG_BT_CTLR_PRIVACY && CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
static void prpa_cache_clear(void);
static uint8_t prpa_cache_find(bt_addr_t *prpa_cache_addr);
static void prpa_cache_add(bt_addr_t *prpa_cache_addr);
static uint8_t prpa_cache_try_resolve(bt_addr_t *rpa);
static void prpa_cache_resolve(struct k_work *work);
static void target_resolve(struct k_work *work);
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */
#endif /* CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
#define PAL_ADDR_MATCH(type, addr) \
		(pal[i].taken && \
		 (pal[i].id_addr_type == (type & 0x1)) && \
		 !memcmp(pal[i].id_addr.val, addr, BDADDR_SIZE))

#define PAL_MATCH(type, addr, sid) \
		(PAL_ADDR_MATCH(type, addr) && \
		 (pal[i].sid == sid))

/* Periodic Advertising Accept List */
#define PAL_SIZE CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST_SIZE
static struct lll_pal pal[PAL_SIZE];

static void pal_clear(void);
#if defined(CONFIG_BT_CTLR_PRIVACY)
static uint8_t pal_addr_find(const uint8_t addr_type,
			     const uint8_t *const addr);
#endif /* CONFIG_BT_CTLR_PRIVACY */
static uint8_t pal_find(const uint8_t addr_type, const uint8_t *const addr,
			const uint8_t sid, uint8_t *const free_idx);
static uint32_t pal_add(const bt_addr_le_t *const id_addr, const uint8_t sid);
static uint32_t pal_remove(const bt_addr_le_t *const id_addr,
			   const uint8_t sid);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST */

#if defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
uint8_t ll_fal_size_get(void)
{
	return CONFIG_BT_CTLR_FAL_SIZE;
}

uint8_t ll_fal_clear(void)
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
	fal_clear();
#else
	filter_clear(&fal_filter);
#endif /* CONFIG_BT_CTLR_PRIVACY */

	return 0;
}

uint8_t ll_fal_add(bt_addr_le_t *addr)
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
		return 0;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	return fal_add(addr);
#else
	return filter_add(&fal_filter, addr->type, addr->a.val);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

uint8_t ll_fal_remove(bt_addr_le_t *addr)
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
		return 0;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	return fal_remove(addr);
#else
	return filter_remove(&fal_filter, addr->type, addr->a.val);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

#if defined(CONFIG_BT_CTLR_PRIVACY)
void ll_rl_id_addr_get(uint8_t rl_idx, uint8_t *id_addr_type, uint8_t *id_addr)
{
	LL_ASSERT(rl_idx < CONFIG_BT_CTLR_RL_SIZE);
	LL_ASSERT(rl[rl_idx].taken);

	*id_addr_type = rl[rl_idx].id_addr_type;
	(void)memcpy(id_addr, rl[rl_idx].id_addr.val, BDADDR_SIZE);
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
		(void)memcpy(rl[i].local_irk, lirk, IRK_SIZE);
		rl[i].local_rpa = NULL;
	}
	memset(rl[i].curr_rpa.val, 0x00, sizeof(rl[i].curr_rpa));
	rl[i].rpas_ready = 0U;
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
	memset(rl[i].target_rpa.val, 0x00, sizeof(rl[i].target_rpa));
#endif
	/* Default to Network Privacy */
	rl[i].dev = 0U;
	/* Add reference to  a Filter Accept List entry */
	j = fal_find(id_addr->type, id_addr->a.val, NULL);
	if (j < ARRAY_SIZE(fal)) {
		fal[j].rl_idx = i;
		rl[i].fal = 1U;
	} else {
		rl[i].fal = 0U;
	}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
	/* Add reference to a periodic list entry */
	j = pal_addr_find(id_addr->type, id_addr->a.val);
	if (j < ARRAY_SIZE(pal)) {
		pal[j].rl_idx = i;
		rl[i].pal = j + 1U;
	} else {
		rl[i].pal = 0U;
	}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST */

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
				(void)memcpy(peer_irks[pi], peer_irks[pj],
					     IRK_SIZE);
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

		/* Check if referenced by a Filter Accept List entry */
		j = fal_find(id_addr->type, id_addr->a.val, NULL);
		if (j < ARRAY_SIZE(fal)) {
			fal[j].rl_idx = FILTER_IDX_NONE;
		}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
		/* Check if referenced by a periodic list entry */
		j = pal_addr_find(id_addr->type, id_addr->a.val);
		if (j < ARRAY_SIZE(pal)) {
			pal[j].rl_idx = FILTER_IDX_NONE;
		}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST */

		rl[i].taken = 0U;

		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
}

void ll_rl_crpa_set(uint8_t id_addr_type, uint8_t *id_addr, uint8_t rl_idx,
		    uint8_t *crpa)
{
	if ((crpa[5] & 0xc0) == 0x40) {

		if (id_addr) {
			/* find the device and return its RPA */
			rl_idx = ull_filter_rl_find(id_addr_type, id_addr,
						    NULL);
		}

		if (rl_idx < ARRAY_SIZE(rl) && rl[rl_idx].taken) {
			(void)memcpy(rl[rl_idx].curr_rpa.val, crpa,
				     sizeof(bt_addr_t));
#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
			conn_rpa_update(rl_idx);
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN) */
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
#endif /* CONFIG_BT_CTLR_PRIVACY */
#endif /* CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
uint8_t ll_pal_size_get(void)
{
	return PAL_SIZE;
}

uint8_t ll_pal_clear(void)
{
	/* FIXME: Check and fail if Periodic Advertising Create Sync is pending.
	 */

	pal_clear();

	return 0;
}

uint8_t ll_pal_add(const bt_addr_le_t *const addr, const uint8_t sid)
{
	/* FIXME: Check and fail if Periodic Advertising Create Sync is pending.
	 */

	if (addr->type == ADDR_TYPE_ANON) {
		return 0;
	}

	return pal_add(addr, sid);
}

uint8_t ll_pal_remove(const bt_addr_le_t *const addr, const uint8_t sid)
{
	/* FIXME: Check and fail if Periodic Advertising Create Sync is pending.
	 */

	if (addr->type == ADDR_TYPE_ANON) {
		return 0;
	}

	return pal_remove(addr, sid);
}

bool ull_filter_ull_pal_addr_match(const uint8_t addr_type,
				   const uint8_t *const addr)
{
	for (int i = 0; i < PAL_SIZE; i++) {
		if (PAL_ADDR_MATCH(addr_type, addr)) {
			return true;
		}
	}

	return false;
}

bool ull_filter_ull_pal_match(const uint8_t addr_type,
			      const uint8_t *const addr, const uint8_t sid)
{
	for (int i = 0; i < PAL_SIZE; i++) {
		if (PAL_MATCH(addr_type, addr, sid)) {
			return true;
		}
	}

	return false;
}

#if defined(CONFIG_BT_CTLR_PRIVACY)
bool ull_filter_ull_pal_listed(const uint8_t rl_idx, uint8_t *const addr_type,
			      uint8_t *const addr)
{
	if (rl_idx >= ARRAY_SIZE(rl)) {
		return false;
	}

	LL_ASSERT(rl[rl_idx].taken);

	if (rl[rl_idx].pal) {
		uint8_t pal_idx = rl[rl_idx].pal - 1;

		*addr_type = pal[pal_idx].id_addr_type;
		(void)memcpy(addr, pal[pal_idx].id_addr.val, BDADDR_SIZE);

		return true;
	}

	return false;
}
#endif /* CONFIG_BT_CTLR_PRIVACY */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST */

void ull_filter_reset(bool init)
{
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
	pal_clear();
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	fal_clear();

	rl_enable = 0U;
	rpa_timeout_ms = DEFAULT_RPA_TIMEOUT_MS;
	rpa_last_ms = -1;
	rl_clear();
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
	prpa_cache_clear();
#endif
	if (init) {
		k_work_init_delayable(&rpa_work, rpa_timeout);
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
		k_work_init(&(resolve_work.prpa_work), prpa_cache_resolve);
		k_work_init(&(t_work.target_work), target_resolve);
#endif
	} else {
		k_work_cancel_delayable(&rpa_work);
	}
#elif defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
	filter_clear(&fal_filter);
#endif /* CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */
}

#if defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
struct lll_filter *ull_filter_lll_get(bool filter)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (filter) {
		return &fal_filter;
	}
	return &rl_filter;
#else
	LL_ASSERT(filter);
	return &fal_filter;
#endif
}

uint8_t ull_filter_lll_fal_match(const struct lll_filter *const filter,
				 uint8_t addr_type, const uint8_t *const addr,
				 uint8_t *devmatch_id)
{
	*devmatch_id = filter_find(filter, addr_type, addr);

	return (*devmatch_id) == FILTER_IDX_NONE ? 0U : 1U;
}

#if defined(CONFIG_BT_CTLR_PRIVACY)
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
	filter_clear(&fal_filter);

	/* enabling advertising */
	if (adv_fp &&
	    (!IS_ENABLED(CONFIG_BT_OBSERVER) ||
	     !(ull_scan_filter_pol_get(0) & 0x1))) {
		/* filter accept list not in use, update FAL */
		fal_update();
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
	filter_clear(&fal_filter);

	/* enabling advertising */
	if ((scan_fp & 0x1) &&
	    (!IS_ENABLED(CONFIG_BT_BROADCASTER) ||
	     !ull_adv_filter_pol_get(0))) {
		/* Filter Accept List not in use, update FAL */
		fal_update();
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
	LOG_DBG("");

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
		uint8_t handle;

		for (handle = 0U; handle < BT_CTLR_ADV_SET; handle++) {
			struct ll_adv_set *adv;

			adv = ull_adv_is_enabled_get(handle);
			if (adv) {
				rpa_adv_refresh(adv);
			}
		}
#endif
	}
}

#if defined(CONFIG_BT_BROADCASTER)
const uint8_t *ull_filter_adva_get(uint8_t rl_idx)
{
	/* AdvA */
	if (rl_idx < ARRAY_SIZE(rl) && rl[rl_idx].lirk) {
		LL_ASSERT(rl[rl_idx].rpas_ready);
		return rl[rl_idx].local_rpa->val;
	}

	return NULL;
}

const uint8_t *ull_filter_tgta_get(uint8_t rl_idx)
{
	/* TargetA */
	if (rl_idx < ARRAY_SIZE(rl) && rl[rl_idx].pirk) {
		return rl[rl_idx].peer_rpa.val;
	}

	return NULL;
}
#endif /* CONFIG_BT_BROADCASTER */

uint8_t ull_filter_rl_find(uint8_t id_addr_type, uint8_t const *const id_addr,
			   uint8_t *const free_idx)
{
	uint8_t i;

	if (free_idx) {
		*free_idx = FILTER_IDX_NONE;
	}

	for (i = 0U; i < CONFIG_BT_CTLR_RL_SIZE; i++) {
		if (LIST_MATCH(rl, i, id_addr_type, id_addr)) {
			return i;
		} else if (free_idx && !rl[i].taken &&
			   (*free_idx == FILTER_IDX_NONE)) {
			*free_idx = i;
		}
	}

	return FILTER_IDX_NONE;
}

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

uint8_t ull_filter_lll_rl_idx(bool filter, uint8_t devmatch_id)
{
	uint8_t i;

	if (filter) {
		LL_ASSERT(devmatch_id < ARRAY_SIZE(fal));
		LL_ASSERT(fal[devmatch_id].taken);
		i = fal[devmatch_id].rl_idx;
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

bool ull_filter_lll_irk_in_fal(uint8_t rl_idx)
{
	if (rl_idx >= ARRAY_SIZE(rl)) {
		return false;
	}

	LL_ASSERT(rl[rl_idx].taken);

	return rl[rl_idx].fal;
}

struct lll_fal *ull_filter_lll_fal_get(void)
{
	return fal;
}

struct lll_resolve_list *ull_filter_lll_resolve_list_get(void)
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

bool ull_filter_lll_rl_addr_allowed(uint8_t id_addr_type,
				    const uint8_t *id_addr,
				    uint8_t *const rl_idx)
{
	uint8_t i, j;

	/* We matched an IRK then we're all set. No hw
	 * filters are used in this case.
	 */
	if (*rl_idx != FILTER_IDX_NONE) {
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

bool ull_filter_lll_rl_addr_resolve(uint8_t id_addr_type,
				    const uint8_t *id_addr, uint8_t rl_idx)
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
		if (!k_work_is_pending(&(resolve_work.prpa_work))) {
			/* copy input param to work variable */
			(void)memcpy(resolve_work.rpa.val, rpa->val,
				     sizeof(bt_addr_t));
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
		if (!k_work_is_pending(&(t_work.target_work))) {
			/* copy input param to work variable */
			(void)memcpy(t_work.rpa.val, rpa->val,
				     sizeof(bt_addr_t));
			t_work.cb = cb;
			t_work.idx = rl_idx;

			k_work_submit(&(t_work.target_work));

			return 1;
		}
	}
	return 0;
}
#endif /* CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */

static void fal_clear(void)
{
	for (int i = 0; i < CONFIG_BT_CTLR_FAL_SIZE; i++) {
		uint8_t j = fal[i].rl_idx;

		if (j < ARRAY_SIZE(rl)) {
			rl[j].fal = 0U;
		}
		fal[i].taken = 0U;
	}
}

static uint8_t fal_find(uint8_t addr_type, const uint8_t *const addr,
			uint8_t *const free_idx)
{
	int i;

	if (free_idx) {
		*free_idx = FILTER_IDX_NONE;
	}

	for (i = 0; i < CONFIG_BT_CTLR_FAL_SIZE; i++) {
		if (LIST_MATCH(fal, i, addr_type, addr)) {
			return i;
		} else if (free_idx && !fal[i].taken &&
			   (*free_idx == FILTER_IDX_NONE)) {
			*free_idx = i;
		}
	}

	return FILTER_IDX_NONE;
}

static uint32_t fal_add(bt_addr_le_t *id_addr)
{
	uint8_t i, j;

	i = fal_find(id_addr->type, id_addr->a.val, &j);

	/* Duplicate  check */
	if (i < ARRAY_SIZE(fal)) {
		return 0;
	} else if (j >= ARRAY_SIZE(fal)) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	i = j;

	fal[i].id_addr_type = id_addr->type & 0x1;
	bt_addr_copy(&fal[i].id_addr, &id_addr->a);
	/* Get index to Resolving List if applicable */
	j = ull_filter_rl_find(id_addr->type, id_addr->a.val, NULL);
	if (j < ARRAY_SIZE(rl)) {
		fal[i].rl_idx = j;
		rl[j].fal = 1U;
	} else {
		fal[i].rl_idx = FILTER_IDX_NONE;
	}
	fal[i].taken = 1U;

	return 0;
}

static uint32_t fal_remove(bt_addr_le_t *id_addr)
{
	/* find the device and mark it as empty */
	uint8_t i = fal_find(id_addr->type, id_addr->a.val, NULL);

	if (i < ARRAY_SIZE(fal)) {
		uint8_t j = fal[i].rl_idx;

		if (j < ARRAY_SIZE(rl)) {
			rl[j].fal = 0U;
		}
		fal[i].taken = 0U;

		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
}

static void fal_update(void)
{
	uint8_t i;

	/* Populate filter from fal peers */
	for (i = 0U; i < CONFIG_BT_CTLR_FAL_SIZE; i++) {
		uint8_t j;

		if (!fal[i].taken) {
			continue;
		}

		j = fal[i].rl_idx;

		if (!rl_enable || j >= ARRAY_SIZE(rl) || !rl[j].pirk ||
		    rl[j].dev) {
			filter_insert(&fal_filter, i, fal[i].id_addr_type,
				      fal[i].id_addr.val);
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
	struct lll_adv_aux *lll_aux;
	struct pdu_adv *prev;
	struct lll_adv *lll;
	struct pdu_adv *pdu;
	uint8_t pri_idx;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	uint8_t sec_idx;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	if (adv->own_addr_type != BT_ADDR_LE_PUBLIC_ID &&
	    adv->own_addr_type != BT_ADDR_LE_RANDOM_ID) {
		return;
	}

	lll = &adv->lll;
	if (lll->rl_idx >= ARRAY_SIZE(rl)) {
		return;
	}


	pri_idx = UINT8_MAX;
	lll_aux = NULL;
	pdu = NULL;
	prev = lll_adv_data_peek(lll);

	if (false) {

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	} else if (prev->type == PDU_ADV_TYPE_EXT_IND) {
		struct pdu_adv_com_ext_adv *pri_com_hdr;
		struct pdu_adv_ext_hdr pri_hdr_flags;
		struct pdu_adv_ext_hdr *pri_hdr;

		/* Pick the primary PDU header flags */
		pri_com_hdr = (void *)&prev->adv_ext_ind;
		pri_hdr = (void *)pri_com_hdr->ext_hdr_adv_data;
		if (pri_com_hdr->ext_hdr_len) {
			pri_hdr_flags = *pri_hdr;
		} else {
			*(uint8_t *)&pri_hdr_flags = 0U;
		}

		/* AdvA, in primary or auxiliary PDU */
		if (pri_hdr_flags.adv_addr) {
			pdu = lll_adv_data_alloc(lll, &pri_idx);
			(void)memcpy(pdu, prev, (PDU_AC_LL_HEADER_SIZE +
						 prev->len));

#if (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
		} else if (pri_hdr_flags.aux_ptr) {
			struct pdu_adv_com_ext_adv *sec_com_hdr;
			struct pdu_adv_ext_hdr sec_hdr_flags;
			struct pdu_adv_ext_hdr *sec_hdr;
			struct pdu_adv *sec_pdu;

			lll_aux = lll->aux;
			sec_pdu = lll_adv_aux_data_peek(lll_aux);

			sec_com_hdr = (void *)&sec_pdu->adv_ext_ind;
			sec_hdr = (void *)sec_com_hdr->ext_hdr_adv_data;
			if (sec_com_hdr->ext_hdr_len) {
				sec_hdr_flags = *sec_hdr;
			} else {
				*(uint8_t *)&sec_hdr_flags = 0U;
			}

			if (sec_hdr_flags.adv_addr) {
				pdu = lll_adv_aux_data_alloc(lll_aux, &sec_idx);
				(void)memcpy(pdu, sec_pdu,
					     (PDU_AC_LL_HEADER_SIZE +
					      sec_pdu->len));
			}
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */
		}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

	} else {
		pdu = lll_adv_data_alloc(lll, &pri_idx);
		(void)memcpy(pdu, prev, (PDU_AC_LL_HEADER_SIZE + prev->len));
	}

	if (pdu) {
		ull_adv_pdu_update_addrs(adv, pdu);

		if (pri_idx != UINT8_MAX) {
			lll_adv_data_enqueue(lll, pri_idx);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		} else {
			lll_adv_aux_data_enqueue(lll_aux, sec_idx);
#endif /* CONFIG_BT_CTLR_ADV_EXT */

		}
	}
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

	/* NOTE: Allowed when passive scanning, otherwise deny if advertising,
	 *       active scanning, initiating or periodic sync create is active.
	 */
	return ((IS_ENABLED(CONFIG_BT_BROADCASTER) && ull_adv_is_enabled(0)) ||
		(IS_ENABLED(CONFIG_BT_OBSERVER) &&
		 (ull_scan_is_enabled(0) & ~ULL_SCAN_IS_PASSIVE)))
		? 0 : 1;
}

static void rpa_timeout(struct k_work *work)
{
	ull_filter_rpa_update(true);
	k_work_schedule(&rpa_work, K_MSEC(rpa_timeout_ms));
}

static void rpa_refresh_start(void)
{
	LOG_DBG("");
	k_work_schedule(&rpa_work, K_MSEC(rpa_timeout_ms));
}

static void rpa_refresh_stop(void)
{
	k_work_cancel_delayable(&rpa_work);
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

	index = filter_find(filter, addr_type, bdaddr);
	if (index == FILTER_IDX_NONE) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	filter->enable_bitmask &= ~BIT(index);
	filter->addr_type_bitmask &= ~BIT(index);

	return 0;
}
#endif /* !CONFIG_BT_CTLR_PRIVACY */

static uint32_t filter_find(const struct lll_filter *const filter,
			    uint8_t addr_type, const uint8_t *const bdaddr)
{
	int index;

	if (!filter->enable_bitmask) {
		return FILTER_IDX_NONE;
	}

	index = LLL_FILTER_SIZE;
	while (index--) {
		if ((filter->enable_bitmask & BIT(index)) &&
		    (((filter->addr_type_bitmask >> index) & 0x01) ==
		     (addr_type & 0x01)) &&
		    !memcmp(filter->bdaddr[index], bdaddr, BDADDR_SIZE)) {
			return index;
		}
	}

	return FILTER_IDX_NONE;
}

static void filter_insert(struct lll_filter *const filter, int index,
			  uint8_t addr_type, const uint8_t *const bdaddr)
{
	filter->enable_bitmask |= BIT(index);
	filter->addr_type_bitmask |= ((addr_type & 0x01) << index);
	(void)memcpy(&filter->bdaddr[index][0], bdaddr, BDADDR_SIZE);
}

static void filter_clear(struct lll_filter *filter)
{
	filter->enable_bitmask = 0;
	filter->addr_type_bitmask = 0;
}
#endif /* CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
static void pal_clear(void)
{
	for (int i = 0; i < PAL_SIZE; i++) {

#if defined(CONFIG_BT_CTLR_PRIVACY)
		uint8_t j = pal[i].rl_idx;

		if (j < ARRAY_SIZE(pal)) {
			rl[j].pal = 0U;
		}
#endif /* CONFIG_BT_CTLR_PRIVACY */

		pal[i].taken = 0U;
	}
}

#if defined(CONFIG_BT_CTLR_PRIVACY)
static uint8_t pal_addr_find(const uint8_t addr_type, const uint8_t *const addr)
{
	for (int i = 0; i < PAL_SIZE; i++) {
		if (PAL_ADDR_MATCH(addr_type, addr)) {
			return i;
		}
	}

	return FILTER_IDX_NONE;
}
#endif /* CONFIG_BT_CTLR_PRIVACY */

static uint8_t pal_find(const uint8_t addr_type, const uint8_t *const addr,
			const uint8_t sid, uint8_t *const free_idx)
{
	int i;

	if (free_idx) {
		*free_idx = FILTER_IDX_NONE;
	}

	for (i = 0; i < PAL_SIZE; i++) {
		if (PAL_MATCH(addr_type, addr, sid)) {
			return i;
		} else if (free_idx && !pal[i].taken &&
			   (*free_idx == FILTER_IDX_NONE)) {
			*free_idx = i;
		}
	}

	return FILTER_IDX_NONE;
}

static uint32_t pal_add(const bt_addr_le_t *const id_addr, const uint8_t sid)
{
	uint8_t i, j;

	i = pal_find(id_addr->type, id_addr->a.val, sid, &j);

	/* Duplicate  check */
	if (i < PAL_SIZE) {
		return BT_HCI_ERR_INVALID_PARAM;
	} else if (j >= PAL_SIZE) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	i = j;

	pal[i].id_addr_type = id_addr->type & 0x1;
	bt_addr_copy(&pal[i].id_addr, &id_addr->a);
	pal[i].sid = sid;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* Get index to Resolving List if applicable */
	j = ull_filter_rl_find(id_addr->type, id_addr->a.val, NULL);
	if (j < ARRAY_SIZE(rl)) {
		pal[i].rl_idx = j;
		rl[j].pal = i + 1U;
	} else {
		pal[i].rl_idx = FILTER_IDX_NONE;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

	pal[i].taken = 1U;

	return 0;
}

static uint32_t pal_remove(const bt_addr_le_t *const id_addr, const uint8_t sid)
{
	/* find the device and mark it as empty */
	uint8_t i = pal_find(id_addr->type, id_addr->a.val, sid, NULL);

	if (i < PAL_SIZE) {

#if defined(CONFIG_BT_CTLR_PRIVACY)
		uint8_t j = pal[i].rl_idx;

		if (j < ARRAY_SIZE(rl)) {
			rl[j].pal = 0U;
		}
#endif /* CONFIG_BT_CTLR_PRIVACY */

		pal[i].taken = 0U;

		return 0;
	}

	return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST */

#if defined(CONFIG_BT_CTLR_PRIVACY) && \
	defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
static void conn_rpa_update(uint8_t rl_idx)
{
	uint16_t handle;

	for (handle = 0U; handle < CONFIG_BT_MAX_CONN; handle++) {
		struct ll_conn *conn = ll_connected_get(handle);

		/* The RPA of the connection matches the RPA that was just
		 * resolved
		 */
		if (conn && !memcmp(conn->peer_id_addr, rl[rl_idx].curr_rpa.val,
				    BDADDR_SIZE)) {
			(void)memcpy(conn->peer_id_addr, rl[rl_idx].id_addr.val,
				     BDADDR_SIZE);
			break;
		}
	}
}
#endif /* CONFIG_BT_CTLR_PRIVACY && CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

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

	if (rl[idx].taken && bt_addr_eq(&(rl[idx].target_rpa), search_rpa)) {
		j = idx;
	} else {
		/* No match - so not in list Need to see if we can resolve */
		if (bt_rpa_irk_matches(rl[idx].local_irk, search_rpa)) {
			/* Could resolve, store RPA */
			(void)memcpy(rl[idx].target_rpa.val, search_rpa->val,
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
		(void)mayfly_enqueue(TICKER_USER_ID_THREAD,
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
			(void)memcpy(rl[j].curr_rpa.val, search_rpa->val,
				     sizeof(bt_addr_t));
#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
			conn_rpa_update(j);
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */
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
		(void)mayfly_enqueue(TICKER_USER_ID_THREAD,
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

	(void)memcpy(prpa_cache[newest_prpa].rpa.val, rpa->val,
		     sizeof(bt_addr_t));
	prpa_cache[newest_prpa].taken = 1U;
}

static uint8_t prpa_cache_find(bt_addr_t *rpa)
{
	for (uint8_t i = 0; i < CONFIG_BT_CTLR_RPA_CACHE_SIZE; i++) {
		if (prpa_cache[i].taken &&
		    bt_addr_eq(&(prpa_cache[i].rpa), rpa)) {
			return i;
		}
	}
	return FILTER_IDX_NONE;
}

const struct lll_prpa_cache *ull_filter_lll_prpa_cache_get(void)
{
	return prpa_cache;
}
#endif /* !CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY */
