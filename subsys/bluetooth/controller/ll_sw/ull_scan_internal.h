/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* NOTE: Definitions used internal to ULL implementations */

int ull_scan_init(void);
int ull_scan_reset(void);

/* Set scan parameters */
void ull_scan_params_set(struct lll_scan *lll, u8_t type, u16_t interval,
			 u16_t window, u8_t filter_policy);

/* Enable and start scanning/initiating role */
u8_t ull_scan_enable(struct ll_scan_set *scan);

/* Disable scanning/initiating role */
u8_t ull_scan_disable(u16_t handle, struct ll_scan_set *scan);

/* Return ll_scan_set context (unconditional) */
struct ll_scan_set *ull_scan_set_get(u16_t handle);

/* Return the scan set handle given the scan set instance */
u16_t ull_scan_handle_get(struct ll_scan_set *scan);

/* Return ll_scan_set context if enabled */
struct ll_scan_set *ull_scan_is_enabled_get(u16_t handle);

/* Return ll_scan_set contesst if disabled */
struct ll_scan_set *ull_scan_is_disabled_get(u16_t handle);

/* Return flags if enabled */
u32_t ull_scan_is_enabled(u16_t handle);

/* Return filter policy used */
u32_t ull_scan_filter_pol_get(u16_t handle);
