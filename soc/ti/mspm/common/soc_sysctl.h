/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_TI_MSPM_COMMON_SOC_SYSCTL_H_
#define ZEPHYR_SOC_TI_MSPM_COMMON_SOC_SYSCTL_H_

#include <zephyr/devicetree.h>

/**
 * @brief MSPM SYSCTL soclock Register Offsets
 *
 * Offsets are relative to the SYSCTL base (soclock region base = SYSCTL base + 0x1000).
 */
#define SYSCTL_SYSOSCCFG_OFFSET    0x1100 /**< SYSOSC configuration */
#define SYSCTL_MCLKCFG_OFFSET      0x1104 /**< Main clock configuration */
#define SYSCTL_HSCLKEN_OFFSET      0x1108 /**< HSCLK source enable/disable */
#define SYSCTL_HSCLKCFG_OFFSET     0x110C /**< HSCLK source selection */
#define SYSCTL_HFCLKCLKCFG_OFFSET  0x1110 /**< HFCLK configuration */
#define SYSCTL_LFCLKCFG_OFFSET     0x1114 /**< LFXT configuration */
#define SYSCTL_SYSPLLCFG0_OFFSET   0x1120 /**< SYSPLL ref/output config */
#define SYSCTL_SYSPLLCFG1_OFFSET   0x1124 /**< SYSPLL divider */
#define SYSCTL_SYSPLLPARAM0_OFFSET 0x1128 /**< SYSPLL param0 */
#define SYSCTL_SYSPLLPARAM1_OFFSET 0x112C /**< SYSPLL param1 */

#if defined(CONFIG_SOC_SERIES_MSPM33C)
#define SYSCTL_SYSPLLPARAM2_OFFSET  0x1130 /**< SYSPLL param2 */
#define SYSCTL_SYSPLLLDOCTL_OFFSET  0x1134 /**< SYSPLL LDO ctl */
#define SYSCTL_SYSPLLLDOPROG_OFFSET 0x1138 /**< SYSPLL LDO VOUT PROG */
#define SYSCTL_GENCLKEN_OFFSET      0x113C /**< General clock enable */
#define SYSCTL_GENCLKCFG_OFFSET     0x1140 /**< General clock configuration */
#else
#define SYSCTL_GENCLKCFG_OFFSET 0x1138 /**< General clock configuration */
#define SYSCTL_GENCLKEN_OFFSET  0x113C /**< General clock enable */
#endif

#define SYSCTL_SYSOSCTRIMUSER_OFFSET 0x1170 /**< SYSOSC user-specified trim */
#define SYSCTL_CLKSTATUS_OFFSET      0x1204 /**< Clock module status */
#define SYSCTL_SYSSTATUS_OFFSET      0x1208 /**< System status */
#define SYSCTL_RSTCAUSE_OFFSET       0x1220 /**< Reset Cause */
#define SYSCTL_LFXTCTL_OFFSET        0x1314 /**< LFXT and LFCLK control */
#define SYSCTL_EXLFCTL_OFFSET        0x1318 /**< LFCLK_IN control */

/* sysosccfg bits */
#define SYSCTL_SYSOSCCFG_DISABLE       BIT(10)
#define SYSCTL_SYSOSCCFG_FREQ          GENMASK(1, 0)
#define SYSCTL_SYSOSCCFG_FREQ_BASE     0x0U /* 32 MHz */
#define SYSCTL_SYSOSCCFG_FREQ_4M       0x1U /* 4 MHz */
#define SYSCTL_SYSOSCCFG_FREQ_USERTRIM 0x2U /* 16 or 24 MHz, needs sysosctrimuser */

/* sysosctrimuser bits */
#define SYSCTL_SYSOSCTRIMUSER_FREQ     GENMASK(1, 0)
#define SYSCTL_SYSOSCTRIMUSER_FREQ_16M 0x1U
#define SYSCTL_SYSOSCTRIMUSER_FREQ_24M 0x2U

/* lfclkcfg bits */
#define SYSCTL_LFCLKCFG_LOWCAP   BIT(8)
#define SYSCTL_LFCLKCFG_XT1DRIVE GENMASK(1, 0)

/* syspllcfg0 bits */
#if defined(CONFIG_SOC_SERIES_MSPM33C)
#define SYSCTL_SYSPLLCFG0_RDIVCLK2X GENMASK(11, 8)
#define SYSCTL_SYSPLLCFG0_RDIVCLK1  GENMASK(15, 12)
#define SYSCTL_SYSPLLCFG0_RDIVCLK0  GENMASK(19, 16)
#else
#define SYSCTL_SYSPLLCFG0_RDIVCLK2X GENMASK(19, 16)
#define SYSCTL_SYSPLLCFG0_RDIVCLK1  GENMASK(15, 12)
#define SYSCTL_SYSPLLCFG0_RDIVCLK0  GENMASK(11, 8)
#endif /* defined(CONFIG_SOC_SERIES_MSPM33C) */

#define SYSCTL_SYSPLLCFG0_RDIVCLK2X_VAL(x) (x - 1)
#define SYSCTL_SYSPLLCFG0_RDIVCLK1_VAL(x)  ((x / 2) - 1)
#define SYSCTL_SYSPLLCFG0_RDIVCLK0_VAL(x)  ((x / 2) - 1)
#define SYSCTL_SYSPLLCFG0_ENABLECLK2X      BIT(6)
#define SYSCTL_SYSPLLCFG0_ENABLECLK1       BIT(5)
#define SYSCTL_SYSPLLCFG0_ENABLECLK0       BIT(4)
#define SYSCTL_SYSPLLCFG0_MCLK2XVCO        BIT(1)
#define SYSCTL_SYSPLLCFG0_SYSPLLREF        BIT(0)

/* syspllcfg1 bits */
#define SYSCTL_SYSPLLCFG1_QDIV        GENMASK(14, 8)
#define SYSCTL_SYSPLLCFG1_QDIV_VAL(x) (x - 1)
#define SYSCTL_SYSPLLCFG1_PDIV        GENMASK(1, 0)
#define SYSCTL_SYSPLLCFG1_PDIV_VAL(x) LOG2(x)

/* mclkcfg bits */
#define SYSCTL_MCLKCFG_USEHSCLK           BIT(16)
#define SYSCTL_MCLKCFG_USELFCLK           BIT(20)
#define SYSCTL_MCLKCFG_MCLKDIVCFG         GENMASK(26, 24)
#define SYSCTL_MCLKCFG_MCLKDIVCFG_VAL_1_1 (0x0)
#define SYSCTL_MCLKCFG_MCLKDIVCFG_VAL_1_2 (0x1)
#define SYSCTL_MCLKCFG_MCLKDIVCFG_VAL_1_4 (0x3)
#define SYSCTL_MCLKCFG_MCLKDIVCFG_VAL_2_2 (0x5)
#define SYSCTL_MCLKCFG_MCLKDIVCFG_VAL_2_4 (0x7)
#define SYSCTL_MCLKCFG_UDIV               GENMASK(5, 4)
#define SYSCTL_MCLKCFG_UDIV_VAL(x)        (x - 1)
#define SYSCTL_MCLKCFG_MDIV               GENMASK(3, 0)
#define SYSCTL_MCLKCFG_MDIV_VAL(x)        (x - 1)

/* genclkcfg bits */
#define SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV        GENMASK(15, 12)
#define SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV_VAL(x) (x - 1)
#define SYSCTL_GENCLKCFG_MFPCLKSRC              BIT(9)
#define SYSCTL_GENCLKCFG_CANCLKSRC              BIT(8)

/* genclken bits */
#define SYSCTL_GENCLKEN_MFPCLKEN BIT(4)

/* hsclken bits */
#define SYSCTL_HSCLKEN_USEEXTHFCLK BIT(16)
#define SYSCTL_HSCLKEN_SYSPLLEN    BIT(8)
#define SYSCTL_HSCLKEN_HFXTEN      BIT(0)

/* hsclkcfg bits */
#define SYSCTL_HSCLKCFG_HSCLKSEL BIT(0)

/* hfclkclkcfg bits */
#define SYSCTL_HFCLKCLKCFG_HFCLKFLTCHK        BIT(28)
#define SYSCTL_HFCLKCLKCFG_HFXTRSEL           GENMASK(13, 12)
#define SYSCTL_HFCLKCLKCFG_HFXTRSEL_4_8_MHZ   (0x0)
#define SYSCTL_HFCLKCLKCFG_HFXTRSEL_8_16_MHZ  (0x1)
#define SYSCTL_HFCLKCLKCFG_HFXTRSEL_16_32_MHZ (0x2)
#define SYSCTL_HFCLKCLKCFG_HFXTRSEL_32_48_MHZ (0x3)
#define SYSCTL_HFCLKCLKCFG_HFXTTIME           GENMASK(7, 0)
#define SYSCTL_HFCLKCLKCFG_HFXTTIME_VAL(x)    (x >> 6)

/* clkstatus bits */
#define SYSCTL_CLKSTATUS_LFCLKFAIL            BIT(23)
#define SYSCTL_CLKSTATUS_HSCLKGOOD            BIT(21)
#define SYSCTL_CLKSTATUS_HSCLKDEAD            BIT(20)
#define SYSCTL_CLKSTATUS_CURMCLKSEL           BIT(17)
#define SYSCTL_CLKSTATUS_CURHSCLKSEL          BIT(16)
#define SYSCTL_CLKSTATUS_SYSPLLOFF            BIT(14)
#define SYSCTL_CLKSTATUS_HFCLKOFF             BIT(13)
#define SYSCTL_CLKSTATUS_LFOSCGOOD            BIT(11)
#define SYSCTL_CLKSTATUS_LFXTGOOD             BIT(10)
#define SYSCTL_CLKSTATUS_SYSPLLGOOD           BIT(9)
#define SYSCTL_CLKSTATUS_HFCLKGOOD            BIT(8)
#define SYSCTL_CLKSTATUS_LFCLKMUX             GENMASK(7, 6)
#define SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFOSC   0x0U
#define SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFXT    0x1U
#define SYSCTL_CLKSTATUS_LFCLKMUX_VAL_LFCLKIN 0x2U
#define SYSCTL_CLKSTATUS_HSCLKMUX             BIT(4)

/* lfxtctl bits */
#define SYSCTL_LFXTCTL_KEY        GENMASK(31, 24)
#define SYSCTL_LFXTCTL_KEY_VAL    (0x91)
#define SYSCTL_LFXTCTL_SETUSELFXT BIT(1)
#define SYSCTL_LFXTCTL_STARTLFXT  BIT(0)

/* exlfctl bits */
#define SYSCTL_EXLFCTL_KEY        GENMASK(31, 24)
#define SYSCTL_EXLFCTL_KEY_VAL    (0x36)
#define SYSCTL_EXLFCTL_SETUSEEXLF BIT(0)

#endif /* ZEPHYR_SOC_TI_MSPM_COMMON_SOC_SYSCTL_H_ */
