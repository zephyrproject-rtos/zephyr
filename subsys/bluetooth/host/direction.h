/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_DF_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_DF_H_

/* @brief Antenna information for LE Direction Finding */
struct bt_le_df_ant_info {
	uint8_t switch_sample_rates;
	uint8_t num_ant;
	uint8_t max_switch_pattern_len;
	uint8_t max_CTE_len;
};

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_DF_H_ */
