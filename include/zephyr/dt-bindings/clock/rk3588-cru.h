/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * RK3588 CRU clock identifiers used by the GMAC controllers, and the SCMI
 * clock identifiers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RK3588_CRU_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RK3588_CRU_H_

#define MCLK_GMAC0_OUT    248
#define CLK_GMAC0_PTP_REF 308
#define CLK_GMAC1_PTP_REF 309
#define CLK_GMAC_125M     310
#define CLK_GMAC_50M      311
#define PCLK_GMAC0        344
#define PCLK_GMAC1        345
#define ACLK_GMAC0        349
#define ACLK_GMAC1        350

/* Clocks the SCMI clock protocol of TF-A serves; the numbers are the firmware's. */
#define SCMI_CLK_CPUL       0
#define SCMI_CLK_DSU        1
#define SCMI_CLK_CPUB01     2
#define SCMI_CLK_CPUB23     3
#define SCMI_CLK_DDR        4
#define SCMI_CLK_GPU        5
#define SCMI_CLK_NPU        6
#define SCMI_CLK_SBUS       7
#define SCMI_PCLK_SBUS      8
#define SCMI_CCLK_SD        9
#define SCMI_DCLK_SD        10
#define SCMI_ACLK_SECURE_NS 11
#define SCMI_HCLK_SECURE_NS 12
#define SCMI_TCLK_WDT       13
#define SCMI_KEYLADDER_CORE 14
#define SCMI_KEYLADDER_RNG  15
#define SCMI_ACLK_SECURE_S  16
#define SCMI_HCLK_SECURE_S  17
#define SCMI_PCLK_SECURE_S  18
#define SCMI_CRYPTO_RNG     19
#define SCMI_CRYPTO_CORE    20
#define SCMI_CRYPTO_PKA     21
#define SCMI_SPLL           22
#define SCMI_HCLK_SD        23

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RK3588_CRU_H_ */
