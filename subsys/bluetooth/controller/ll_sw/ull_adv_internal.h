/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

/* helper function to start periodic advertising */
u32_t ull_adv_sync_start(struct ll_adv_sync_set *sync, u32_t ticks_anchor,
			 u32_t volatile *ret_cb);
