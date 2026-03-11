/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_pic32cz_ca_clock.h
 * @brief List clock subsystem IDs for pic32cz_ca family.
 *
 * Clock subsystem IDs. To be used in devicetree nodes, and as argument for clock API.
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CZ_CA_CLOCK_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CZ_CA_CLOCK_H_

/**
 * @brief Derive a 32-bit clock subsystem identifier.
 *
 * Encodes the clock subsystem type, MCLK bus information,
 * GCLK peripheral channel, and instance number into a
 * single 32-bit identifier.
 *
 * Bit field layout:
 * - 00..07 (8 bits): inst
 *
 * - 08..13 (6 bits): gclkperiph
 * (values from 0 to 47)
 *
 * - 14..19 (6 bits): mclkmaskbit
 * (values from 0 to 31)
 *
 * - 20..25 (6 bits): mclkmaskreg
 * following values
 * MCLKMSK0 (0)
 * MCLKMSK1 (1)
 * MCLKMSK2 (2)
 *
 * - 26..31 (6 bits): type
 * following values
 * SUBSYS_TYPE_XOSC       (0)
 * SUBSYS_TYPE_DFLL48M    (1)
 * SUBSYS_TYPE_DPLL       (2)
 * SUBSYS_TYPE_DPLL_OUT   (3)
 * SUBSYS_TYPE_RTC        (4)
 * SUBSYS_TYPE_XOSC32K    (5)
 * SUBSYS_TYPE_GCLKGEN    (6)
 * SUBSYS_TYPE_GCLKPERIPH (7)
 * SUBSYS_TYPE_MCLKDOMAIN (8)
 * SUBSYS_TYPE_MCLKPERIPH (9)
 *
 * @param type clock subsystem type
 * @param mclkmaskreg select from the AHBx and the APBx buses
 * @param mclkmaskbit select the module connected to AHBx or APBx bus (0 to 31)
 * @param gclkperiph gclk peripheral channel number m in PCHTRLm (0 to 63)
 * @param inst instance number of the specified clock type
 *
 * @return Encoded clock subsystem identifier
 */
#define MCHP_CLOCK_DERIVE_ID(type, mclkmaskreg, mclkmaskbit, gclkperiph, inst)                     \
	(((type) << 26) | ((mclkmaskreg) << 20) | ((mclkmaskbit) << 14) | ((gclkperiph) << 8) |    \
	 inst)

/**
 * @name XOSC_TYPE Clock IDs
 * @{
 */
/** @brief External Oscillator (XOSC) clock ID. */
#define CLOCK_MCHP_XOSC_ID     MCHP_CLOCK_DERIVE_ID(0, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum index for XOSC clock IDs. */
#define CLOCK_MCHP_XOSC_ID_MAX (0)
/** @} */

/**
 * @name DFLL48M_TYPE Clock IDs
 * @{
 */
/** @brief DFLL48M (48 MHz Digital Frequency Locked Loop) clock ID. */
#define CLOCK_MCHP_DFLL48M_ID     MCHP_CLOCK_DERIVE_ID(1, 0x3f, 0x3f, 0, 0)
/** @brief Maximum index for DFLL48M clock IDs. */
#define CLOCK_MCHP_DFLL48M_ID_MAX (0)
/** @} */

/**
 * @name DPLL_TYPE Clock IDs
 * @{
 */
/** @brief DPLL0 clock ID. */
#define CLOCK_MCHP_DPLL_ID_DPLL0 MCHP_CLOCK_DERIVE_ID(2, 0x3f, 0x3f, 1, 0)
/** @brief DPLL1 clock ID. */
#define CLOCK_MCHP_DPLL_ID_DPLL1 MCHP_CLOCK_DERIVE_ID(2, 0x3f, 0x3f, 2, 1)
/** @brief Maximum index for DPLL clock IDs. */
#define CLOCK_MCHP_DPLL_ID_MAX   (1)
/** @} */

/**
 * @name DPLL_OUT_TYPE Clock IDs
 * @{
 */
/** @brief DPLL0 Output 0 clock ID. */
#define CLOCK_MCHP_DPLL0_ID_OUT0   MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 0)
/** @brief DPLL0 Output 1 clock ID. */
#define CLOCK_MCHP_DPLL0_ID_OUT1   MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 1)
/** @brief DPLL0 Output 2 clock ID. */
#define CLOCK_MCHP_DPLL0_ID_OUT2   MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 2)
/** @brief DPLL0 Output 3 clock ID. */
#define CLOCK_MCHP_DPLL0_ID_OUT3   MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 3)
/** @brief DPLL1 Output 0 clock ID. */
#define CLOCK_MCHP_DPLL1_ID_OUT0   MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 4)
/** @brief DPLL1 Output 1 clock ID. */
#define CLOCK_MCHP_DPLL1_ID_OUT1   MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 5)
/** @brief DPLL1 Output 2 clock ID. */
#define CLOCK_MCHP_DPLL1_ID_OUT2   MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 6)
/** @brief DPLL1 Output 3 clock ID. */
#define CLOCK_MCHP_DPLL1_ID_OUT3   MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 7)
/** @brief Maximum index for DPLL output clock IDs. */
#define CLOCK_MCHP_DPLL_OUT_ID_MAX (7)
/** @} */

/**
 * @name RTC_TYPE Clock IDs
 * @{
 */
/** @brief Real-Time Counter (RTC) clock ID. */
#define CLOCK_MCHP_RTC_ID     MCHP_CLOCK_DERIVE_ID(4, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum index for RTC clock IDs. */
#define CLOCK_MCHP_RTC_ID_MAX (0)
/** @} */

/**
 * @name XOSC32K_TYPE Clock IDs
 * @{
 */
/** @brief 32 kHz External Oscillator (XOSC32K) clock ID. */
#define CLOCK_MCHP_XOSC32K_ID     MCHP_CLOCK_DERIVE_ID(5, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum index for XOSC32K clock IDs. */
#define CLOCK_MCHP_XOSC32K_ID_MAX (0)
/** @} */

/**
 * @name GCLKGEN_TYPE Clock IDs
 * @{
 */
/** @brief Generic Clock Generator 0 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN0  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 0)
/** @brief Generic Clock Generator 1 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN1  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 1)
/** @brief Generic Clock Generator 2 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN2  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 2)
/** @brief Generic Clock Generator 3 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN3  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 3)
/** @brief Generic Clock Generator 4 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN4  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 4)
/** @brief Generic Clock Generator 5 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN5  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 5)
/** @brief Generic Clock Generator 6 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN6  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 6)
/** @brief Generic Clock Generator 7 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN7  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 7)
/** @brief Generic Clock Generator 8 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN8  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 8)
/** @brief Generic Clock Generator 9 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN9  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 9)
/** @brief Generic Clock Generator 10 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN10 MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 10)
/** @brief Generic Clock Generator 11 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN11 MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 11)
/** @brief Generic Clock Generator 12 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN12 MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 12)
/** @brief Generic Clock Generator 13 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN13 MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 13)
/** @brief Generic Clock Generator 14 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN14 MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 14)
/** @brief Generic Clock Generator 15 ID. */
#define CLOCK_MCHP_GCLKGEN_ID_GEN15 MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 15)
/** @brief Maximum index for Generic Clock Generator IDs. */
#define CLOCK_MCHP_GCLKGEN_ID_MAX   (15)
/** @} */

/**
 * @name GCLKPERIPH_TYPE Clock IDs
 * @{
 */
/** @brief GCLK Peripheral ID: Frequency Meter Measure. */
#define CLOCK_MCHP_GCLKPERIPH_ID_FREQM_MSR    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 3, 0)
/** @brief GCLK Peripheral ID: Frequency Meter Reference. */
#define CLOCK_MCHP_GCLKPERIPH_ID_FREQM_REF    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 4, 1)
/** @brief GCLK Peripheral ID: External Interrupt Controller (EIC). */
#define CLOCK_MCHP_GCLKPERIPH_ID_EIC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 5, 2)
/** @brief GCLK Peripheral ID: Event System Channel 0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH0    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 6, 3)
/** @brief GCLK Peripheral ID: Event System Channel 1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH1    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 7, 4)
/** @brief GCLK Peripheral ID: Event System Channel 2. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH2    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 8, 5)
/** @brief GCLK Peripheral ID: Event System Channel 3. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH3    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 9, 6)
/** @brief GCLK Peripheral ID: Event System Channel 4. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH4    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 10, 7)
/** @brief GCLK Peripheral ID: Event System Channel 5. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH5    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 11, 8)
/** @brief GCLK Peripheral ID: Event System Channel 6. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH6    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 12, 9)
/** @brief GCLK Peripheral ID: Event System Channel 7. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH7    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 13, 10)
/** @brief GCLK Peripheral ID: Event System Channel 8. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH8    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 14, 11)
/** @brief GCLK Peripheral ID: Event System Channel 9. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH9    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 15, 12)
/** @brief GCLK Peripheral ID: Event System Channel 10. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH10   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 16, 13)
/** @brief GCLK Peripheral ID: Event System Channel 11. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH11   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 17, 14)
/** @brief GCLK Peripheral ID: SERCOM0 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM0_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 15)
/** @brief GCLK Peripheral ID: SERCOM1 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM1_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 16)
/** @brief GCLK Peripheral ID: SERCOM4 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM4_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 17)
/** @brief GCLK Peripheral ID: SERCOM2 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM2_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 19, 18)
/** @brief GCLK Peripheral ID: SERCOM3 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM3_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 19, 19)
/** @brief GCLK Peripheral ID: SERCOM5 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM5_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 19, 20)
/** @brief GCLK Peripheral ID: SERCOM6 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM6_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 19, 21)
/** @brief GCLK Peripheral ID: SERCOM7 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM7_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 20, 22)
/** @brief GCLK Peripheral ID: SERCOM8 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM8_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 20, 23)
/** @brief GCLK Peripheral ID: SERCOM9 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM9_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 20, 24)
/** @brief GCLK Peripheral ID: SERCOM0 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM0_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 21, 25)
/** @brief GCLK Peripheral ID: SERCOM1 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM1_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 22, 26)
/** @brief GCLK Peripheral ID: SERCOM2 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM2_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 23, 27)
/** @brief GCLK Peripheral ID: SERCOM3 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM3_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 24, 28)
/** @brief GCLK Peripheral ID: SERCOM4 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM4_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 25, 29)
/** @brief GCLK Peripheral ID: SERCOM5 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM5_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 26, 30)
/** @brief GCLK Peripheral ID: SERCOM6 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM6_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 27, 31)
/** @brief GCLK Peripheral ID: SERCOM7 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM7_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 28, 32)
/** @brief GCLK Peripheral ID: SERCOM8 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM8_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 29, 33)
/** @brief GCLK Peripheral ID: SERCOM9 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM9_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 30, 34)
/** @brief GCLK Peripheral ID: TCC0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 31, 35)
/** @brief GCLK Peripheral ID: TCC1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 32, 36)
/** @brief GCLK Peripheral ID: TCC2. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC2         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 33, 37)
/** @brief GCLK Peripheral ID: TCC6. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC6         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 37, 38)
/** @brief GCLK Peripheral ID: TCC7. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC7         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 38, 39)
/** @brief GCLK Peripheral ID: TCC8. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC8         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 39, 40)
/** @brief GCLK Peripheral ID: TCC9. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC9         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 40, 41)
/** @brief GCLK Peripheral ID: ADC. */
#define CLOCK_MCHP_GCLKPERIPH_ID_ADC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 41, 42)
/** @brief GCLK Peripheral ID: AC (Analog Comparator). */
#define CLOCK_MCHP_GCLKPERIPH_ID_AC           MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 42, 43)
/** @brief GCLK Peripheral ID: PTC (Peripheral Touch Controller). */
#define CLOCK_MCHP_GCLKPERIPH_ID_PTC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 43, 44)
/** @brief GCLK Peripheral ID: I2S0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_I2S0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 44, 45)
/** @brief GCLK Peripheral ID: I2S1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_I2S1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 45, 46)
/** @brief GCLK Peripheral ID: CAN0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 46, 47)
/** @brief GCLK Peripheral ID: CAN1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 47, 48)
/** @brief GCLK Peripheral ID: CAN2. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN2         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 48, 49)
/** @brief GCLK Peripheral ID: CAN3. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN3         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 49, 50)
/** @brief GCLK Peripheral ID: CAN4. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN4         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 50, 51)
/** @brief GCLK Peripheral ID: CAN5. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN5         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 51, 52)
/** @brief GCLK Peripheral ID: GMAC TX. */
#define CLOCK_MCHP_GCLKPERIPH_ID_GMAC_TX      MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 54, 53)
/** @brief GCLK Peripheral ID: GMAC TSU. */
#define CLOCK_MCHP_GCLKPERIPH_ID_GMAC_TSU     MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 55, 54)
/** @brief GCLK Peripheral ID: SQI0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SQI0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 56, 55)
/** @brief GCLK Peripheral ID: SQI1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SQI1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 57, 56)
/** @brief GCLK Peripheral ID: SDHC0 Core. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDHC0_CORE   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 58, 57)
/** @brief GCLK Peripheral ID: SDHC0 Slow. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDHC0_SLOW   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 59, 58)
/** @brief GCLK Peripheral ID: SDHC1 Core. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDHC1_CORE   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 60, 59)
/** @brief GCLK Peripheral ID: SDHC1 Slow. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDHC1_SLOW   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 61, 60)
/** @brief GCLK Peripheral ID: MLB. */
#define CLOCK_MCHP_GCLKPERIPH_ID_MLB          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 62, 61)
/** @brief GCLK Peripheral ID: CM7 Trace. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CM7_TRACE    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 63, 62)
/** @brief Maximum index for GCLK Peripheral IDs. */
#define CLOCK_MCHP_GCLKPERIPH_ID_MAX          (62)
/** @} */

/**
 * @name MCLKDOMAIN_TYPE Clock IDs
 * @{
 */
/** @brief Main Clock Domain ID: CPU. */
#define CLOCK_MCHP_MCLKDOMAIN_ID_CPU    MCHP_CLOCK_DERIVE_ID(8, 0x3f, 0x3f, 0x3f, 0)
/** @brief Main Clock Domain ID: Peripheral. */
#define CLOCK_MCHP_MCLKDOMAIN_ID_PERIPH MCHP_CLOCK_DERIVE_ID(8, 0x3f, 0x3f, 0x3f, 1)
/** @brief Maximum index for Main Clock Domain IDs. */
#define CLOCK_MCHP_MCLKDOMAIN_MAX       (1)
/** @} */

/**
 * @name MCLKPERIPH_TYPE Clock IDs
 * (-todo confirm AHB/APB)
 * @{
 */
/** @brief MCLK Peripheral ID: DSU (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DSU_AHB      MCHP_CLOCK_DERIVE_ID(9, 0, 0, 0x3f, 0)
/** @brief MCLK Peripheral ID: DSU (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DSU_APB      MCHP_CLOCK_DERIVE_ID(9, 0, 1, 0x3f, 1)
/** @brief MCLK Peripheral ID: FCW (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCW_AHB      MCHP_CLOCK_DERIVE_ID(9, 0, 2, 0x3f, 2)
/** @brief MCLK Peripheral ID: FCW (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCW_APB      MCHP_CLOCK_DERIVE_ID(9, 0, 3, 0x3f, 3)
/** @brief MCLK Peripheral ID: FCR (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCR_AHB      MCHP_CLOCK_DERIVE_ID(9, 0, 4, 0x3f, 4)
/** @brief MCLK Peripheral ID: FCR (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCR_APB      MCHP_CLOCK_DERIVE_ID(9, 0, 5, 0x3f, 5)
/** @brief MCLK Peripheral ID: PM. */
#define CLOCK_MCHP_MCLKPERIPH_ID_PM           MCHP_CLOCK_DERIVE_ID(9, 0, 6, 0x3f, 6)
/** @brief MCLK Peripheral ID: SUPC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SUPC         MCHP_CLOCK_DERIVE_ID(9, 0, 7, 0x3f, 7)
/** @brief MCLK Peripheral ID: RSTC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_RSTC         MCHP_CLOCK_DERIVE_ID(9, 0, 8, 0x3f, 8)
/** @brief MCLK Peripheral ID: OSCCTRL. */
#define CLOCK_MCHP_MCLKPERIPH_ID_OSCCTRL      MCHP_CLOCK_DERIVE_ID(9, 0, 9, 0x3f, 9)
/** @brief MCLK Peripheral ID: OSC32KCTRL. */
#define CLOCK_MCHP_MCLKPERIPH_ID_OSC32KCTRL   MCHP_CLOCK_DERIVE_ID(9, 0, 10, 0x3f, 10)
/** @brief MCLK Peripheral ID: FREQM. */
#define CLOCK_MCHP_MCLKPERIPH_ID_FREQM        MCHP_CLOCK_DERIVE_ID(9, 0, 13, 0x3f, 11)
/** @brief MCLK Peripheral ID: WDT. */
#define CLOCK_MCHP_MCLKPERIPH_ID_WDT          MCHP_CLOCK_DERIVE_ID(9, 0, 14, 0x3f, 12)
/** @brief MCLK Peripheral ID: RTC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_RTC          MCHP_CLOCK_DERIVE_ID(9, 0, 15, 0x3f, 13)
/** @brief MCLK Peripheral ID: EIC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_EIC          MCHP_CLOCK_DERIVE_ID(9, 0, 16, 0x3f, 14)
/** @brief MCLK Peripheral ID: PAC (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PAC_AHB      MCHP_CLOCK_DERIVE_ID(9, 0, 17, 0x3f, 15)
/** @brief MCLK Peripheral ID: PAC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PAC_APB      MCHP_CLOCK_DERIVE_ID(9, 0, 18, 0x3f, 16)
/** @brief MCLK Peripheral ID: DRMTCM. */
#define CLOCK_MCHP_MCLKPERIPH_ID_DRMTCM       MCHP_CLOCK_DERIVE_ID(9, 0, 19, 0x3f, 17)
/** @brief MCLK Peripheral ID: MCRAMC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_MCRAMC       MCHP_CLOCK_DERIVE_ID(9, 0, 20, 0x3f, 18)
/** @brief MCLK Peripheral ID: TRAM. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TRAM         MCHP_CLOCK_DERIVE_ID(9, 0, 21, 0x3f, 19)
/** @brief MCLK Peripheral ID: PORT (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PORT_AHB     MCHP_CLOCK_DERIVE_ID(9, 0, 22, 0x3f, 20)
/** @brief MCLK Peripheral ID: PORT (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PORT_APB     MCHP_CLOCK_DERIVE_ID(9, 0, 23, 0x3f, 21)
/** @brief MCLK Peripheral ID: DMAC (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMAC_AHB     MCHP_CLOCK_DERIVE_ID(9, 0, 24, 0x3f, 22)
/** @brief MCLK Peripheral ID: DMAC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMAC_APB     MCHP_CLOCK_DERIVE_ID(9, 0, 25, 0x3f, 23)
/** @brief MCLK Peripheral ID: BUS (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_BUS_AHB      MCHP_CLOCK_DERIVE_ID(9, 0, 26, 0x3f, 24)
/** @brief MCLK Peripheral ID: BUS (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_BUS_APB      MCHP_CLOCK_DERIVE_ID(9, 0, 27, 0x3f, 25)
/** @brief MCLK Peripheral ID: BOOT ROM (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_BOOT_ROM_AHB MCHP_CLOCK_DERIVE_ID(9, 0, 28, 0x3f, 26)
/** @brief MCLK Peripheral ID: BOOT ROM (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_BOOT_ROM_APB MCHP_CLOCK_DERIVE_ID(9, 0, 29, 0x3f, 27)
/** @brief MCLK Peripheral ID: EVSYS. */
#define CLOCK_MCHP_MCLKPERIPH_ID_EVSYS        MCHP_CLOCK_DERIVE_ID(9, 0, 30, 0x3f, 28)
/** @brief MCLK Peripheral ID: SERCOM0. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM0      MCHP_CLOCK_DERIVE_ID(9, 0, 31, 0x3f, 29)

/** @brief MCLK Peripheral ID: SERCOM1. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM1 MCHP_CLOCK_DERIVE_ID(9, 1, 0, 0x3f, 30)
/** @brief MCLK Peripheral ID: SERCOM2. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM2 MCHP_CLOCK_DERIVE_ID(9, 1, 1, 0x3f, 31)
/** @brief MCLK Peripheral ID: SERCOM3. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM3 MCHP_CLOCK_DERIVE_ID(9, 1, 2, 0x3f, 32)
/** @brief MCLK Peripheral ID: SERCOM4. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM4 MCHP_CLOCK_DERIVE_ID(9, 1, 3, 0x3f, 33)
/** @brief MCLK Peripheral ID: SERCOM5. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM5 MCHP_CLOCK_DERIVE_ID(9, 1, 4, 0x3f, 34)
/** @brief MCLK Peripheral ID: SERCOM6. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM6 MCHP_CLOCK_DERIVE_ID(9, 1, 5, 0x3f, 35)
/** @brief MCLK Peripheral ID: SERCOM7. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM7 MCHP_CLOCK_DERIVE_ID(9, 1, 6, 0x3f, 36)
/** @brief MCLK Peripheral ID: SERCOM8. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM8 MCHP_CLOCK_DERIVE_ID(9, 1, 7, 0x3f, 37)
/** @brief MCLK Peripheral ID: SERCOM9. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM9 MCHP_CLOCK_DERIVE_ID(9, 1, 8, 0x3f, 38)
/** @brief MCLK Peripheral ID: TCC0. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC0    MCHP_CLOCK_DERIVE_ID(9, 1, 9, 0x3f, 39)
/** @brief MCLK Peripheral ID: TCC1. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC1    MCHP_CLOCK_DERIVE_ID(9, 1, 10, 0x3f, 40)
/** @brief MCLK Peripheral ID: TCC2. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC2    MCHP_CLOCK_DERIVE_ID(9, 1, 11, 0x3f, 41)
/** @brief MCLK Peripheral ID: TCC3. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC3    MCHP_CLOCK_DERIVE_ID(9, 1, 12, 0x3f, 42)
/** @brief MCLK Peripheral ID: TCC4. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC4    MCHP_CLOCK_DERIVE_ID(9, 1, 13, 0x3f, 43)
/** @brief MCLK Peripheral ID: TCC5. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC5    MCHP_CLOCK_DERIVE_ID(9, 1, 14, 0x3f, 44)
/** @brief MCLK Peripheral ID: TCC6. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC6    MCHP_CLOCK_DERIVE_ID(9, 1, 15, 0x3f, 45)
/** @brief MCLK Peripheral ID: TCC7. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC7    MCHP_CLOCK_DERIVE_ID(9, 1, 16, 0x3f, 46)
/** @brief MCLK Peripheral ID: TCC8. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC8    MCHP_CLOCK_DERIVE_ID(9, 1, 17, 0x3f, 47)
/** @brief MCLK Peripheral ID: TCC9. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC9    MCHP_CLOCK_DERIVE_ID(9, 1, 18, 0x3f, 48)
/** @brief MCLK Peripheral ID: ADC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_ADC     MCHP_CLOCK_DERIVE_ID(9, 1, 19, 0x3f, 49)
/** @brief MCLK Peripheral ID: AC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_AC      MCHP_CLOCK_DERIVE_ID(9, 1, 20, 0x3f, 50)
/** @brief MCLK Peripheral ID: PTC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_PTC     MCHP_CLOCK_DERIVE_ID(9, 1, 21, 0x3f, 51)
/** @brief MCLK Peripheral ID: I2S2. */
#define CLOCK_MCHP_MCLKPERIPH_ID_I2S2    MCHP_CLOCK_DERIVE_ID(9, 1, 22, 0x3f, 52)
/** @brief MCLK Peripheral ID: I2S1. */
#define CLOCK_MCHP_MCLKPERIPH_ID_I2S1    MCHP_CLOCK_DERIVE_ID(9, 1, 23, 0x3f, 53)
/** @brief MCLK Peripheral ID: CAN0. */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN0    MCHP_CLOCK_DERIVE_ID(9, 1, 24, 0x3f, 54)
/** @brief MCLK Peripheral ID: CAN1. */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN1    MCHP_CLOCK_DERIVE_ID(9, 1, 25, 0x3f, 55)
/** @brief MCLK Peripheral ID: CAN2. */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN2    MCHP_CLOCK_DERIVE_ID(9, 1, 26, 0x3f, 56)
/** @brief MCLK Peripheral ID: CAN3. */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN3    MCHP_CLOCK_DERIVE_ID(9, 1, 27, 0x3f, 57)
/** @brief MCLK Peripheral ID: CAN4. */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN4    MCHP_CLOCK_DERIVE_ID(9, 1, 28, 0x3f, 58)
/** @brief MCLK Peripheral ID: CAN5. */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN5    MCHP_CLOCK_DERIVE_ID(9, 1, 29, 0x3f, 59)

/** @brief MCLK Peripheral ID: GMAC (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_GMAC_AHB  MCHP_CLOCK_DERIVE_ID(9, 2, 0, 0x3f, 60)
/** @brief MCLK Peripheral ID: GMAC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_GMAC_APB  MCHP_CLOCK_DERIVE_ID(9, 2, 1, 0x3f, 61)
/** @brief MCLK Peripheral ID: SQI0. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SQI0      MCHP_CLOCK_DERIVE_ID(9, 2, 2, 0x3f, 62)
/** @brief MCLK Peripheral ID: SQI1. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SQI1      MCHP_CLOCK_DERIVE_ID(9, 2, 3, 0x3f, 63)
/** @brief MCLK Peripheral ID: TRNG. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TRNG      MCHP_CLOCK_DERIVE_ID(9, 2, 4, 0x3f, 64)
/** @brief MCLK Peripheral ID: SDHC0 (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SDHC0_AHB MCHP_CLOCK_DERIVE_ID(9, 2, 5, 0x3f, 65)
/** @brief MCLK Peripheral ID: SDHC0 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SDHC0_APB MCHP_CLOCK_DERIVE_ID(9, 2, 6, 0x3f, 66)
/** @brief MCLK Peripheral ID: SDHC1 (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SDHC1_AHB MCHP_CLOCK_DERIVE_ID(9, 2, 7, 0x3f, 67)
/** @brief MCLK Peripheral ID: SDHC1 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SDHC1_APB MCHP_CLOCK_DERIVE_ID(9, 2, 8, 0x3f, 68)
/** @brief MCLK Peripheral ID: HUSB0. */
#define CLOCK_MCHP_MCLKPERIPH_ID_HUSB0     MCHP_CLOCK_DERIVE_ID(9, 2, 9, 0x3f, 69)
/** @brief MCLK Peripheral ID: HUSB1. */
#define CLOCK_MCHP_MCLKPERIPH_ID_HUSB1     MCHP_CLOCK_DERIVE_ID(9, 2, 10, 0x3f, 70)
/** @brief MCLK Peripheral ID: EBI (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_EBI_AHB   MCHP_CLOCK_DERIVE_ID(9, 2, 11, 0x3f, 71)
/** @brief MCLK Peripheral ID: EBI (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_EBI_APB   MCHP_CLOCK_DERIVE_ID(9, 2, 12, 0x3f, 72)
/** @brief MCLK Peripheral ID: HSM. */
#define CLOCK_MCHP_MCLKPERIPH_ID_HSM       MCHP_CLOCK_DERIVE_ID(9, 2, 13, 0x3f, 73)
/** @brief MCLK Peripheral ID: MLB (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_MLB_AHB   MCHP_CLOCK_DERIVE_ID(9, 2, 14, 0x3f, 74)
/** @brief MCLK Peripheral ID: MLB (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_MLB_APB   MCHP_CLOCK_DERIVE_ID(9, 2, 15, 0x3f, 75)

/** @brief Maximum index for MCLK Peripheral IDs. */
#define CLOCK_MCHP_MCLKPERIPH_ID_MAX (75)
/** @} */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CZ_CA_CLOCK_H_ */
