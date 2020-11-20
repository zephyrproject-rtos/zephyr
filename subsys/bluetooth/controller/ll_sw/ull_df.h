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
