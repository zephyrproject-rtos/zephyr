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

/* Rate sources of the peripheral clocks */
#define PLL_CPLL     5
#define PLL_GPLL     6
#define CLK_150M_SRC 224
#define CLK_200M_SRC 225

/* GPIO banks, except GPIO0 at the end of the numbering */
#define PCLK_GPIO1        115
#define DBCLK_GPIO1       116
#define PCLK_GPIO2        117
#define DBCLK_GPIO2       118
#define PCLK_GPIO3        119
#define DBCLK_GPIO3       120
#define PCLK_GPIO4        121
#define DBCLK_GPIO4       122

/* SPI controllers */
#define PCLK_SPI0         146
#define PCLK_SPI1         147
#define PCLK_SPI2         148
#define PCLK_SPI3         149
#define PCLK_SPI4         150
#define CLK_SPI0          151
#define CLK_SPI1          152
#define CLK_SPI2          153
#define CLK_SPI3          154
#define CLK_SPI4          155

#define MCLK_GMAC0_OUT    248

/* eMMC controller */
#define HCLK_EMMC         298
#define ACLK_EMMC         299
#define CCLK_EMMC         300
#define BCLK_EMMC         301
#define TMCLK_EMMC        302

#define CLK_GMAC0_PTP_REF 308
#define CLK_GMAC1_PTP_REF 309
#define CLK_GMAC_125M     310
#define CLK_GMAC_50M      311
#define PCLK_GMAC0        344
#define PCLK_GMAC1        345
#define ACLK_GMAC0        349
#define ACLK_GMAC1        350

/* USB 3.0 OTG0 controller */
#define ACLK_USB3OTG0     402
#define SUSPEND_CLK_USB3OTG0 403
#define REF_CLK_USB3OTG0  404

#define PCLK_GPIO0        625
#define DBCLK_GPIO0       626

/* USB 2.0 PHY of the USB 3.0 OTG0 controller */
#define CLK_USB2PHY_HDPTXRXPHY_REF 672

/* SD card drive and sample phase clocks; the driver gates them only. */
#define SCLK_SDMMC_DRV    685
#define SCLK_SDMMC_SAMPLE 686

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
