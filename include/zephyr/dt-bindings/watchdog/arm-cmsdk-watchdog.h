/*
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CMSDK watchdog reset-flags values for devicetree
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_WATCHDOG_ARM_CMSDK_WATCHDOG_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_WATCHDOG_ARM_CMSDK_WATCHDOG_H_

#define WDT_FLAG_RESET_NONE     0 /**< Do not trigger a reset on watchdog timeout. */
#define WDT_FLAG_RESET_CPU_CORE 1 /**< Reset the CPU core on watchdog timeout. */
#define WDT_FLAG_RESET_SOC      2 /**< Reset the SoC on watchdog timeout. */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_WATCHDOG_ARM_CMSDK_WATCHDOG_H_ */
