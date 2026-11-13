/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * RK3588 CRU clock identifiers.
 *
 * The identifiers are the clock IDs of the RK3588 CRU clock controller, as
 * used by the RK3588 devicetree.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RK3588_CRU_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RK3588_CRU_H_

#define PCLK_GPIO3                119
#define MCLK_GMAC0_OUT            248
#define PCLK_TOP_ROOT             257
#define CLK_GMAC0_PTP_REF         308
#define CLK_GMAC1_PTP_REF         309
#define CLK_GMAC_125M             310
#define CLK_GMAC_50M              311
#define ACLK_PHP_GIC_ITS          312
#define ACLK_MMU_PCIE             313
#define ACLK_PCIE_1L2_DBI         319
#define ACLK_PCIE_1L2_MSTR        324
#define ACLK_PCIE_1L2_SLV         329
#define PCLK_PCIE_1L2             334
#define CLK_PCIE_AUX4             339
#define PCLK_PHP_ROOT             343
#define PCLK_GMAC0                344
#define PCLK_GMAC1                345
#define ACLK_PCIE_ROOT            346
#define ACLK_PHP_ROOT             347
#define ACLK_PCIE_BRIDGE          348
#define ACLK_GMAC0                349
#define ACLK_GMAC1                350
#define CLK_PIPEPHY0_PIPE_G       364
#define CLK_PCIE1L2_PIPE          371
#define PCLK_PCIE_COMBO_PIPE_PHY0 374
#define CLK_REF_PIPE_PHY0         680

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RK3588_CRU_H_ */
