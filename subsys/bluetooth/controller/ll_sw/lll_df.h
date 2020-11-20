/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @brief Configuration of Constant Tone Extension for connectionless
 * transmission.
 */
struct lll_df_adv_cfg {
	uint8_t is_enabled:1;
	uint8_t cte_length:6;   /* Length of CTE in 8us units */
	uint8_t cte_type:2;
	uint8_t cte_count:6;
	uint8_t ant_sw_len:6;
	uint8_t ant_ids[CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN];
};

/* @brief Min supported length of antenna switching pattern */
#define LLL_DF_MIN_ANT_PATTERN_LEN 3
