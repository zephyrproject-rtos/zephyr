/*
 * Copyright (C) 2026 Advanced Micro Devices, Inc. (AMD)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_XCLK_WIZ_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_XCLK_WIZ_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/kernel.h>

#define CLK_WIZ_STATUS_OFFSET   0x0004U
#define CLK_WIZ_RECONFIG_OFFSET 0x0014U
#define CLK_WIZ_REG1_OFFSET     0x0330U
#define CLK_WIZ_REG2_OFFSET     0x0334U
#define CLK_WIZ_REG3_OFFSET     0x0338U
#define CLK_WIZ_REG11_OFFSET    0x0378U
#define CLK_WIZ_REG12_OFFSET    0x0380U
#define CLK_WIZ_REG13_OFFSET    0x0384U
#define CLK_WIZ_REG15_OFFSET    0x039CU
#define CLK_WIZ_REG16_OFFSET    0x03A0U
#define CLK_WIZ_REG17_OFFSET    0x03A8U
#define CLK_WIZ_REG19_OFFSET    0x03CCU
#define CLK_WIZ_REG25_OFFSET    0x03F0U

#define CLK_WIZ_LOCKED          BIT(0)

#define CLK_WIZ_RECONFIG_LOAD   BIT(0)
#define CLK_WIZ_RECONFIG_SADDR  BIT(1)

#define CLK_WIZ_REG1_PREDIV2        BIT(12)
#define CLK_WIZ_REG1_EN             BIT(9)
#define CLK_WIZ_REG1_MX             BIT(10)
#define CLK_WIZ_REG1_EDGE_SHIFT     8U
#define XCLK_WIZ_REG1_EDGE_MASK     0x100U

#define CLK_WIZ_L_MASK              0x00FFU
#define CLK_WIZ_H_MASK              0xFF00U
#define CLK_WIZ_H_SHIFT             8U

#define CLK_WIZ_REG3_PREDIV2        BIT(11)
#define CLK_WIZ_REG3_USED           BIT(12)
#define CLK_WIZ_REG3_MX             BIT(9)
#define CLK_WIZ_REG3_EDGE_MASK      BIT(8)
#define CLK_WIZ_REG3_P5EN_MASK      BIT(13)
#define CLK_WIZ_P5EN_SHIFT          13U
#define CLK_WIZ_P5FEDGE_SHIFT       15U

#define CLK_WIZ_REG12_EDGE_SHIFT    10U
#define CLK_WIZ_REG12_EDGE_MASK     BIT(10)

#define CLK_WIZ_REG11_CP_MASK       0x0FU
#define CLK_WIZ_REG17_RES_SHIFT     1U
#define CLK_WIZ_REG17_RES_MASK      0x0FU

#define CLK_WIZ_LOCK_FB_DLY_SHIFT   10U
#define CLK_WIZ_LOCK_REF_DLY_SHIFT  10U

#define CLK_WIZ_VCO_MIN_HZ      (2160000000ULL)
#define CLK_WIZ_VCO_MAX_HZ      (4320000000ULL)
#define CLK_WIZ_M_MIN           4U
#define CLK_WIZ_M_MAX           432U
#define CLK_WIZ_D_MIN           1U
#define CLK_WIZ_D_MAX           123U
#define CLK_WIZ_O_MIN           2U
#define CLK_WIZ_O_MAX           511U
#define CLK_WIZ_MAX_OUTPUTS     7U

#endif
