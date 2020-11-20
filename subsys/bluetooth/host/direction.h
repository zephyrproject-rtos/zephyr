/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_DF_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_DF_H_

/* @brief Optional switching and sampling rates for Angle of Departure
 * and Angle or Arrival modes.
 */
enum bt_df_opt_swich_sample_rates {
	/* Support Angle of Departure 1us switching TX */
	DF_AOD_1US_SWITCH_TX_SUPP = BIT(0),
	/* Support Angle of departure 1us sampling RX */
	DF_AOD_1US_SAMPL_RX_SUPP = BIT(1),
	/* Support Angle of Arrival 1us switching and sampling RX */
	DF_AOA_1US_SWITCH_SAMPL_RX_SUPP  = BIT(2),
};

/* @brief Constant Tone Extension parameters for connectionless
 * transmission.
 *
 * The structure holds information required to setup CTE transmission
 * in periodic advertising.
 */
struct bt_le_df_adv_cte_tx_params {
	/* Length of CTE in 8us units */
	uint8_t  cte_len;
	/* CTE Type: AoA, AoD 1us slots, AoD 2us slots */
	uint8_t  cte_type;
	/* Number of CTE to transmit in each periodic adv interval */
	uint8_t  cte_count;
	/* Number of Antenna IDs in the switch pattern */
	uint8_t  num_ant_ids;
	/* List of antenna IDs in the pattern */
	uint8_t  *ant_ids;
};
#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_DF_H_ */
