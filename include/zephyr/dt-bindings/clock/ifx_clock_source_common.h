/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define CLK_SOURCE_IHO
#define CLK_SOURCE_PILO

#define IFX_IHO           1  /*!< Internal High Speed Oscillator Input Clock */
#define IFX_IMO           2  /*!< Internal Main Oscillator Input Clock */
#define IFX_ECO           3  /*!< External Crystal Oscillator Input Clock */
#define IFX_EXT           4  /*!< External Input Clock */
#define IFX_ALTHF         5  /*!< Alternate High Frequency Input Clock */
#define IFX_ALTLF         6  /*!< Alternate Low Frequency Input Clock */
#define IFX_ILO           7  /*!< Internal Low Speed Oscillator Input Clock */
#define IFX_PILO          8  /*!< Precision ILO Input Clock */
#define IFX_WCO           9  /*!< Watch Crystal Oscillator Input Clock */
#define IFX_MFO           10 /*!< Medium Frequency Oscillator Clock */
#define IFX_PATHMUX       11 /*!< Path selection mux for input to FLL/PLLs */
#define IFX_FLL           12 /*!< Frequency-Locked Loop Clock */
#define IFX_PLL200        13 /*!< 200MHz Phase-Locked Loop Clock */
#define IFX_PLL400        14 /*!< 400MHz Phase-Locked Loop Clock */
#define IFX_ECO_PRESCALER 15 /*!< ECO Prescaler Divider */
#define IFX_LF            16 /*!< Low Frequency Clock */
#define IFX_MF            17 /*!< Medium Frequency Clock */
#define IFX_HF            18 /*!< High Frequency Clock */
#define IFX_PUMP          19 /*!< Analog Pump Clock */
#define IFX_BAK           20 /*!< Backup Power Domain Clock */
#define IFX_ALT_SYS_TICK  21 /*!< Alternative SysTick Clock */
#define IFX_PERI          22 /*!< Peripheral Clock Group */
#define IFX_DPLL250_0     23 /*!< 250MHz Digital Phase-Locked Loop Clock 0 */
#define IFX_DPLL250_1     24 /*!< 250MHz Digital Phase-Locked Loop Clock 1 */
#define IFX_DPLL500       25 /*!< 500MHz Digital Phase-Locked Loop Clock */

#define IFX_CLK_HF_NO_DIVIDE    0  /**< don't divide clkHf */
#define IFX_CLK_HF_DIVIDE_BY_2  1  /**< divide clkHf by 2 */
#define IFX_CLK_HF_DIVIDE_BY_3  2  /**< divide clkHf by 3 */
#define IFX_CLK_HF_DIVIDE_BY_4  3  /**< divide clkHf by 4 */
#define IFX_CLK_HF_DIVIDE_BY_5  4  /**< divide clkHf by 5 */
#define IFX_CLK_HF_DIVIDE_BY_6  5  /**< divide clkHf by 6 */
#define IFX_CLK_HF_DIVIDE_BY_7  6  /**< divide clkHf by 7 */
#define IFX_CLK_HF_DIVIDE_BY_8  7  /**< divide clkHf by 8 */
#define IFX_CLK_HF_DIVIDE_BY_9  8  /**< divide clkHf by 9 */
#define IFX_CLK_HF_DIVIDE_BY_10 9  /**< divide clkHf by 10 */
#define IFX_CLK_HF_DIVIDE_BY_11 10 /**< divide clkHf by 11 */
#define IFX_CLK_HF_DIVIDE_BY_12 11 /**< divide clkHf by 12 */
#define IFX_CLK_HF_DIVIDE_BY_13 12 /**< divide clkHf by 13 */
#define IFX_CLK_HF_DIVIDE_BY_14 13 /**< divide clkHf by 14 */
#define IFX_CLK_HF_DIVIDE_BY_15 14 /**< divide clkHf by 15 */
#define IFX_CLK_HF_DIVIDE_BY_16 15 /**< divide clkHf by 16 */
#define IFX_CLK_HF_MAX_DIVIDER     /**< Max divider */

/* Target resource types for peripheral dividers */
#define IFX_RSC_ADC     0  /*!< Analog to digital converter */
#define IFX_RSC_ADCMIC  1  /*!< Analog to digital converter with Analog Mic support */
#define IFX_RSC_BLESS   2  /*!< Bluetooth communications block */
#define IFX_RSC_CAN     3  /*!< CAN communication block */
#define IFX_RSC_CLKPATH 4  /*!< Clock Path. DEPRECATED. */
#define IFX_RSC_CLOCK   5  /*!< Clock */
#define IFX_RSC_CRYPTO  6  /*!< Crypto hardware accelerator */
#define IFX_RSC_DAC     7  /*!< Digital to analog converter */
#define IFX_RSC_DMA     8  /*!< DMA controller */
#define IFX_RSC_DW      9  /*!< Datawire DMA controller */
#define IFX_RSC_ETH     10 /*!< Ethernet communications block */
#define IFX_RSC_GPIO    11 /*!< General purpose I/O pin */
#define IFX_RSC_I2S     12 /*!< I2S communications block */
#define IFX_RSC_I3C     13 /*!< I3C communications block */
#define IFX_RSC_KEYSCAN 14 /*!< KeyScan block */
#define IFX_RSC_LCD     15 /*!< Segment LCD controller */
#define IFX_RSC_LIN     16 /*!< LIN communications block */
#define IFX_RSC_LPCOMP  17 /*!< Low power comparator */
#define IFX_RSC_LPTIMER 18 /*!< Low power timer */
#define IFX_RSC_OPAMP   19 /*!< Opamp */
#define IFX_RSC_PDM     20 /*!< PCM/PDM communications block */
#define IFX_RSC_PTC     21 /*!< Programmable Threshold comparator */
#define IFX_RSC_SMIF    22 /*!< Quad-SPI communications block */
#define IFX_RSC_RTC     23 /*!< Real time clock */
#define IFX_RSC_SCB     24 /*!< Serial Communications Block */
#define IFX_RSC_SDHC    25 /*!< SD Host Controller */
#define IFX_RSC_SDIODEV 26 /*!< SDIO Device Block */
#define IFX_RSC_TCPWM   27 /*!< Timer/Counter/PWM block */
#define IFX_RSC_TDM     28 /*!< TDM block */
#define IFX_RSC_UDB     29 /*!< UDB Array */
#define IFX_RSC_USB     30 /*!< USB communication block */
#define IFX_RSC_INVALID 31 /*!< Placeholder for invalid type */
