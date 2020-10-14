/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_ULL_DF_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_ULL_DF_H_

#include <sys/util.h>

/* @brief Direction Finding switchin sampling rates
 *
 * The enum provides information about supported switchin
 * and sampling rates in differen Direction Finding types:
 * - Angle of departure: 1us switching for transmission
 * - Angle of departure 1us sampling for reception
 * - Angle of arrival 1us switchin-sampling for reception.
 *
 * @note Pay attention that both types AoD and AoA
 * support 2US swiching-sampling as mandatory.
 */
enum df_switch_sampl_support {
	DF_AOD_1US_TX	= BIT(0),
	DF_AOD_1US_RX	= BIT(1),
	DF_AOA_1US	= BIT(2)
};

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_ULL_DF_H_ */
