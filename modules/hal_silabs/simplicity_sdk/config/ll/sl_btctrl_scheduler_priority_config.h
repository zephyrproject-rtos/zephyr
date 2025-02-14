/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This configuration header is used by the Silicon Labs Bluetooth Controller init functions.
 * It is used to configure scheduler priority options.
 */

#ifndef SL_BTCTRL_SCHEDULER_PRIORITY_CONFIG_H
#define SL_BTCTRL_SCHEDULER_PRIORITY_CONFIG_H
#include "sl_btctrl_linklayer_defs.h"

#define SL_BT_CONTROLLER_SCHEDULER_PRI_SCAN_MIN  191
#define SL_BT_CONTROLLER_SCHEDULER_PRI_SCAN_MAX  143
#define SL_BT_CONTROLLER_SCHEDULER_PRI_SCAN_STEP 4

#define SL_BT_CONTROLLER_SCHEDULER_PRI_ADV_MIN  175
#define SL_BT_CONTROLLER_SCHEDULER_PRI_ADV_MAX  127
#define SL_BT_CONTROLLER_SCHEDULER_PRI_ADV_STEP 4

#define SL_BT_CONTROLLER_SCHEDULER_PRI_CONN_MIN 135
#define SL_BT_CONTROLLER_SCHEDULER_PRI_CONN_MAX 0

#define SL_BT_CONTROLLER_SCHEDULER_PRI_INIT_MIN 55
#define SL_BT_CONTROLLER_SCHEDULER_PRI_INIT_MAX 15

#define SL_BT_CONTROLLER_SCHEDULER_PRI_RAIL_WINDOW_MIN 16
#define SL_BT_CONTROLLER_SCHEDULER_PRI_RAIL_WINDOW_MAX 32

#define SL_BT_CONTROLLER_SCHEDULER_PRI_PAWR_TX_MIN 15
#define SL_BT_CONTROLLER_SCHEDULER_PRI_PAWR_TX_MAX 5
#define SL_BT_CONTROLLER_SCHEDULER_PRI_PAWR_RX_MIN 20
#define SL_BT_CONTROLLER_SCHEDULER_PRI_PAWR_RX_MAX 10

/* Default priority configuration */
#define SL_BTCTRL_SCHEDULER_PRIORITIES                                                             \
	{.scan_min = SL_BT_CONTROLLER_SCHEDULER_PRI_SCAN_MIN,                                      \
	 .scan_max = SL_BT_CONTROLLER_SCHEDULER_PRI_SCAN_MAX,                                      \
	 .adv_min = SL_BT_CONTROLLER_SCHEDULER_PRI_ADV_MIN,                                        \
	 .adv_max = SL_BT_CONTROLLER_SCHEDULER_PRI_ADV_MAX,                                        \
	 .conn_min = SL_BT_CONTROLLER_SCHEDULER_PRI_CONN_MIN,                                      \
	 .conn_max = SL_BT_CONTROLLER_SCHEDULER_PRI_CONN_MAX,                                      \
	 .init_min = SL_BT_CONTROLLER_SCHEDULER_PRI_INIT_MIN,                                      \
	 .init_max = SL_BT_CONTROLLER_SCHEDULER_PRI_INIT_MAX,                                      \
	 .rail_mapping_offset = SL_BT_CONTROLLER_SCHEDULER_PRI_RAIL_WINDOW_MIN,                    \
	 .rail_mapping_range = (SL_BT_CONTROLLER_SCHEDULER_PRI_RAIL_WINDOW_MAX -                   \
				SL_BT_CONTROLLER_SCHEDULER_PRI_RAIL_WINDOW_MIN),                   \
	 0,                                                                                        \
	 .adv_step = SL_BT_CONTROLLER_SCHEDULER_PRI_ADV_STEP,                                      \
	 .scan_step = SL_BT_CONTROLLER_SCHEDULER_PRI_SCAN_STEP,                                    \
	 .pawr_tx_min = SL_BT_CONTROLLER_SCHEDULER_PRI_PAWR_TX_MIN,                                \
	 .pawr_tx_max = SL_BT_CONTROLLER_SCHEDULER_PRI_PAWR_TX_MAX,                                \
	 .pawr_rx_min = SL_BT_CONTROLLER_SCHEDULER_PRI_PAWR_RX_MIN,                                \
	 .pawr_rx_max = SL_BT_CONTROLLER_SCHEDULER_PRI_PAWR_RX_MAX}

#endif /* SL_BTCTRL_SCHEDULER_PRIORITY_CONFIG_H */
