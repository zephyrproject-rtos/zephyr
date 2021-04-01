/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @brief Direction Finding switching sampling rates
 *
 * The enum provides information about supported switching
 * and sampling rates in different Direction Finding types:
 * - Angle of departure: 1us switching for transmission
 * - Angle of departure 1us sampling for reception
 * - Angle of arrival 1us switching-sampling for reception.
 *
 * @note Pay attention that both types AoD and AoA
 * support 2US switching-sampling as mandatory.
 */
enum df_switch_sample_support {
	DF_AOD_1US_TX   = BIT(0),
	DF_AOD_1US_RX   = BIT(1),
	DF_AOA_1US      = BIT(2)
};

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
