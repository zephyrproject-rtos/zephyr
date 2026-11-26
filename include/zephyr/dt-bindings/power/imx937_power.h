/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Power domain definitions for NXP i.MX937 SoC
 * @ingroup imx937_power_domains
 *
 * This file defines power domain IDs for the i.MX937 system-on-chip.
 * These definitions are based on MIMX9375 fsl_power.h from MCUXpresso SDK.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_IMX937_POWER_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_IMX937_POWER_H_

/**
 * @defgroup imx937_power_domains  NXP i.MX937 Power Domains
 * @brief Power domain slice indices for i.MX937 SoC
 * @ingroup devicetree
 * @{
 */

/** Analog power domain */
#define IMX937_PD_ANA           0
/** Always-on power domain */
#define IMX937_PD_AON           1
/** BBSM (Battery-Backed Security Module) power domain */
#define IMX937_PD_BBSM          2
/** Camera subsystem power domain */
#define IMX937_PD_CAMERA        3
/** CCM/SRC/GPC power domain */
#define IMX937_PD_CCMSRCGPC     4
/** A55 Core 0 power domain */
#define IMX937_PD_A55C0         5
/** A55 Core 1 power domain */
#define IMX937_PD_A55C1         6
/** A55 Core 2 power domain */
#define IMX937_PD_A55C2         7
/** A55 Core 3 power domain */
#define IMX937_PD_A55C3         8
/** A55 Platform power domain */
#define IMX937_PD_A55P          9
/** DDR subsystem power domain */
#define IMX937_PD_DDR           10
/** Display subsystem power domain */
#define IMX937_PD_DISPLAY       11
/** GPU power domain */
#define IMX937_PD_GPU           12
/** HSIO Top power domain */
#define IMX937_PD_HSIO_TOP      13
/** HSIO Wakeup-AON power domain */
#define IMX937_PD_HSIO_WAON     14
/** M7 core power domain */
#define IMX937_PD_M7            15
/** NETC (Network Controller) power domain */
#define IMX937_PD_NETC          16
/** NOC (Network-on-Chip) power domain */
#define IMX937_PD_NOC           17
/** NPU (Neural Processing Unit) power domain */
#define IMX937_PD_NPU           18
/** VPU (Video Processing Unit) power domain */
#define IMX937_PD_VPU           19
/** Wakeup domain power domain */
#define IMX937_PD_WAKEUP        20

/** Total number of MIX power domain slices */
#define IMX937_NUM_MIX_SLICE    21

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_IMX937_POWER_H_ */
