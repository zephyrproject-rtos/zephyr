/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* NOTE: Definitions used internal to ULL implementations */

#define SCAN_HANDLE_1M        0
#define SCAN_HANDLE_PHY_CODED 1

#define EXT_SCAN_DURATION_UNIT_US 10000U
#define EXT_SCAN_PERIOD_UNIT_US   1280000U

/* Convert period in 1.28 s units to duration of 10 ms units*/
#define ULL_SCAN_PERIOD_TO_DURATION(period) \
	((uint32_t)(period) * EXT_SCAN_PERIOD_UNIT_US / \
	 EXT_SCAN_DURATION_UNIT_US)

/* Convert duration in 10 ms unit to radio events count */
#define ULL_SCAN_DURATION_TO_EVENTS(duration, interval) \
	(((uint32_t)(duration) * EXT_SCAN_DURATION_UNIT_US / \
	  SCAN_INT_UNIT_US) / (interval))

/* Convert period in 1.28 s unit to radio events count */
#define ULL_SCAN_PERIOD_TO_EVENTS(period, interval) \
	(((uint32_t)(period) * EXT_SCAN_PERIOD_UNIT_US / \
	  SCAN_INT_UNIT_US) / (interval))

int ull_scan_init(void);
int ull_scan_reset(void);

/* Set scan parameters */
uint32_t ull_scan_params_set(struct lll_scan *lll, uint8_t type,
			     uint16_t interval, uint16_t window,
			     uint8_t filter_policy);

/* Enable and start scanning/initiating role */
uint8_t ull_scan_enable(struct ll_scan_set *scan);

/* Disable scanning/initiating role */
uint8_t ull_scan_disable(uint8_t handle, struct ll_scan_set *scan);

/* Helper function to handle scan done events */
void ull_scan_done(struct node_rx_event_done *done);

/* Helper function to dequeue scan timeout event */
void ull_scan_term_dequeue(uint8_t handle);

/* Return ll_scan_set context (unconditional) */
struct ll_scan_set *ull_scan_set_get(uint8_t handle);

/* Return the scan set handle given the scan set instance */
uint8_t ull_scan_handle_get(struct ll_scan_set *scan);

/* Helper function to check and return if a valid scan context */
struct ll_scan_set *ull_scan_is_valid_get(struct ll_scan_set *scan);

/* Return ll_scan_set context if enabled */
struct ll_scan_set *ull_scan_is_enabled_get(uint8_t handle);

/* Return ll_scan_set context if disabled */
struct ll_scan_set *ull_scan_is_disabled_get(uint8_t handle);

/* Return flags if enabled */
#define ULL_SCAN_IS_PASSIVE   BIT(0)
#define ULL_SCAN_IS_ACTIVE    BIT(1)
#define ULL_SCAN_IS_INITIATOR BIT(2)
#define ULL_SCAN_IS_SYNC      BIT(3)
uint32_t ull_scan_is_enabled(uint8_t handle);

/* Return filter policy used */
uint32_t ull_scan_filter_pol_get(uint8_t handle);

int ull_scan_aux_init(void);
int ull_scan_aux_reset(void);

/* Helper to setup scanning on auxiliary channel */
void ull_scan_aux_setup(memq_link_t *link, struct node_rx_pdu *rx);

/* Helper to clean up auxiliary channel scanning */
void ull_scan_aux_done(struct node_rx_event_done *done);

/* Return the scan aux set instance given the handle */
struct ll_scan_aux_set *ull_scan_aux_set_get(uint8_t handle);

/* Helper function to check and return if a valid aux scan context */
struct ll_scan_aux_set *ull_scan_aux_is_valid_get(struct ll_scan_aux_set *aux);

/* Helper function to flush and release incomplete auxiliary PDU chaining */
void ull_scan_aux_release(memq_link_t *link, struct node_rx_pdu *rx);

/* Helper function to stop auxiliary scan context */
#if defined(CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS)
int ull_scan_aux_stop(void *parent);
#else /* !CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
int ull_scan_aux_stop(struct ll_scan_aux_set *aux);
#endif /* !CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
