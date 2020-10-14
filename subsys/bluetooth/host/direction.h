/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_DF_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_DF_H_

#include <sys/util.h>

enum df_opt_swich_sample_rates {
	/* Support Angle of Departure 1us switching TX */
	DF_AOD_1US_SWITCH_TX_SUPP = BIT(0),
	/* Support Angle of departure 1us sampling RX */
	DF_AOD_1US_SAMPL_RX_SUPP = BIT(1),
	/* Support Angle of Arrival 1us switching and sampling RX */
	DF_AOA_1US_SWITCH_SAMPL_RX_SUPP  = BIT(2),
};

/* @brief Antenna information for LE Direction Finding */
struct bt_le_df_ant_info {
	uint8_t switch_sample_rates;
	uint8_t num_ant;
	uint8_t max_switch_pattern_len;
	uint8_t max_CTE_len;
};

#if IS_ENABLED(CONFIG_TEST)
int hci_df_read_ant_info(struct bt_le_df_ant_info *ant_info);
#endif /* CONFIG_TEST */

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_DF_H_ */
