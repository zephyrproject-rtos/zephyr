/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* NOTE: Definitions used internal to ULL implementations */

#define SCAN_HANDLE_1M        0
#define SCAN_HANDLE_PHY_CODED 1

int ull_scan_init(void);
int ull_scan_reset(void);

/* Set scan parameters */
void ull_scan_params_set(struct lll_scan *lll, uint8_t type, uint16_t interval,
			 uint16_t window, uint8_t filter_policy);

/* Enable and start scanning/initiating role */
uint8_t ull_scan_enable(struct ll_scan_set *scan);

/* Disable scanning/initiating role */
uint8_t ull_scan_disable(uint8_t handle, struct ll_scan_set *scan);

/* Return ll_scan_set context (unconditional) */
struct ll_scan_set *ull_scan_set_get(uint8_t handle);

/* Return the scan set handle given the scan set instance */
uint8_t ull_scan_handle_get(struct ll_scan_set *scan);

/* Return ll_scan_set context if enabled */
struct ll_scan_set *ull_scan_is_enabled_get(uint8_t handle);

/* Return ll_scan_set contesst if disabled */
struct ll_scan_set *ull_scan_is_disabled_get(uint8_t handle);

/* Return flags if enabled */
uint32_t ull_scan_is_enabled(uint8_t handle);

/* Return filter policy used */
uint32_t ull_scan_filter_pol_get(uint8_t handle);

int ull_scan_aux_init(void);
int ull_scan_aux_reset(void);

/* Helper to setup scanning on auxiliary channel */
void ull_scan_aux_setup(memq_link_t *link, struct node_rx_hdr *rx, uint8_t phy);

/* Helper to clean up auxiliary channel scanning */
void ull_scan_aux_done(struct node_rx_event_done *done);
