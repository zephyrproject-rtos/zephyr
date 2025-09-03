/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define CLK_SOURCE_IHO
#define CLK_SOURCE_PILO

#define IFX_CAT1_CLOCK_BLOCK_IHO   1  /*!< Internal High Speed Oscillator Input Clock */
#define IFX_CAT1_CLOCK_BLOCK_IMO   2  /*!< Internal Main Oscillator Input Clock */
#define IFX_CAT1_CLOCK_BLOCK_ECO   3  /*!< External Crystal Oscillator Input Clock */
#define IFX_CAT1_CLOCK_BLOCK_EXT   4  /*!< External Input Clock */
#define IFX_CAT1_CLOCK_BLOCK_ALTHF 5  /*!< Alternate High Frequency Input Clock */
#define IFX_CAT1_CLOCK_BLOCK_ALTLF 6  /*!< Alternate Low Frequency Input Clock */
#define IFX_CAT1_CLOCK_BLOCK_ILO   7  /*!< Internal Low Speed Oscillator Input Clock */
#define IFX_CAT1_CLOCK_BLOCK_PILO  8  /*!< Precision ILO Input Clock */
#define IFX_CAT1_CLOCK_BLOCK_WCO   9  /*!< Watch Crystal Oscillator Input Clock */
#define IFX_CAT1_CLOCK_BLOCK_MFO   10 /*!< Medium Frequency Oscillator Clock */

#define IFX_CAT1_CLOCK_BLOCK_PATHMUX 11 /*!< Path selection mux for input to FLL/PLLs */

#define IFX_CAT1_CLOCK_BLOCK_FLL           12 /*!< Frequency-Locked Loop Clock */
#define IFX_CAT1_CLOCK_BLOCK_PLL200        13 /*!< 200MHz Phase-Locked Loop Clock */
#define IFX_CAT1_CLOCK_BLOCK_PLL400        14 /*!< 400MHz Phase-Locked Loop Clock */
#define IFX_CAT1_CLOCK_BLOCK_ECO_PRESCALER 15 /*!< ECO Prescaler Divider */

#define IFX_CAT1_CLOCK_BLOCK_LF 16 /*!< Low Frequency Clock */
#define IFX_CAT1_CLOCK_BLOCK_MF 17 /*!< Medium Frequency Clock */
#define IFX_CAT1_CLOCK_BLOCK_HF 18 /*!< High Frequency Clock */

#define IFX_CAT1_CLOCK_BLOCK_PUMP         19 /*!< Analog Pump Clock */
#define IFX_CAT1_CLOCK_BLOCK_BAK          20 /*!< Backup Power Domain Clock */
#define IFX_CAT1_CLOCK_BLOCK_ALT_SYS_TICK 21 /*!< Alternative SysTick Clock */
#define IFX_CAT1_CLOCK_BLOCK_PERI         22 /*!< Peripheral Clock Group */

#define IFX_CAT1_CLKHF_NO_DIVIDE    0  /**< don't divide clkHf */
#define IFX_CAT1_CLKHF_DIVIDE_BY_2  1  /**< divide clkHf by 2 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_3  2  /**< divide clkHf by 3 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_4  3  /**< divide clkHf by 4 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_5  4  /**< divide clkHf by 5 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_6  5  /**< divide clkHf by 6 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_7  6  /**< divide clkHf by 7 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_8  7  /**< divide clkHf by 8 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_9  8  /**< divide clkHf by 9 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_10 9  /**< divide clkHf by 10 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_11 10 /**< divide clkHf by 11 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_12 11 /**< divide clkHf by 12 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_13 12 /**< divide clkHf by 13 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_14 13 /**< divide clkHf by 14 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_15 14 /**< divide clkHf by 15 */
#define IFX_CAT1_CLKHF_DIVIDE_BY_16 15 /**< divide clkHf by 16 */
#define IFX_CAT1_CLKHF_MAX_DIVIDER     /**< Max divider */

#define IFX_CAT1_CLKPATH_IN_IMO    0 /**< Select the IMO as the output of the path mux */
#define IFX_CAT1_CLKPATH_IN_EXT    1 /**< Select the EXT as the output of the path mux */
#define IFX_CAT1_CLKPATH_IN_ECO    2 /**< Select the ECO as the output of the path mux */
#define IFX_CAT1_CLKPATH_IN_ALTHF  3 /**< Select the ALTHF as the output of the path mux */
/* Select the DSI MUX output as the output of the path mux */
#define IFX_CAT1_CLKPATH_IN_DSIMUX 4
#define IFX_CAT1_CLKPATH_IN_LPECO  5 /**< Select the LPECO as the output of the path mux */
#define IFX_CAT1_CLKPATH_IN_IHO    6 /**< Select the IHO as the output of the path mux */
/* Select a DSI signal (0 - 15) as the output of the DSI mux and path mux.         \
 *   Make sure the DSI clock sources are available on used device.                   \
 */
#define IFX_CAT1_CLKPATH_IN_DSI    0x100
/**< Select the ILO (16) as the output of the DSI mux and path mux */
#define IFX_CAT1_CLKPATH_IN_ILO    0x110
/**< Select the WCO (17) as the output of the DSI mux and path mux */
#define IFX_CAT1_CLKPATH_IN_WCO    0x111
/**< Select the ALTLF (18) as the output of the DSI mux and path mux.                \
 *   Make sure the ALTLF clock sources in available on used device.                  \
 */
#define IFX_CAT1_CLKPATH_IN_ALTLF  0x112
/**< Select the PILO (19) as the output of the DSI mux and path mux.                 \
 *   Make sure the PILO clock sources in available on used device.                   \
 */
#define IFX_CAT1_CLKPATH_IN_PILO   0x113
/**< Select the ILO1 (20) as the output of the DSI mux and path mux */
#define IFX_CAT1_CLKPATH_IN_ILO1   0x114
