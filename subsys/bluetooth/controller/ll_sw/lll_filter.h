/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_FAL_SIZE) && defined(CONFIG_BT_CTLR_RL_SIZE)
#define LLL_FILTER_SIZE MAX(CONFIG_BT_CTLR_FAL_SIZE, CONFIG_BT_CTLR_RL_SIZE)
#elif defined(CONFIG_BT_CTLR_FAL_SIZE)
#define LLL_FILTER_SIZE CONFIG_BT_CTLR_FAL_SIZE
#elif defined(CONFIG_BT_CTLR_RL_SIZE)
#define LLL_FILTER_SIZE CONFIG_BT_CTLR_RL_SIZE
#else
#define LLL_FILTER_SIZE 8
#endif /* CONFIG_BT_CTLR_FAL_SIZE && CONFIG_BT_CTLR_RL_SIZE */

#define IRK_SIZE               16
#define FILTER_IDX_NONE        0xFF
#define LLL_FILTER_BITMASK_ALL (BIT_MASK(LLL_FILTER_SIZE))

#define LLL_FILTER_SIZE_MAX_BITMASK8	(1 * CHAR_BIT)
#define LLL_FILTER_SIZE_MAX_BITMASK16	(2 * CHAR_BIT)

struct lll_filter {
#if (LLL_FILTER_SIZE <= LLL_FILTER_SIZE_MAX_BITMASK8)
	uint8_t  enable_bitmask;
	uint8_t  addr_type_bitmask;
#elif (LLL_FILTER_SIZE <= LLL_FILTER_SIZE_MAX_BITMASK16)
	uint16_t enable_bitmask;
	uint16_t addr_type_bitmask;
#else
#error LLL_FILTER_SIZE must be <= LLL_FILTER_SIZE_MAX_BITMASK16
#endif
	uint8_t  bdaddr[LLL_FILTER_SIZE][BDADDR_SIZE];
};

/* Filter Accept List peer list */
struct lll_fal {
	uint8_t   taken:1;
	uint8_t   id_addr_type:1;
	uint8_t   rl_idx;
	bt_addr_t id_addr;
};

/* Periodic Advertising List */
struct lll_pal {
	bt_addr_t id_addr;
	uint8_t   taken:1;
	uint8_t   id_addr_type:1;
	uint8_t   sid;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t   rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */
};

/* Resolve list */
struct lll_resolve_list {
	uint8_t   taken:1;
	uint8_t   rpas_ready:1;
	uint8_t   pirk:1;
	uint8_t   lirk:1;
	uint8_t   dev:1;
	uint8_t   fal:1;

	uint8_t   id_addr_type:1;
	bt_addr_t id_addr;

	uint8_t   local_irk[IRK_SIZE];
	uint8_t   pirk_idx;
	bt_addr_t curr_rpa;
	bt_addr_t peer_rpa;
	bt_addr_t *local_rpa;
#if defined(CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY)
	bt_addr_t target_rpa;
#endif

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
	uint16_t   pal:9; /* 0 - not present, 1 to 256 - lll_pal entry index */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST */
};

extern uint8_t ull_filter_lll_fal_match(struct lll_filter const *const filter,
					uint8_t addr_type,
					uint8_t const *const addr,
					uint8_t *devmatch_id);
extern bool ull_filter_lll_lrpa_used(uint8_t rl_idx);
extern bt_addr_t *ull_filter_lll_lrpa_get(uint8_t rl_idx);
extern uint8_t *ull_filter_lll_irks_get(uint8_t *count);
extern uint8_t ull_filter_lll_rl_idx(bool fal, uint8_t devmatch_id);
extern uint8_t ull_filter_lll_rl_irk_idx(uint8_t irkmatch_id);
extern bool ull_filter_lll_irk_in_fal(uint8_t rl_idx);
extern struct lll_filter *ull_filter_lll_get(bool fal);
extern struct lll_fal *ull_filter_lll_fal_get(void);
extern struct lll_resolve_list *ull_filter_lll_resolve_list_get(void);
extern bool ull_filter_lll_rl_idx_allowed(uint8_t irkmatch_ok, uint8_t rl_idx);
extern bool ull_filter_lll_rl_addr_allowed(uint8_t id_addr_type,
					   const uint8_t *id_addr,
					   uint8_t *const rl_idx);
extern bool ull_filter_lll_rl_addr_resolve(uint8_t id_addr_type,
					   const uint8_t *id_addr,
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
