/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This configuration header is used by the Silicon Labs Bluetooth Controller init functions.
 * It is used to configure PHY support options.
 */

#ifndef SL_BTCTRL_PHY_SUPPORT_CONFIG_H
#define SL_BTCTRL_PHY_SUPPORT_CONFIG_H

#if defined(CONFIG_BT_CTLR_PHY_2M)
#define SL_BT_CONTROLLER_2M_PHY_SUPPORT 1
#else
#define SL_BT_CONTROLLER_2M_PHY_SUPPORT 0
#endif

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#define SL_BT_CONTROLLER_CODED_PHY_SUPPORT 1
#else
#define SL_BT_CONTROLLER_CODED_PHY_SUPPORT 0
#endif

#endif /* SL_BTCTRL_PHY_SUPPORT_CONFIG_H */
