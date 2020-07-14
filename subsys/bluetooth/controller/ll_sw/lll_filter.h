/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define WL_SIZE            8
#define FILTER_IDX_NONE    0xFF

struct lll_filter {
	uint8_t  enable_bitmask;
	uint8_t  addr_type_bitmask;
	uint8_t  bdaddr[WL_SIZE][BDADDR_SIZE];
};

/* Whitelist peer list */
struct lll_whitelist {
	uint8_t      taken:1;
	uint8_t      id_addr_type:1;
	uint8_t      rl_idx;
	bt_addr_t id_addr;
};

struct lll_resolvelist {
	uint8_t      taken:1;
	uint8_t      rpas_ready:1;
	uint8_t      pirk:1;
	uint8_t      lirk:1;
	uint8_t      dev:1;
	uint8_t      wl:1;

	uint8_t      id_addr_type:1;
	bt_addr_t id_addr;

	uint8_t      local_irk[16];
	uint8_t      pirk_idx;
	bt_addr_t curr_rpa;
	bt_addr_t peer_rpa;
	bt_addr_t *local_rpa;
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
	bt_addr_t target_rpa;
#endif
};

bool ull_filter_lll_lrpa_used(uint8_t rl_idx);
extern bt_addr_t *ull_filter_lll_lrpa_get(uint8_t rl_idx);
extern uint8_t *ull_filter_lll_irks_get(uint8_t *count);
extern uint8_t ull_filter_lll_rl_idx(bool whitelist, uint8_t devmatch_id);
extern uint8_t ull_filter_lll_rl_irk_idx(uint8_t irkmatch_id);
extern bool ull_filter_lll_irk_whitelisted(uint8_t rl_idx);
extern struct lll_filter *ull_filter_lll_get(bool whitelist);
extern struct lll_whitelist *ull_filter_lll_whitelist_get(void);
extern struct lll_resolvelist *ull_filter_lll_resolvelist_get(void);
extern bool ull_filter_lll_rl_idx_allowed(uint8_t irkmatch_ok, uint8_t rl_idx);
extern bool ull_filter_lll_rl_addr_allowed(uint8_t id_addr_type, uint8_t *id_addr,
					   uint8_t *rl_idx);
extern bool ull_filter_lll_rl_addr_resolve(uint8_t id_addr_type, uint8_t *id_addr,
					   uint8_t rl_idx);
extern bool ull_filter_lll_rl_enabled(void);
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
typedef void (*resolve_callback_t)(void *param);
extern uint8_t ull_filter_deferred_resolve(bt_addr_t *rpa,
					resolve_callback_t cb);
extern uint8_t ull_filter_deferred_targeta_resolve(bt_addr_t *rpa,
						uint8_t rl_idx,
						resolve_callback_t cb);
#endif
