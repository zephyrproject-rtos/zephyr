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
#if defined(CONFIG_BT_PER_ADV_SYNC_MAX) && defined(CONFIG_BT_CTLR_DF_PER_SCAN_CTE_NUM_MAX)
#define SYNC_IQ_REPORT_CNT (CONFIG_BT_PER_ADV_SYNC_MAX * CONFIG_BT_CTLR_DF_PER_SCAN_CTE_NUM_MAX * 2)
#else
#define SYNC_IQ_REPORT_CNT 0U
#endif

#if defined(CONFIG_BT_MAX_CONN) && defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
#define CONN_IQ_REPORT_CNT (CONFIG_BT_MAX_CONN * 2)
#else
#define CONN_IQ_REPORT_CNT 0U
#endif

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
#define DTM_IQ_REPORT_CNT CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT_NUM_MAX
#else
#define DTM_IQ_REPORT_CNT 0U
#endif

#define IQ_REPORT_CNT (SYNC_IQ_REPORT_CNT + CONN_IQ_REPORT_CNT + DTM_IQ_REPORT_CNT)
