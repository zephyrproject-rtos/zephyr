/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_df_init(void);
int ull_df_reset(void);

/* Release link to node_rx_iq_report memory. */
void ull_df_iq_report_link_release(memq_link_t *link);
/* Release memory of node_rx_iq_report. */
void ull_df_iq_report_mem_release(struct node_rx_hdr *rx);
/* Change quota of free node_iq_report links. Delta may be negative,
 * then it will decrease number of free link elements.
 */
void ull_iq_report_link_inc_quota(int8_t delta);
/* Allocate node_rx_iq_report in free report PDUs list */
void ull_df_rx_iq_report_alloc(uint8_t max);
/* Initialized DF sync configuration. */
void ull_df_sync_cfg_init(struct lll_df_sync *cfg);
/* Returns information if CTE sampling for periodic sync is requested to disable. */
bool ull_df_sync_cfg_is_not_enabled(struct lll_df_sync *df_cfg);
/* Returns information if CTE sampling for a connection is not enabled. */
bool ull_df_conn_cfg_is_not_enabled(struct lll_df_conn_rx_cfg *rx_cfg);
