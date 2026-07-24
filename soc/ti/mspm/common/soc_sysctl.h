/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_TI_MSPM_COMMON_SOC_SYSCTL_H_
#define ZEPHYR_SOC_TI_MSPM_COMMON_SOC_SYSCTL_H_

#include <zephyr/devicetree.h>

/**
 * @brief MSPM SYSCTL SOCLOCK Region Register Layout
 *
 * Offsets are relative to the SOCLOCK region base (SYSCTL base + 0x1000).
 */
struct mspm_sysctl_soclock_regs {
	uint8_t RESERVED0[0x100];    /**< Reserved, offset: 0x0000 - 0x0100 */
	volatile uint32_t SYSOSCCFG; /**< SYSOSC configuration, offset: 0x0100 */
	volatile uint32_t MCLKCFG;   /**< Main clock configuration, offset: 0x0104 */

#if defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_L122X_L222X) ||                                         \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_L111X) ||                                           \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_G1X0X_G3X0X) ||                                     \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_GX51X) ||                                           \
	defined(CONFIG_SOC_MSPM33_DEVICE_FAMILY_C321X)
	volatile uint32_t HSCLKEN; /**< HSCLK source enable/disable, offset: 0x0108 */
#else
	uint8_t RESERVED1[0x4]; /**< Reserved, offset: 0x0108 - 0x010C */
#endif

#if defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_L122X_L222X) ||                                         \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_G1X0X_G3X0X) ||                                     \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_GX51X) ||                                           \
	defined(CONFIG_SOC_MSPM33_DEVICE_FAMILY_C321X)
	volatile uint32_t HSCLKCFG;    /**< HSCLK source selection, offset: 0x010C */
	volatile uint32_t HFCLKCLKCFG; /**< HFCLK configuration, offset: 0x0110 */
#else
	uint8_t RESERVED2[0x8]; /**< Reserved, offset: 0x010C - 0x0114 */
#endif

#if defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_L122X_L222X) ||                                         \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_L111X) ||                                           \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_G1X0X_G3X0X) ||                                     \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_GX51X) ||                                           \
	defined(CONFIG_SOC_MSPM33_DEVICE_FAMILY_C321X)
	volatile uint32_t LFCLKCFG; /**< LFXT configuration, offset: 0x0114 */
#else
	uint8_t RESERVED3[0x4]; /**< Reserved, offset: 0x0114 - 0x0118 */
#endif

	uint8_t RESERVED4[0x8]; /**< Reserved, offset: 0x0118 - 0x0120 */

#if defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_G1X0X_G3X0X) ||                                         \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_GX51X) ||                                           \
	defined(CONFIG_SOC_MSPM33_DEVICE_FAMILY_C321X)

	volatile uint32_t SYSPLLCFG0;   /**< SYSPLL ref/output config, offset: 0x0120 */
	volatile uint32_t SYSPLLCFG1;   /**< SYSPLL divider, offset: 0x0124 */
	volatile uint32_t SYSPLLPARAM0; /**< SYSPLL param0, offset: 0x0128 */
	volatile uint32_t SYSPLLPARAM1; /**< SYSPLL param1, offset: 0x012C */
#else
	uint8_t RESERVED5[0x10]; /**< Reserved, offset: 0x0120 - 0x0130 */
#endif

#if defined(CONFIG_SOC_MSPM33_DEVICE_FAMILY_C321X)
	volatile uint32_t SYSPLLPARAM2;  /**< SYSPLL param2, offset: 0x0130 */
	volatile uint32_t SYSPLLLDOCTL;  /**< SYSPLL LDO CTL, offset: 0x0134 */
	volatile uint32_t SYSPLLLDOPROG; /**< SYSPLL LDO VOUT PROG, offset: 0x0138 */
	volatile uint32_t GENCLKEN;      /**< General clock enable, offset: 0x013C */
	volatile uint32_t GENCLKCFG;     /**< General clock configuration, offset: 0x0140 */
	uint8_t RESERVED6[0x10];         /**< Reserved, offset: 0x0144 - 0x0154 */
#else
	uint8_t RESERVED6[0x8];      /**< Reserved, offset: 0x0130 - 0x0138 */
	volatile uint32_t GENCLKCFG; /**< General clock configuration, offset: 0x0138 */
	volatile uint32_t GENCLKEN;  /**< General clock enable, offset: 0x013C */
	uint8_t RESERVED7[0x14];     /**< Reserved, offset: 0x0140 - 0x0154 */
#endif

	uint8_t RESERVED8[0xB0];     /**< Reserved, offset: 0x0154 - 0x0204 */
	volatile uint32_t CLKSTATUS; /**< Clock module status, offset: 0x0204 */
	uint8_t RESERVED9[0x10C];    /**< Reserved, offset: 0x0208 - 0x0314 */

#if defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_L122X_L222X) ||                                         \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_L111X) ||                                           \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_G1X0X_G3X0X) ||                                     \
	defined(CONFIG_SOC_MSPM0_DEVICE_FAMILY_GX51X) ||                                           \
	defined(CONFIG_SOC_MSPM33_DEVICE_FAMILY_C321X)
	volatile uint32_t LFXTCTL; /**< LFXT and LFCLK control, offset: 0x0314 */
	volatile uint32_t EXLFCTL; /**< LFCLK_IN control, offset: 0x0318 */
#else
	uint8_t RESERVED10[0x8]; /**< Reserved, offset: 0x0314 - 0x031C */
#endif

	uint8_t RESERVED11[0xF44]; /**< Reserved, offset: 0x031C - 0x1248 */
};

/**
 * @brief MSPM SYSCTL Top-Level Register Layout
 *
 * Maps the full SYSCTL peripheral from base address.
 */
struct mspm_sysctl_regs {
	uint8_t RESERVED0[0x1000];               /**< Reserved, offset: 0x0000 - 0x1000 */
	struct mspm_sysctl_soclock_regs SOCLOCK; /**< SOCLOCK region, offset: 0x1000 */
};

#define MSPM_SYSCTL_NODE DT_NODELABEL(sysctl)
#define MSPM_SYSCTL_ADDR DT_REG_ADDR(MSPM_SYSCTL_NODE)
#define MSPM_SYSCTL_REGS ((volatile struct mspm_sysctl_regs *)MSPM_SYSCTL_ADDR)

/* SYSOSCCFG bits */
#define SYSCTL_SYSOSCCFG_DISABLE   BIT(10)
#define SYSCTL_SYSOSCCFG_FREQ      GENMASK(1, 0)
#define SYSCTL_SYSOSCCFG_FREQ_BASE 0x0U /* 32 MHz */
#define SYSCTL_SYSOSCCFG_FREQ_4M   0x1U /* 4 MHz */

/* LFCLKCFG bits */
#define SYSCTL_LFCLKCFG_LOWCAP   BIT(8)
#define SYSCTL_LFCLKCFG_XT1DRIVE GENMASK(1, 0)

/* SYSPLLCFG0 bits */
#define SYSCTL_SYSPLLCFG0_RDIVCLK2X   GENMASK(19, 16)
#define SYSCTL_SYSPLLCFG0_RDIVCLK1    GENMASK(15, 12)
#define SYSCTL_SYSPLLCFG0_RDIVCLK0    GENMASK(11, 8)
#define SYSCTL_SYSPLLCFG0_RDIV_VAL(x) (x - 1)
#define SYSCTL_SYSPLLCFG0_ENABLECLK2X BIT(6)
#define SYSCTL_SYSPLLCFG0_ENABLECLK1  BIT(5)
#define SYSCTL_SYSPLLCFG0_ENABLECLK0  BIT(4)
#define SYSCTL_SYSPLLCFG0_MCLK2XVCO   BIT(1)
#define SYSCTL_SYSPLLCFG0_SYSPLLREF   BIT(0)

/* SYSPLLCFG1 bits */
#define SYSCTL_SYSPLLCFG1_QDIV        GENMASK(14, 8)
#define SYSCTL_SYSPLLCFG1_QDIV_VAL(x) (x - 1)
#define SYSCTL_SYSPLLCFG1_PDIV        GENMASK(1, 0)
#define SYSCTL_SYSPLLCFG1_PDIV_VAL(x) LOG2(x)

/* MCLKCFG bits */
#define SYSCTL_MCLKCFG_USEHSCLK    BIT(16)
#define SYSCTL_MCLKCFG_USELFCLK    BIT(20)
#define SYSCTL_MCLKCFG_UDIV        GENMASK(5, 4)
#define SYSCTL_MCLKCFG_UDIV_VAL(x) (x - 1)
#define SYSCTL_MCLKCFG_MDIV        GENMASK(3, 0)
#define SYSCTL_MCLKCFG_MDIV_VAL(x) (x - 1)

/* GENCLKCFG bits */
#define SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV        GENMASK(15, 12)
#define SYSCTL_GENCLKCFG_HFCLK4MFPCLKDIV_VAL(x) (x - 1)
#define SYSCTL_GENCLKCFG_MFPCLKSRC              BIT(9)

/* GENCLKEN bits */
#define SYSCTL_GENCLKEN_MFPCLKEN BIT(4)

/* HSCLKEN bits */
#define SYSCTL_HSCLKEN_USEEXTHFCLK BIT(16)
#define SYSCTL_HSCLKEN_SYSPLLEN    BIT(8)
#define SYSCTL_HSCLKEN_HFXTEN      BIT(0)

/* HSCLKCFG bits */
#define SYSCTL_HSCLKCFG_HSCLKSEL BIT(0)

/* HFCLKCLKCFG bits */
#define SYSCTL_HFCLKCLKCFG_HFCLKFLTCHK        BIT(28)
#define SYSCTL_HFCLKCLKCFG_HFXTRSEL           GENMASK(13, 12)
#define SYSCTL_HFCLKCLKCFG_HFXTRSEL_4_8_MHZ   (0x0)
#define SYSCTL_HFCLKCLKCFG_HFXTRSEL_8_16_MHZ  (0x1)
#define SYSCTL_HFCLKCLKCFG_HFXTRSEL_16_32_MHZ (0x2)
#define SYSCTL_HFCLKCLKCFG_HFXTRSEL_32_48_MHZ (0x3)
#define SYSCTL_HFCLKCLKCFG_HFXTTIME           GENMASK(7, 0)
#define SYSCTL_HFCLKCLKCFG_HFXTTIME_VAL(x)    (x >> 6)

/* CLKSTATUS bits */
#define SYSCTL_CLKSTATUS_HSCLKGOOD  BIT(21)
#define SYSCTL_CLKSTATUS_CURMCLKSEL BIT(17)
#define SYSCTL_CLKSTATUS_SYSPLLOFF  BIT(14)
#define SYSCTL_CLKSTATUS_LFXTGOOD   BIT(10)
#define SYSCTL_CLKSTATUS_HSCLKMUX   BIT(4)
#define SYSCTL_CLKSTATUS_HFCLKGOOD  BIT(8)
#define SYSCTL_CLKSTATUS_SYSPLLGOOD BIT(9)

/* LFXTCTL bits */
#define SYSCTL_LFXTCTL_KEY        GENMASK(31, 24)
#define SYSCTL_LFXTCTL_KEY_VAL    (0x91)
#define SYSCTL_LFXTCTL_SETUSELFXT BIT(1)
#define SYSCTL_LFXTCTL_STARTLFXT  BIT(0)

/* EXLFCTL bits */
#define SYSCTL_EXLFCTL_KEY        GENMASK(31, 24)
#define SYSCTL_EXLFCTL_KEY_VAL    (0x36)
#define SYSCTL_EXLFCTL_SETUSEEXLF BIT(0)

#endif /* ZEPHYR_SOC_TI_MSPM_COMMON_SOC_SYSCTL_H_ */
