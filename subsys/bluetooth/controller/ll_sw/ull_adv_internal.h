/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define ULL_ADV_RANDOM_DELAY HAL_TICKER_US_TO_TICKS(10000)

int ull_adv_init(void);
int ull_adv_reset(void);

/* Return ll_adv_set context (unconditional) */
struct ll_adv_set *ull_adv_set_get(u8_t handle);

/* Return the adv set handle given the adv set instance */
u16_t ull_adv_handle_get(struct ll_adv_set *adv);

/* Return ll_adv_set context if enabled */
struct ll_adv_set *ull_adv_is_enabled_get(u8_t handle);

/* Return flags, for now just: enabled */
u32_t ull_adv_is_enabled(u8_t handle);

/* Return filter policy used */
u32_t ull_adv_filter_pol_get(u8_t handle);

/* Return ll_adv_set context if created */
struct ll_adv_set *ull_adv_is_created_get(u8_t handle);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
int ull_adv_aux_init(void);
int ull_adv_aux_reset(void);

/* helper function to start auxiliary advertising */
u32_t ull_adv_aux_start(struct ll_adv_aux_set *aux, u32_t ticks_anchor,
			u32_t volatile *ret_cb);

/* helper function to stop auxiliary advertising */
u8_t ull_adv_aux_stop(struct ll_adv_aux_set *aux);

/* helper function to schedule a mayfly to get aux offset */
void ull_adv_aux_offset_get(struct ll_adv_set *adv);

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
int ull_adv_sync_init(void);
int ull_adv_sync_reset(void);

/* helper function to start periodic advertising */
u32_t ull_adv_sync_start(struct ll_adv_sync_set *sync, u32_t ticks_anchor,
			 u32_t volatile *ret_cb);

/* helper function to schedule a mayfly to get sync offset */
void ull_adv_sync_offset_get(struct ll_adv_set *adv);
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
