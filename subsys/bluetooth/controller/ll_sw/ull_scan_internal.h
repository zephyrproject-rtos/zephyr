/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* NOTE: Definitions used internal to ULL implementations */

/* Return flags if enabled */
u32_t ll_scan_is_enabled(void);

/* Return ll_scan_set context (unconditional) */
struct ll_scan_set *ull_scan_set_get(u16_t handle);

/* Return ll_scan_set context if enabled */
struct ll_scan_set *ull_scan_is_enabled_get(u16_t handle);

/* Return filter policy used */
u32_t ull_scan_filter_pol_get(u16_t handle);
