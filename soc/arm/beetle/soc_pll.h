/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the ARM LTD Beetle SoC PLL.
 *
 */

#ifndef _ARM_BEETLE_SOC_PLL_H_
#define _ARM_BEETLE_SOC_PLL_H_

/*
 * This header provides the defines to configure the Beetle PLL.
 *
 * BEETLE PLL main register is the PLLCTRL in the System Control
 *
 * The PLLCTRL relevant bits are:
 * - PLL_OUTPUTDIV [9:8]
 * - PLL_INPUTDIV [20:16]
 * - PLL_FEEDDIV [30:24]
 *
 * The formula to calculate the output frequency of the PLL is:
 * Fout = Fin * PLL_FEEDDIV / (PLL_INPUTDIV * PLL_OUTPUTDIV)
 * The Fin = 24Mhz on Beetle
 *
 * PLL_OUTPUTDIV | 0 1 2 3
 * -----------------------
 *               | 1 2 4 8
 *
 * PLL_INPUTDIV = R[20:16] + 1
 *
 * PLL_FEEDDIV = 2*(R[30:24] + 1)
 *
 * BEETLE PLL has a non bypassable divider by 2 in output
 *
 * BEETLE PLL derived clock is prescaled [1-16]
 */

/* BEETLE PLL Masks */
#define PLL_MAINCLK_ENABLE_Msk      0x1
#define PLL_MAINCLK_DISABLE_Msk     0x1
#define PLL_MAINCLK_PRESCALER_Msk   0xF0

/* BEETLE PLL Configuration */
#define BEETLE_PLL_CONFIGURATION    0x17000200

/* BEETLE PLL Supported Frequencies */
/* BEETLE_PLL_48Mhz */
#define BEETLE_PLL_FREQUENCY_48MHZ  48000000
#define BEETLE_PLL_PRESCALER_48MHZ  0x21
/* BEETLE_PLL_36Mhz */
#define BEETLE_PLL_FREQUENCY_36MHZ  36000000
#define BEETLE_PLL_PRESCALER_36MHZ  0x31
/* BEETLE_PLL_24Mhz */
#define BEETLE_PLL_FREQUENCY_24MHZ  24000000
#define BEETLE_PLL_PRESCALER_24MHZ  0x51
/* BEETLE_PLL_12Mhz */
#define BEETLE_PLL_FREQUENCY_12MHZ  12000000
#define BEETLE_PLL_PRESCALER_12MHZ  0xB1

#endif /* _ARM_BEETLE_SOC_PLL_H_ */
