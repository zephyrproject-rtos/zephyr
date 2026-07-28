/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_TI_MSPM_COMMON_SOC_SYSCTL_H_
#define ZEPHYR_SOC_TI_MSPM_COMMON_SOC_SYSCTL_H_

#include <zephyr/devicetree.h>

/**
 * @brief MSPM SYSCTL soclock Region Register Layout
 *
 * Offsets are relative to the soclock region base (SYSCTL base + 0x1000).
 */
struct mspm_sysctl_soclock_regs {
	uint8_t reserved0[0x100];       /**< Reserved, offset: 0x0000 - 0x0100 */
	volatile uint32_t sysosccfg;    /**< SYSOSC configuration, offset: 0x0100 */
	volatile uint32_t mclkcfg;      /**< Main clock configuration, offset: 0x0104 */
	volatile uint32_t hsclken;      /**< HSCLK source enable/disable, offset: 0x0108 */
	volatile uint32_t hsclkcfg;     /**< HSCLK source selection, offset: 0x010C */
	volatile uint32_t hfclkclkcfg;  /**< HFCLK configuration, offset: 0x0110 */
	volatile uint32_t lfclkcfg;     /**< LFXT configuration, offset: 0x0114 */
	uint8_t reserved4[0x8];         /**< Reserved, offset: 0x0118 - 0x0120 */
	volatile uint32_t syspllcfg0;   /**< SYSPLL ref/output config, offset: 0x0120 */
	volatile uint32_t syspllcfg1;   /**< SYSPLL divider, offset: 0x0124 */
	volatile uint32_t syspllparam0; /**< SYSPLL param0, offset: 0x0128 */
	volatile uint32_t syspllparam1; /**< SYSPLL param1, offset: 0x012C */

#if defined(CONFIG_SOC_SERIES_MSPM33C)
	volatile uint32_t syspllparam2;  /**< SYSPLL param2, offset: 0x0130 */
	volatile uint32_t syspllldoctl;  /**< SYSPLL LDO ctl, offset: 0x0134 */
	volatile uint32_t syspllldoprog; /**< SYSPLL LDO VOUT PROG, offset: 0x0138 */
	volatile uint32_t genclken;      /**< General clock enable, offset: 0x013C */
	volatile uint32_t genclkcfg;     /**< General clock configuration, offset: 0x0140 */
	uint8_t reserved6[0x10];         /**< Reserved, offset: 0x0144 - 0x0154 */
#else
	uint8_t reserved6[0x8];      /**< Reserved, offset: 0x0130 - 0x0138 */
	volatile uint32_t genclkcfg; /**< General clock configuration, offset: 0x0138 */
	volatile uint32_t genclken;  /**< General clock enable, offset: 0x013C */
	uint8_t reserved7[0x14];     /**< Reserved, offset: 0x0140 - 0x0154 */
#endif

	uint8_t reserved8[0x1C];          /**< Reserved, offset: 0x0154 - 0x0170 */
	volatile uint32_t sysosctrimuser; /**< SYSOSC user-specified trim, offset: 0x0170 */
	uint8_t reserved9[0x90];          /**< Reserved, offset: 0x0174 - 0x0204 */
	volatile uint32_t clkstatus;      /**< Clock module status, offset: 0x0204 */
	volatile uint32_t sysstatus;      /**< System status, offset: 0x0208 */
	uint8_t reserved10[0x14];         /**< Reserved, offset: 0x020C - 0x0220 */
	volatile uint32_t rstcause;       /**< Reset Cause, offset: 0x220 */
	uint8_t reserved11[0xF0];         /**< Reserved, offset: 0x0224 - 0x0314 */
	volatile uint32_t lfxtctl;        /**< LFXT and LFCLK control, offset: 0x0314 */
	volatile uint32_t exlfctl;        /**< LFCLK_IN control, offset: 0x0318 */
};

/**
 * @brief MSPM SYSCTL Top-Level Register Layout
 *
 * Maps the full SYSCTL peripheral from base address.
 */
struct mspm_sysctl_regs {
	uint8_t reserved0[0x1000];               /**< Reserved, offset: 0x0000 - 0x1000 */
	struct mspm_sysctl_soclock_regs soclock; /**< soclock region, offset: 0x1000 */
};

#define MSPM_SYSCTL_NODE DT_NODELABEL(sysctl)
#define MSPM_SYSCTL_ADDR DT_REG_ADDR(MSPM_SYSCTL_NODE)
#define MSPM_SYSCTL_REGS ((volatile struct mspm_sysctl_regs *)MSPM_SYSCTL_ADDR)

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
#define SYSCTL_SYSPLLCFG0_RDIVCLK2X        GENMASK(19, 16)
#define SYSCTL_SYSPLLCFG0_RDIVCLK1         GENMASK(15, 12)
#define SYSCTL_SYSPLLCFG0_RDIVCLK0         GENMASK(11, 8)
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
#define SYSCTL_MCLKCFG_USEHSCLK    BIT(16)
#define SYSCTL_MCLKCFG_USELFCLK    BIT(20)
#define SYSCTL_MCLKCFG_UDIV        GENMASK(5, 4)
#define SYSCTL_MCLKCFG_UDIV_VAL(x) (x - 1)
#define SYSCTL_MCLKCFG_MDIV        GENMASK(3, 0)
#define SYSCTL_MCLKCFG_MDIV_VAL(x) (x - 1)

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
