/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include "hal/debug.h"
#include "ull_df.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_df
#include "common/log.h"

/* @brief Function sets CTE transmission parameters for a connection.
 *
 * @param[in]handle                     Connection handle.
 * @param[in]cte_types                  Bitfield holding information about
 *                                      allowed CTE types.
 * @param[in]switch_pattern_len         Number of antenna ids in switch pattern.
 * @param[in]ant_id                     Array of antenna identifiers.
 *
 * @return Status of command completion.
 */
uint8_t ll_df_set_conn_cte_tx_params(uint16_t handle, uint8_t cte_types,
				     uint8_t switch_pattern_len,
				     uint8_t *ant_id)
{
	if (cte_types & BT_HCI_LE_AOD_CTE_RSP_1US ||
	    cte_types & BT_HCI_LE_AOD_CTE_RSP_2US) {

		if (!IS_ENABLED(CONFIG_BT_CTLR_DF_ANT_SWITCH_TX)) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}

		if (switch_pattern_len < BT_HCI_LE_SWITCH_PATTERN_LEN_MIN ||
		    switch_pattern_len > BT_HCI_LE_SWITCH_PATTERN_LEN_MAX ||
		    !ant_id) {
			return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
		}
	}

	return BT_HCI_ERR_CMD_DISALLOWED;
}

/* @brief Function provides information about Direction Finding
 * antennae switching and sampling related settings.
 *
 * @param[out]switch_sample_rates       Pointer to store available antennae
 *                                      switch-sampling configurations.
 * @param[out]num_ant                   Pointer to store number of available
 *                                      antennae.
 * @param[out]max_switch_pattern_len    Pointer to store maximum number of
 *                                      antennae switch patterns.
 * @param[out]max_cte_len               Pointer to store maximum length of CTE
 *                                      in [8us] units.
 */
void ll_df_read_ant_inf(uint8_t *switch_sample_rates,
			uint8_t *num_ant,
			uint8_t *max_switch_pattern_len,
			uint8_t *max_cte_len)
{
	/* Currently filled with data that inform about
	 * lack of antenna support for Direction Finding
	 */
	*switch_sample_rates = 0;
	*num_ant = 0;
	*max_switch_pattern_len = 0;
	*max_cte_len = 0;
}
