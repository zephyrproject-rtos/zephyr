/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* NOTE: Definitions used internal to ULL implementations */

/* Return flags, for now just: enabled */
u32_t ull_adv_is_enabled(u16_t handle);

/* Return ll_adv_set context (unconditional) */
struct ll_adv_set *ull_adv_set_get(u16_t handle);

/* Return ll_adv_set context if enabled */
struct ll_adv_set *ull_adv_is_enabled_get(u16_t handle);

/* Return filter policy used */
u32_t ull_adv_filter_pol_get(u16_t handle);
