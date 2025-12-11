/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Structure to store CTE receive parameters for unit tests setup.
 */
struct ut_bt_df_conn_cte_rx_params {
	uint8_t slot_durations;
	uint8_t switch_pattern_len;
	uint8_t *ant_ids;
};

/**
 * @brief Function sends HCI_LE_Set_Connectionless_CTE_Sampling_Enable
 *        to controller.
 *
 * @param conn_handle                 Connection instance handle.
 * @param params                      CTE sampling parameters.
 * @param enable                      Enable or disable CTE sampling.
 *
 * @return Zero if success, non-zero value in case of failure.
 */
int send_set_conn_cte_rx_params(uint16_t conn_handle,
				const struct ut_bt_df_conn_cte_rx_params *params, bool enable);
