/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define WL_SIZE            8
#define FILTER_IDX_NONE    0xFF

struct lll_filter {
	u8_t  enable_bitmask;
	u8_t  addr_type_bitmask;
	u8_t  bdaddr[WL_SIZE][BDADDR_SIZE];
};

/* Whitelist peer list */
struct lll_whitelist {
	u8_t      taken:1;
	u8_t      id_addr_type:1;
	u8_t      rl_idx;
	bt_addr_t id_addr;
};

struct lll_resolvelist {
	u8_t      taken:1;
	u8_t      rpas_ready:1;
	u8_t      pirk:1;
	u8_t      lirk:1;
	u8_t      dev:1;
	u8_t      wl:1;

	u8_t      id_addr_type:1;
	bt_addr_t id_addr;

	u8_t      local_irk[16];
	u8_t      pirk_idx;
	bt_addr_t curr_rpa;
	bt_addr_t peer_rpa;
	bt_addr_t *local_rpa;
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
	bt_addr_t target_rpa;
#endif
};

bool ull_filter_lll_lrpa_used(u8_t rl_idx);
extern bt_addr_t *ull_filter_lll_lrpa_get(u8_t rl_idx);
extern u8_t *ull_filter_lll_irks_get(u8_t *count);
extern u8_t ull_filter_lll_rl_idx(bool whitelist, u8_t devmatch_id);
extern u8_t ull_filter_lll_rl_irk_idx(u8_t irkmatch_id);
extern bool ull_filter_lll_irk_whitelisted(u8_t rl_idx);
extern struct lll_filter *ull_filter_lll_get(bool whitelist);
extern struct lll_whitelist *ull_filter_lll_whitelist_get(void);
extern struct lll_resolvelist *ull_filter_lll_resolvelist_get(void);
extern bool ull_filter_lll_rl_idx_allowed(u8_t irkmatch_ok, u8_t rl_idx);
extern bool ull_filter_lll_rl_addr_allowed(u8_t id_addr_type, u8_t *id_addr,
					   u8_t *rl_idx);
extern bool ull_filter_lll_rl_addr_resolve(u8_t id_addr_type, u8_t *id_addr,
					   u8_t rl_idx);
extern bool ull_filter_lll_rl_enabled(void);
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
typedef void (*resolve_callback_t)(void *param);
extern u8_t ull_filter_deferred_resolve(bt_addr_t *rpa,
					resolve_callback_t cb);
extern u8_t ull_filter_deferred_targeta_resolve(bt_addr_t *rpa,
						u8_t rl_idx,
						resolve_callback_t cb);
#endif
