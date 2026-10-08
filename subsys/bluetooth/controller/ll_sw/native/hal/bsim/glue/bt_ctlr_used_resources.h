/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HAL_BSIM_GLUE_BT_CTLR_USED_RESOURCES_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HAL_BSIM_GLUE_BT_CTLR_USED_RESOURCES_H_

/* The BabbleSim LLL does not use any nRF PPI channels or groups. Provided
 * because nrfx_reserved_resources.h includes it when the split controller is
 * enabled on an nRF based board.
 */
#define BT_CTLR_USED_PPI_CHANNELS 0U
#define BT_CTLR_USED_PPI_GROUPS   0U

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HAL_BSIM_GLUE_BT_CTLR_USED_RESOURCES_H_ */
