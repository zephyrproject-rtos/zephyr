/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_pic32cm_sg_gc_clock.h
 * @brief List clock subsystem IDs for pic32cm_sg_gc family.
 *
 * Clock subsystem IDs. To be used in devicetree nodes, and as argument for clock API.
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CM_SG_GC_CLOCK_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CM_SG_GC_CLOCK_H_

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
/** @brief Maximum index for DPLL clock IDs. */
#define CLOCK_MCHP_DPLL_ID_MAX   (0)
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
/** @brief DPLL0 Output 4 clock ID. */
#define CLOCK_MCHP_DPLL0_ID_OUT4   MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 4)
/** @brief Maximum index for DPLL output clock IDs. */
#define CLOCK_MCHP_DPLL_OUT_ID_MAX (4)
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
/** @brief Maximum index for Generic Clock Generator IDs. */
#define CLOCK_MCHP_GCLKGEN_ID_MAX   (11)
/** @} */

/**
 * @name GCLKPERIPH_TYPE Clock IDs
 * @{
 */
/** @brief GCLK Peripheral ID: Frequency Meter Measure 0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_FREQM_MSR0   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 2, 0)
/** @brief GCLK Peripheral ID: Frequency Meter Measure 1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_FREQM_MSR1   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 3, 1)
/** @brief GCLK Peripheral ID: Frequency Meter Reference. */
#define CLOCK_MCHP_GCLKPERIPH_ID_FREQM_REF    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 4, 2)
/** @brief GCLK Peripheral ID: External Interrupt Controller (EIC). */
#define CLOCK_MCHP_GCLKPERIPH_ID_EIC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 5, 3)
/** @brief GCLK Peripheral ID: CAN0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 6, 4)
/** @brief GCLK Peripheral ID: CAN1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 7, 5)
/** @brief GCLK Peripheral ID: Event System Channel 0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH0    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 8, 6)
/** @brief GCLK Peripheral ID: Event System Channel 1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH1    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 9, 7)
/** @brief GCLK Peripheral ID: Event System Channel 2. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH2    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 10, 8)
/** @brief GCLK Peripheral ID: Event System Channel 3. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH3    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 11, 9)
/** @brief GCLK Peripheral ID: Event System Channel 4. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH4    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 12, 10)
/** @brief GCLK Peripheral ID: Event System Channel 5. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH5    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 13, 11)
/** @brief GCLK Peripheral ID: Event System Channel 6. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH6    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 14, 12)
/** @brief GCLK Peripheral ID: Event System Channel 7. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH7    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 15, 13)
/** @brief GCLK Peripheral ID: Event System Channel 8. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH8    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 16, 14)
/** @brief GCLK Peripheral ID: Event System Channel 9. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH9    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 17, 15)
/** @brief GCLK Peripheral ID: Event System Channel 10. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH10   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 16)
/** @brief GCLK Peripheral ID: Event System Channel 11. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH11   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 19, 17)
/** @brief GCLK Peripheral ID: SERCOM0 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM0_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 20, 18)
/** @brief GCLK Peripheral ID: SERCOM1 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM1_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 20, 19)
/** @brief GCLK Peripheral ID: SERCOM2 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM2_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 20, 20)
/** @brief GCLK Peripheral ID: SERCOM3 Slow clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM3_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 20, 21)
/** @brief GCLK Peripheral ID: SERCOM0 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM0_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 21, 22)
/** @brief GCLK Peripheral ID: SERCOM1 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM1_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 22, 23)
/** @brief GCLK Peripheral ID: SERCOM2 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM2_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 23, 24)
/** @brief GCLK Peripheral ID: SERCOM3 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM3_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 24, 25)
/** @brief GCLK Peripheral ID: TCC0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 25, 26)
/** @brief GCLK Peripheral ID: TCC1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 25, 27)
/** @brief GCLK Peripheral ID: TCC2. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC2         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 26, 28)
/** @brief GCLK Peripheral ID: TCC3. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC3         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 27, 29)
/** @brief GCLK Peripheral ID: SERCOM4 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM4_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 28, 30)
/** @brief GCLK Peripheral ID: SERCOM5 Core clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM5_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 29, 31)
/** @brief GCLK Peripheral ID: TCC4. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC4         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 30, 32)
/** @brief GCLK Peripheral ID: TCC5. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC5         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 31, 33)
/** @brief GCLK Peripheral ID: TCC6. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC6         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 32, 34)
/** @brief GCLK Peripheral ID: ADC. */
#define CLOCK_MCHP_GCLKPERIPH_ID_ADC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 33, 35)
/** @brief GCLK Peripheral ID: AC (Analog Comparator). */
#define CLOCK_MCHP_GCLKPERIPH_ID_AC           MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 34, 36)
/** @brief GCLK Peripheral ID: PTC (Peripheral Touch Controller). */
#define CLOCK_MCHP_GCLKPERIPH_ID_PTC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 35, 37)
/** @brief GCLK Peripheral ID: CCL0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CCL0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 36, 38)
/** @brief GCLK Peripheral ID: CCL1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CCL1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 37, 39)
/** @brief GCLK Peripheral ID: USB. */
#define CLOCK_MCHP_GCLKPERIPH_ID_USB          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 38, 40)
/** @brief Maximum index for GCLK Peripheral IDs. */
#define CLOCK_MCHP_GCLKPERIPH_ID_MAX          (40)
/** @} */

/**
 * @name MCLKDOMAIN_TYPE Clock IDs
 * @{
 */
/** @brief Main Clock Domain ID: CPU. */
#define CLOCK_MCHP_MCLKDOMAIN_ID_CPU MCHP_CLOCK_DERIVE_ID(8, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum index for Main Clock Domain IDs. */
#define CLOCK_MCHP_MCLKDOMAIN_MAX    (0)
/** @} */

/**
 * @name MCLKPERIPH_TYPE Clock IDs
 * @{
 */
/** @brief MCLK Peripheral ID: DSU (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DSU_AHB      MCHP_CLOCK_DERIVE_ID(9, 0, 0, 0x3f, 0)
/** @brief MCLK Peripheral ID: FCR (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCR_AHB      MCHP_CLOCK_DERIVE_ID(9, 0, 1, 0x3f, 1)
/** @brief MCLK Peripheral ID: FCW (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCW_AHB      MCHP_CLOCK_DERIVE_ID(9, 0, 2, 0x3f, 2)
/** @brief MCLK Peripheral ID: PM. */
#define CLOCK_MCHP_MCLKPERIPH_ID_PM           MCHP_CLOCK_DERIVE_ID(9, 0, 3, 0x3f, 3)
/** @brief MCLK Peripheral ID: SUPC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SUPC         MCHP_CLOCK_DERIVE_ID(9, 0, 4, 0x3f, 4)
/** @brief MCLK Peripheral ID: RSTC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_RSTC         MCHP_CLOCK_DERIVE_ID(9, 0, 5, 0x3f, 5)
/** @brief MCLK Peripheral ID: OSCCTRL. */
#define CLOCK_MCHP_MCLKPERIPH_ID_OSCCTRL      MCHP_CLOCK_DERIVE_ID(9, 0, 6, 0x3f, 6)
/** @brief MCLK Peripheral ID: OSC32KCTRL. */
#define CLOCK_MCHP_MCLKPERIPH_ID_OSC32KCTRL   MCHP_CLOCK_DERIVE_ID(9, 0, 7, 0x3f, 7)
/** @brief MCLK Peripheral ID: FREQM. */
#define CLOCK_MCHP_MCLKPERIPH_ID_FREQM        MCHP_CLOCK_DERIVE_ID(9, 0, 10, 0x3f, 8)
/** @brief MCLK Peripheral ID: WDT. */
#define CLOCK_MCHP_MCLKPERIPH_ID_WDT          MCHP_CLOCK_DERIVE_ID(9, 0, 11, 0x3f, 9)
/** @brief MCLK Peripheral ID: RTC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_RTC          MCHP_CLOCK_DERIVE_ID(9, 0, 12, 0x3f, 10)
/** @brief MCLK Peripheral ID: EIC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_EIC          MCHP_CLOCK_DERIVE_ID(9, 0, 13, 0x3f, 11)
/** @brief MCLK Peripheral ID: PAC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PAC_APB      MCHP_CLOCK_DERIVE_ID(9, 0, 14, 0x3f, 12)
/** @brief MCLK Peripheral ID: TRAM. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TRAM         MCHP_CLOCK_DERIVE_ID(9, 0, 15, 0x3f, 13)
/** @brief MCLK Peripheral ID: MCRAMC_APB. */
#define CLOCK_MCHP_MCLKPERIPH_ID_MCRAMC_APB   MCHP_CLOCK_DERIVE_ID(9, 0, 21, 0x3f, 14)
/** @brief MCLK Peripheral ID: MCRAMC_AHB. */
#define CLOCK_MCHP_MCLKPERIPH_ID_MCRAMC_AHB   MCHP_CLOCK_DERIVE_ID(9, 0, 22, 0x3f, 15)
/** @brief MCLK Peripheral ID: CAN0. */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN0         MCHP_CLOCK_DERIVE_ID(9, 0, 23, 0x3f, 16)
/** @brief MCLK Peripheral ID: CAN1. */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN1         MCHP_CLOCK_DERIVE_ID(9, 0, 24, 0x3f, 17)
/** @brief MCLK Peripheral ID: H2PB0 (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_H2PB0_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 25, 0x3f, 18)
/** @brief MCLK Peripheral ID: H2PB0 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_H2PB0_APB    MCHP_CLOCK_DERIVE_ID(9, 0, 26, 0x3f, 19)
/** @brief MCLK Peripheral ID: PORT (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PORT_APB     MCHP_CLOCK_DERIVE_ID(9, 0, 27, 0x3f, 20)
/** @brief MCLK Peripheral ID: DMAC (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMAC_AHB     MCHP_CLOCK_DERIVE_ID(9, 0, 28, 0x3f, 21)
/** @brief MCLK Peripheral ID: DMAC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMAC_APB     MCHP_CLOCK_DERIVE_ID(9, 0, 29, 0x3f, 22)
/** @brief MCLK Peripheral ID: HMATRIX2 (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_HMATRIX2_AHB MCHP_CLOCK_DERIVE_ID(9, 0, 30, 0x3f, 23)
/** @brief MCLK Peripheral ID: HMATRIX2 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_HMATRIX2_APB MCHP_CLOCK_DERIVE_ID(9, 0, 31, 0x3f, 24)

/** @brief MCLK Peripheral ID: BROMC (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_BROMC_AHB MCHP_CLOCK_DERIVE_ID(9, 1, 0, 0x3f, 25)
/** @brief MCLK Peripheral ID: BROMC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_BROMC_APB MCHP_CLOCK_DERIVE_ID(9, 1, 1, 0x3f, 26)
/** @brief MCLK Peripheral ID: IDAU. */
#define CLOCK_MCHP_MCLKPERIPH_ID_IDAU      MCHP_CLOCK_DERIVE_ID(9, 1, 2, 0x3f, 27)
/** @brief MCLK Peripheral ID: EVSYS. */
#define CLOCK_MCHP_MCLKPERIPH_ID_EVSYS     MCHP_CLOCK_DERIVE_ID(9, 1, 3, 0x3f, 28)
/** @brief MCLK Peripheral ID: SERCOM0. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM0   MCHP_CLOCK_DERIVE_ID(9, 1, 4, 0x3f, 29)
/** @brief MCLK Peripheral ID: SERCOM1. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM1   MCHP_CLOCK_DERIVE_ID(9, 1, 5, 0x3f, 30)
/** @brief MCLK Peripheral ID: SERCOM2. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM2   MCHP_CLOCK_DERIVE_ID(9, 1, 6, 0x3f, 31)
/** @brief MCLK Peripheral ID: SERCOM3. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM3   MCHP_CLOCK_DERIVE_ID(9, 1, 7, 0x3f, 32)
/** @brief MCLK Peripheral ID: TCC0. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC0      MCHP_CLOCK_DERIVE_ID(9, 1, 8, 0x3f, 33)
/** @brief MCLK Peripheral ID: TCC1. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC1      MCHP_CLOCK_DERIVE_ID(9, 1, 9, 0x3f, 34)
/** @brief MCLK Peripheral ID: TCC2. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC2      MCHP_CLOCK_DERIVE_ID(9, 1, 10, 0x3f, 35)
/** @brief MCLK Peripheral ID: TCC3. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC3      MCHP_CLOCK_DERIVE_ID(9, 1, 11, 0x3f, 36)
/** @brief MCLK Peripheral ID: SERCOM4. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM4   MCHP_CLOCK_DERIVE_ID(9, 1, 12, 0x3f, 37)
/** @brief MCLK Peripheral ID: SERCOM5. */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM5   MCHP_CLOCK_DERIVE_ID(9, 1, 13, 0x3f, 38)
/** @brief MCLK Peripheral ID: TCC4. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC4      MCHP_CLOCK_DERIVE_ID(9, 1, 14, 0x3f, 39)
/** @brief MCLK Peripheral ID: TCC5. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC5      MCHP_CLOCK_DERIVE_ID(9, 1, 15, 0x3f, 40)
/** @brief MCLK Peripheral ID: TCC6. */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC6      MCHP_CLOCK_DERIVE_ID(9, 1, 16, 0x3f, 41)
/** @brief MCLK Peripheral ID: ADC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_ADC       MCHP_CLOCK_DERIVE_ID(9, 1, 17, 0x3f, 42)
/** @brief MCLK Peripheral ID: AC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_AC        MCHP_CLOCK_DERIVE_ID(9, 1, 18, 0x3f, 43)
/** @brief MCLK Peripheral ID: PTC. */
#define CLOCK_MCHP_MCLKPERIPH_ID_PTC       MCHP_CLOCK_DERIVE_ID(9, 1, 19, 0x3f, 44)
/** @brief MCLK Peripheral ID: CCL0. */
#define CLOCK_MCHP_MCLKPERIPH_ID_CCL0      MCHP_CLOCK_DERIVE_ID(9, 1, 20, 0x3f, 45)
/** @brief MCLK Peripheral ID: CCL1. */
#define CLOCK_MCHP_MCLKPERIPH_ID_CCL1      MCHP_CLOCK_DERIVE_ID(9, 1, 21, 0x3f, 46)
/** @brief MCLK Peripheral ID: USB_AHB. */
#define CLOCK_MCHP_MCLKPERIPH_ID_USB_AHB   MCHP_CLOCK_DERIVE_ID(9, 1, 22, 0x3f, 47)
/** @brief MCLK Peripheral ID: USB_APB. */
#define CLOCK_MCHP_MCLKPERIPH_ID_USB_APB   MCHP_CLOCK_DERIVE_ID(9, 1, 23, 0x3f, 48)
/** @brief MCLK Peripheral ID: AT. */
#define CLOCK_MCHP_MCLKPERIPH_ID_AT        MCHP_CLOCK_DERIVE_ID(9, 1, 24, 0x3f, 49)
/** @brief MCLK Peripheral ID: H2PB1_AHB. */
#define CLOCK_MCHP_MCLKPERIPH_ID_H2PB1_AHB MCHP_CLOCK_DERIVE_ID(9, 1, 25, 0x3f, 50)
/** @brief MCLK Peripheral ID: H2PB1_APB. */
#define CLOCK_MCHP_MCLKPERIPH_ID_H2PB1_APB MCHP_CLOCK_DERIVE_ID(9, 1, 26, 0x3f, 51)
/** @brief MCLK Peripheral ID: H2PB2_AHB. */
#define CLOCK_MCHP_MCLKPERIPH_ID_H2PB2_AHB MCHP_CLOCK_DERIVE_ID(9, 1, 27, 0x3f, 52)
/** @brief MCLK Peripheral ID: H2PB2_APB. */
#define CLOCK_MCHP_MCLKPERIPH_ID_H2PB2_APB MCHP_CLOCK_DERIVE_ID(9, 1, 28, 0x3f, 53)
/** @brief MCLK Peripheral ID: HSM. */
#define CLOCK_MCHP_MCLKPERIPH_ID_HSM       MCHP_CLOCK_DERIVE_ID(9, 1, 30, 0x3f, 54)
/** @brief MCLK Peripheral ID: PUF. */
#define CLOCK_MCHP_MCLKPERIPH_ID_PUF       MCHP_CLOCK_DERIVE_ID(9, 1, 31, 0x3f, 55)

/** @brief Maximum index for MCLK Peripheral IDs. */
#define CLOCK_MCHP_MCLKPERIPH_ID_MAX (55)
/** @} */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CM_SG_GC_CLOCK_H_ */
