/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
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
	DF_AOD_1US_TX = BIT(0),
	DF_AOD_1US_RX = BIT(1),
	DF_AOA_1US = BIT(2)
};

/* TODO Verify required number of IQ reports.
 * At the moment it is set to 2 (if CONFIG_BT_PER_ADV_SYNC_MAX is set the value
 * is multiplied by 2):
 * - for LLL -> ULL
 * - for ULL -> LL(HCI).
 */
#if defined(CONFIG_BT_PER_ADV_SYNC_MAX)
#define IQ_REPORT_CNT (CONFIG_BT_PER_ADV_SYNC_MAX * 2)
#else
#define IQ_REPORT_CNT 0
#endif
