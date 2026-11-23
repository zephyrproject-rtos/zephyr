/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RK3588 clock identifiers
 * @ingroup rk3588_clock
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

/**
 * @defgroup rk3588_clock RK3588 SCMI clock identifiers
 * @brief Indices accepted by the TF-A SCMI clock protocol on RK3588.
 * @ingroup devicetree-clocks
 * @{
 */

/** Cortex-A55 cluster clock. */
#define SCMI_CLK_CPUL       0
/** DSU clock. */
#define SCMI_CLK_DSU        1
/** Cortex-A76 cluster 0 and 1 clock. */
#define SCMI_CLK_CPUB01     2
/** Cortex-A76 cluster 2 and 3 clock. */
#define SCMI_CLK_CPUB23     3
/** DDR clock. */
#define SCMI_CLK_DDR        4
/** GPU clock. */
#define SCMI_CLK_GPU        5
/** NPU clock. */
#define SCMI_CLK_NPU        6
/** System bus clock. */
#define SCMI_CLK_SBUS       7
/** System bus peripheral clock. */
#define SCMI_PCLK_SBUS      8
/** SD card controller clock. */
#define SCMI_CCLK_SD        9
/** SD card clock. */
#define SCMI_DCLK_SD        10
/** Non-secure ACLK of the secure subsystem. */
#define SCMI_ACLK_SECURE_NS 11
/** Non-secure HCLK of the secure subsystem. */
#define SCMI_HCLK_SECURE_NS 12
/** Watchdog timer clock. */
#define SCMI_TCLK_WDT       13
/** Keyladder core clock. */
#define SCMI_KEYLADDER_CORE 14
/** Keyladder RNG clock. */
#define SCMI_KEYLADDER_RNG  15
/** Secure ACLK of the secure subsystem. */
#define SCMI_ACLK_SECURE_S  16
/** Secure HCLK of the secure subsystem. */
#define SCMI_HCLK_SECURE_S  17
/** Secure PCLK of the secure subsystem. */
#define SCMI_PCLK_SECURE_S  18
/** Crypto RNG clock. */
#define SCMI_CRYPTO_RNG     19
/** Crypto core clock. */
#define SCMI_CRYPTO_CORE    20
/** Crypto PKA clock. */
#define SCMI_CRYPTO_PKA     21
/** Secure PLL. */
#define SCMI_SPLL           22
/** SD card AHB clock. */
#define SCMI_HCLK_SD        23

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RK3588_CRU_H_ */
