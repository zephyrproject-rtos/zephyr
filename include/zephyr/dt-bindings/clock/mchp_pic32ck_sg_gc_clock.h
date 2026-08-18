/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_pic32ck_sg_gc_clock.h
 * @brief List clock subsystem IDs for pic32ck_sg_gc family.
 *
 * Clock subsystem IDs. To be used in devicetree nodes, and as argument for clock API.
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CK_SG_GC_CLOCK_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CK_SG_GC_CLOCK_H_

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
 * SUBSYS_TYPE_PLL       (2)
 * SUBSYS_TYPE_PLL_OUT   (3)
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
 * @name PLL_TYPE Clock IDs
 * @{
 */
/** @brief PLL0 clock ID. */
#define CLOCK_MCHP_PLL_ID_PLL0 MCHP_CLOCK_DERIVE_ID(2, 0x3f, 0x3f, 0, 0)
/** @brief Maximum index for PLL clock IDs. */
#define CLOCK_MCHP_PLL_ID_MAX  (0)
/** @} */

/**
 * @name PLL_OUT_TYPE Clock IDs
 * @{
 */
/** @brief PLL0 Output 0 clock ID. */
#define CLOCK_MCHP_PLL0_ID_OUT0    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 0)
/** @brief PLL0 Output 1 clock ID. */
#define CLOCK_MCHP_PLL0_ID_OUT1    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 1)
/** @brief PLL0 Output 2 clock ID. */
#define CLOCK_MCHP_PLL0_ID_OUT2    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 2)
/** @brief PLL0 Output 3 clock ID. */
#define CLOCK_MCHP_PLL0_ID_OUT3    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 3)
/** @brief PLL0 Output 4 clock ID. */
#define CLOCK_MCHP_PLL0_ID_OUT4    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 4)
/** @brief PLL0 Output 5 clock ID. */
#define CLOCK_MCHP_PLL0_ID_OUT5    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 5)
/** @brief Maximum index for PLL0 Output clock IDs. */
#define CLOCK_MCHP_PLL0_OUT_ID_MAX (5)

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
/** @brief GCLK Peripheral ID: Event System Channel 0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH0    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 6, 4)
/** @brief GCLK Peripheral ID: Event System Channel 1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH1    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 7, 5)
/** @brief GCLK Peripheral ID: Event System Channel 2. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH2    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 8, 6)
/** @brief GCLK Peripheral ID: Event System Channel 3. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH3    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 9, 7)
/** @brief GCLK Peripheral ID: Event System Channel 4. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH4    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 10, 8)
/** @brief GCLK Peripheral ID: Event System Channel 5. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH5    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 11, 9)
/** @brief GCLK Peripheral ID: Event System Channel 6. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH6    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 12, 10)
/** @brief GCLK Peripheral ID: Event System Channel 7. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH7    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 13, 11)
/** @brief GCLK Peripheral ID: Event System Channel 8. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH8    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 14, 12)
/** @brief GCLK Peripheral ID: Event System Channel 9. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH9    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 15, 13)
/** @brief GCLK Peripheral ID: Event System Channel 10. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH10   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 16, 14)
/** @brief GCLK Peripheral ID: Event System Channel 11. */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH11   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 17, 15)
/** @brief GCLK Peripheral ID: SDMMC0 Slow Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDMMC0_SLOW  MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 16)
/** @brief GCLK Peripheral ID: SDMMC1 Slow Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDMMC1_SLOW  MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 17)
/** @brief GCLK Peripheral ID: SERCOM0 Slow Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM0_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 18)
/** @brief GCLK Peripheral ID: SERCOM1 Slow Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM1_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 19)
/** @brief GCLK Peripheral ID: SERCOM4 Slow Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM4_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 20)
/** @brief GCLK Peripheral ID: SERCOM2 Slow Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM2_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 21)
/** @brief GCLK Peripheral ID: SERCOM3 Slow Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM3_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 22)
/** @brief GCLK Peripheral ID: SERCOM5 Slow Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM5_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 23)
/** @brief GCLK Peripheral ID: SERCOM6 Slow Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM6_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 24)
/** @brief GCLK Peripheral ID: SERCOM7 Slow Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM7_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 25)
/** @brief GCLK Peripheral ID: SERCOM0 Core Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM0_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 19, 26)
/** @brief GCLK Peripheral ID: SERCOM1 Core Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM1_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 20, 27)
/** @brief GCLK Peripheral ID: SERCOM2 Core Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM2_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 21, 28)
/** @brief GCLK Peripheral ID: SERCOM3 Core Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM3_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 22, 29)
/** @brief GCLK Peripheral ID: TCC0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 23, 30)
/** @brief GCLK Peripheral ID: TCC1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 23, 31)
/** @brief GCLK Peripheral ID: TCC2. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC2         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 24, 32)
/** @brief GCLK Peripheral ID: TCC3. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC3         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 24, 33)
/** @brief GCLK Peripheral ID: SERCOM4 Core Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM4_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 25, 34)
/** @brief GCLK Peripheral ID: SERCOM5 Core Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM5_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 26, 35)
/** @brief GCLK Peripheral ID: SERCOM6 Core Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM6_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 27, 36)
/** @brief GCLK Peripheral ID: SERCOM7 Core Clock. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM7_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 28, 37)
/** @brief GCLK Peripheral ID: TCC4. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC4         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 29, 38)
/** @brief GCLK Peripheral ID: TCC5. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC5         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 30, 39)
/** @brief GCLK Peripheral ID: TCC6. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC6         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 31, 40)
/** @brief GCLK Peripheral ID: TCC7. */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC7         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 32, 41)
/** @brief GCLK Peripheral ID: ADC. */
#define CLOCK_MCHP_GCLKPERIPH_ID_ADC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 33, 42)
/** @brief GCLK Peripheral ID: AC. */
#define CLOCK_MCHP_GCLKPERIPH_ID_AC           MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 34, 43)
/** @brief GCLK Peripheral ID: PTC. */
#define CLOCK_MCHP_GCLKPERIPH_ID_PTC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 35, 44)
/** @brief GCLK Peripheral ID: SPI_IXS. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SPI_IXS      MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 36, 45)
/** @brief GCLK Peripheral ID: CCL. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CCL          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 37, 46)
/** @brief GCLK Peripheral ID: PDEC. */
#define CLOCK_MCHP_GCLKPERIPH_ID_PDEC         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 38, 47)
/** @brief GCLK Peripheral ID: CAN0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 39, 48)
/** @brief GCLK Peripheral ID: CAN1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 40, 49)
/** @brief GCLK Peripheral ID: ETH_TX. */
#define CLOCK_MCHP_GCLKPERIPH_ID_ETH_TX       MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 41, 50)
/** @brief GCLK Peripheral ID: ETH_TSU. */
#define CLOCK_MCHP_GCLKPERIPH_ID_ETH_TSU      MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 42, 51)
/** @brief GCLK Peripheral ID: SQI. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SQI          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 43, 52)
/** @brief GCLK Peripheral ID: SDMMC0. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDMMC0       MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 44, 53)
/** @brief GCLK Peripheral ID: SDMMC1. */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDMMC1       MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 45, 54)
/** @brief GCLK Peripheral ID: USB. */
#define CLOCK_MCHP_GCLKPERIPH_ID_USB          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 46, 55)
/** @brief GCLK Peripheral ID: CPU0_TRACE. */
#define CLOCK_MCHP_GCLKPERIPH_ID_CPU0_TRACE   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 47, 56)
/** @brief Maximum index for GCLK Peripheral IDs. */
#define CLOCK_MCHP_GCLKPERIPH_ID_MAX          (56)
/** @} */

/**
 * @name MCLKDOMAIN_TYPE Clock IDs
 * @{
 */
/** @brief Main Clock Domain ID: CPU. */
#define CLOCK_MCHP_MCLKCPU_ID  MCHP_CLOCK_DERIVE_ID(8, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum index for Main Clock Domain IDs. */
#define CLOCK_MCHP_MCLKCPU_MAX (0)
/** @} */

/**
 * @name MCLKPERIPH_TYPE Clock IDs
 * @{
 */
/** @brief MCLK Peripheral ID: DSU (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DSU_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 3, 0x3f, 0)
/** @brief MCLK Peripheral ID: FCR (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCR_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 4, 0x3f, 1)
/** @brief MCLK Peripheral ID: FCW (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCW_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 5, 0x3f, 2)
/** @brief MCLK Peripheral ID: PAC (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PAC_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 6, 0x3f, 3)
/** @brief MCLK Peripheral ID: DMA0 (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMA0_AHB   MCHP_CLOCK_DERIVE_ID(9, 0, 8, 0x3f, 4)
/** @brief MCLK Peripheral ID: DMA1 (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMA1_AHB   MCHP_CLOCK_DERIVE_ID(9, 0, 9, 0x3f, 5)
/** @brief MCLK Peripheral ID: PRM (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PRM_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 12, 0x3f, 6)
/** @brief MCLK Peripheral ID: CAN0 (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN0_AHB   MCHP_CLOCK_DERIVE_ID(9, 0, 13, 0x3f, 7)
/** @brief MCLK Peripheral ID: CAN1 (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN1_AHB   MCHP_CLOCK_DERIVE_ID(9, 0, 14, 0x3f, 8)
/** @brief MCLK Peripheral ID: ETH (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_ETH_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 15, 0x3f, 9)
/** @brief MCLK Peripheral ID: SQI (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SQI_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 16, 0x3f, 10)
/** @brief MCLK Peripheral ID: SDMMC0 (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SDMMC0_AHB MCHP_CLOCK_DERIVE_ID(9, 0, 17, 0x3f, 11)
/** @brief MCLK Peripheral ID: SDMMC1 (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SDMMC1_AHB MCHP_CLOCK_DERIVE_ID(9, 0, 18, 0x3f, 12)
/** @brief MCLK Peripheral ID: USBFS (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_USBFS_AHB  MCHP_CLOCK_DERIVE_ID(9, 0, 19, 0x3f, 13)
/** @brief MCLK Peripheral ID: USBHS (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_USBHS_AHB  MCHP_CLOCK_DERIVE_ID(9, 0, 20, 0x3f, 14)
/** @brief MCLK Peripheral ID: EBI (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_EBI_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 21, 0x3f, 15)
/** @brief MCLK Peripheral ID: HSM (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_HSM_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 22, 0x3f, 16)

/** @brief MCLK Peripheral ID: DSU (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DSU_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 0, 0x3f, 17)
/** @brief MCLK Peripheral ID: FCR (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCR_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 1, 0x3f, 18)
/** @brief MCLK Peripheral ID: FCW (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCW_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 2, 0x3f, 19)
/** @brief MCLK Peripheral ID: RSTC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_RSTC_APB       MCHP_CLOCK_DERIVE_ID(9, 1, 5, 0x3f, 20)
/** @brief MCLK Peripheral ID: OSCCTRL (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_OSCCTRL_APB    MCHP_CLOCK_DERIVE_ID(9, 1, 6, 0x3f, 21)
/** @brief MCLK Peripheral ID: OSC32KCTRL (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_OSC32KCTRL_APB MCHP_CLOCK_DERIVE_ID(9, 1, 7, 0x3f, 22)
/** @brief MCLK Peripheral ID: FREQM (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_FREQM_APB      MCHP_CLOCK_DERIVE_ID(9, 1, 10, 0x3f, 23)
/** @brief MCLK Peripheral ID: WDT (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_WDT_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 11, 0x3f, 24)
/** @brief MCLK Peripheral ID: RTC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_RTC_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 12, 0x3f, 25)
/** @brief MCLK Peripheral ID: EIC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_EIC_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 13, 0x3f, 26)
/** @brief MCLK Peripheral ID: PAC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PAC_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 14, 0x3f, 27)
/** @brief MCLK Peripheral ID: TRAM (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TRAM_APB       MCHP_CLOCK_DERIVE_ID(9, 1, 15, 0x3f, 28)
/** @brief MCLK Peripheral ID: MBISTINTF (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_MBISTINTF_APB  MCHP_CLOCK_DERIVE_ID(9, 1, 18, 0x3f, 29)
/** @brief MCLK Peripheral ID: TDM (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TDM_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 19, 0x3f, 30)

/** @brief MCLK Peripheral ID: PORT (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PORT_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 0, 0x3f, 31)
/** @brief MCLK Peripheral ID: DMA0 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMA0_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 1, 0x3f, 32)
/** @brief MCLK Peripheral ID: DMA1 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMA1_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 2, 0x3f, 33)
/** @brief MCLK Peripheral ID: PRM (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PRM_APB     MCHP_CLOCK_DERIVE_ID(9, 2, 4, 0x3f, 34)
/** @brief MCLK Peripheral ID: IDAU (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_IDAU_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 5, 0x3f, 35)
/** @brief MCLK Peripheral ID: EVSYS (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_EVSYS_APB   MCHP_CLOCK_DERIVE_ID(9, 2, 6, 0x3f, 36)
/** @brief MCLK Peripheral ID: SERCOM0 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM0_APB MCHP_CLOCK_DERIVE_ID(9, 2, 7, 0x3f, 37)
/** @brief MCLK Peripheral ID: SERCOM1 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM1_APB MCHP_CLOCK_DERIVE_ID(9, 2, 8, 0x3f, 38)
/** @brief MCLK Peripheral ID: SERCOM2 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM2_APB MCHP_CLOCK_DERIVE_ID(9, 2, 9, 0x3f, 39)
/** @brief MCLK Peripheral ID: SERCOM3 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM3_APB MCHP_CLOCK_DERIVE_ID(9, 2, 10, 0x3f, 40)
/** @brief MCLK Peripheral ID: TCC0 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC0_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 11, 0x3f, 41)
/** @brief MCLK Peripheral ID: TCC1 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC1_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 12, 0x3f, 42)
/** @brief MCLK Peripheral ID: TCC2 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC2_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 13, 0x3f, 43)
/** @brief MCLK Peripheral ID: TCC3 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC3_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 14, 0x3f, 44)

/** @brief MCLK Peripheral ID: SERCOM4 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM4_APB MCHP_CLOCK_DERIVE_ID(9, 3, 0, 0x3f, 45)
/** @brief MCLK Peripheral ID: SERCOM5 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM5_APB MCHP_CLOCK_DERIVE_ID(9, 3, 1, 0x3f, 46)
/** @brief MCLK Peripheral ID: SERCOM6 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM6_APB MCHP_CLOCK_DERIVE_ID(9, 3, 2, 0x3f, 47)
/** @brief MCLK Peripheral ID: SERCOM7 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM7_APB MCHP_CLOCK_DERIVE_ID(9, 3, 3, 0x3f, 48)
/** @brief MCLK Peripheral ID: TCC4 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC4_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 4, 0x3f, 49)
/** @brief MCLK Peripheral ID: TCC5 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC5_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 5, 0x3f, 50)
/** @brief MCLK Peripheral ID: TCC6 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC6_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 6, 0x3f, 51)
/** @brief MCLK Peripheral ID: TCC7 (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC7_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 7, 0x3f, 52)
/** @brief MCLK Peripheral ID: ADC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_ADC_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 8, 0x3f, 53)
/** @brief MCLK Peripheral ID: AC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AC_APB      MCHP_CLOCK_DERIVE_ID(9, 3, 9, 0x3f, 54)
/** @brief MCLK Peripheral ID: PTC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PTC_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 10, 0x3f, 55)
/** @brief MCLK Peripheral ID: I2S (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_I2S_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 11, 0x3f, 56)
/** @brief MCLK Peripheral ID: PCC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PCC_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 12, 0x3f, 57)
/** @brief MCLK Peripheral ID: CCL (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_CCL_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 13, 0x3f, 58)
/** @brief MCLK Peripheral ID: PDEC (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_PDEC_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 14, 0x3f, 59)
/** @brief MCLK Peripheral ID: ETH (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_ETH_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 15, 0x3f, 60)
/** @brief MCLK Peripheral ID: TRNG (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_TRNG_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 16, 0x3f, 61)
/** @brief MCLK Peripheral ID: USB (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_USB_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 17, 0x3f, 62)
/** @brief MCLK Peripheral ID: EBI (APB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_EBI_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 18, 0x3f, 63)
/** @brief MCLK Peripheral ID: BSDAP (AHB). */
#define CLOCK_MCHP_MCLKPERIPH_ID_BSDAP_AHB   MCHP_CLOCK_DERIVE_ID(9, 3, 19, 0x3f, 64)

/** @brief Maximum index for MCLK Peripheral IDs. */
#define CLOCK_MCHP_MCLKPERIPH_ID_MAX (64)
/** @} */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CK_SG_GC_CLOCK_H_ */
