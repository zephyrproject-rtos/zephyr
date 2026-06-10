/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Power domain definitions for NXP i.MX952 SoC
 *
 * This file defines power domain IDs for the i.MX952 system-on-chip.
 * These definitions are based on MIMX9529 fsl_power.h from MCUXpresso SDK.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_IMX952_POWER_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_IMX952_POWER_H_

/**
 * @defgroup imx952_power_domains Power Domain IDs
 * @brief Power domain slice indices for i.MX952 SoC (21 total)
 * @{
 */

/** Analog power domain */
#define IMX952_PD_ANA           0
/** Always-on power domain */
#define IMX952_PD_AON           1
/** BBSM (Battery-Backed Security Module) power domain */
#define IMX952_PD_BBSM          2
/** Camera subsystem power domain */
#define IMX952_PD_CAMERA        3
/** CCM/SRC/GPC power domain */
#define IMX952_PD_CCMSRCGPC     4
/** A55 Core 0 power domain */
#define IMX952_PD_A55C0         5
/** A55 Core 1 power domain */
#define IMX952_PD_A55C1         6
/** A55 Core 2 power domain */
#define IMX952_PD_A55C2         7
/** A55 Core 3 power domain */
#define IMX952_PD_A55C3         8
/** A55 Platform power domain */
#define IMX952_PD_A55P          9
/** DDR subsystem power domain */
#define IMX952_PD_DDR           10
/** Display subsystem power domain */
#define IMX952_PD_DISPLAY       11
/** GPU power domain */
#define IMX952_PD_GPU           12
/** HSIO Top power domain */
#define IMX952_PD_HSIO_TOP      13
/** HSIO Wakeup-AON power domain */
#define IMX952_PD_HSIO_WAON     14
/** M7 core power domain */
#define IMX952_PD_M7            15
/** NETC (Network Controller) power domain */
#define IMX952_PD_NETC          16
/** NOC (Network-on-Chip) power domain */
#define IMX952_PD_NOC           17
/** NPU (Neural Processing Unit) power domain */
#define IMX952_PD_NPU           18
/** VPU (Video Processing Unit) power domain */
#define IMX952_PD_VPU           19
/** Wakeup domain power domain */
#define IMX952_PD_WAKEUP        20

/** Total number of MIX power domain slices */
#define IMX952_NUM_MIX_SLICE    21

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_IMX952_POWER_H_ */
