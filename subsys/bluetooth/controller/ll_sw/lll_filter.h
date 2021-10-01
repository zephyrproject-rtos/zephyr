/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_FAL_SIZE)
#define FAL_SIZE CONFIG_BT_CTLR_FAL_SIZE
#else
#define FAL_SIZE 8
#endif /* CONFIG_BT_CTLR_FAL_SIZE */

#define FILTER_IDX_NONE        0xFF
#define LLL_FILTER_BITMASK_ALL (BIT(FAL_SIZE) - 1)

struct lll_filter {
#if (FAL_SIZE <= 8)
	uint8_t  enable_bitmask;
	uint8_t  addr_type_bitmask;
#elif (FAL_SIZE <= 16)
	uint16_t enable_bitmask;
	uint16_t addr_type_bitmask;
#else
#error FAL_SIZE must be <= 16
#endif
	uint8_t  bdaddr[FAL_SIZE][BDADDR_SIZE];
};

/* Filter Accept List peer list */
struct lll_fal {
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
	uint8_t      fal:1;

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
extern uint8_t ull_filter_lll_rl_idx(bool fal, uint8_t devmatch_id);
extern uint8_t ull_filter_lll_rl_irk_idx(uint8_t irkmatch_id);
extern bool ull_filter_lll_irk_in_fal(uint8_t rl_idx);
extern struct lll_filter *ull_filter_lll_get(bool fal);
extern struct lll_fal *ull_filter_lll_fal_get(void);
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
