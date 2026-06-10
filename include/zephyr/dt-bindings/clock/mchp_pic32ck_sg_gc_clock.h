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
 * Encode clock type, mclk bus, mclk mask bit, gclk pch and instance number,
 * to clock subsystem.
 *
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
 */
#define MCHP_CLOCK_DERIVE_ID(type, mclkmaskreg, mclkmaskbit, gclkperiph, inst)                     \
	(((type) << 26) | ((mclkmaskreg) << 20) | ((mclkmaskbit) << 14) | ((gclkperiph) << 8) |    \
	 inst)

/** @brief External oscillator clock subsystem ID */
#define CLOCK_MCHP_XOSC_ID     MCHP_CLOCK_DERIVE_ID(0, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum XOSC instance ID */
#define CLOCK_MCHP_XOSC_ID_MAX (0)

/** @brief DFLL48M clock subsystem ID */
#define CLOCK_MCHP_DFLL48M_ID     MCHP_CLOCK_DERIVE_ID(1, 0x3f, 0x3f, 0, 0)
/** @brief Maximum DFLL48M instance ID */
#define CLOCK_MCHP_DFLL48M_ID_MAX (0)

/** @brief PLL0 clock subsystem ID */
#define CLOCK_MCHP_PLL_ID_PLL0 MCHP_CLOCK_DERIVE_ID(2, 0x3f, 0x3f, 0, 0)
/** @brief Maximum PLL instance ID */
#define CLOCK_MCHP_PLL_ID_MAX  (0)

/** @brief PLL0 output 0 clock subsystem ID */
#define CLOCK_MCHP_PLL0_ID_OUT0    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 0)
/** @brief PLL0 output 1 clock subsystem ID */
#define CLOCK_MCHP_PLL0_ID_OUT1    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 1)
/** @brief PLL0 output 2 clock subsystem ID */
#define CLOCK_MCHP_PLL0_ID_OUT2    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 2)
/** @brief PLL0 output 3 clock subsystem ID */
#define CLOCK_MCHP_PLL0_ID_OUT3    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 3)
/** @brief PLL0 output 4 clock subsystem ID */
#define CLOCK_MCHP_PLL0_ID_OUT4    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 4)
/** @brief PLL0 output 5 clock subsystem ID */
#define CLOCK_MCHP_PLL0_ID_OUT5    MCHP_CLOCK_DERIVE_ID(3, 0x3f, 0x3f, 0x3f, 5)
/** @brief Maximum PLL0 output instance ID */
#define CLOCK_MCHP_PLL0_OUT_ID_MAX (5)

/** @brief RTC clock subsystem ID */
#define CLOCK_MCHP_RTC_ID     MCHP_CLOCK_DERIVE_ID(4, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum RTC instance ID */
#define CLOCK_MCHP_RTC_ID_MAX (0)

/** @brief 32kHz external oscillator clock subsystem ID */
#define CLOCK_MCHP_XOSC32K_ID     MCHP_CLOCK_DERIVE_ID(5, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum XOSC32K instance ID */
#define CLOCK_MCHP_XOSC32K_ID_MAX (0)

/** @brief Generic clock generator 0 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN0  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 0)
/** @brief Generic clock generator 1 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN1  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 1)
/** @brief Generic clock generator 2 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN2  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 2)
/** @brief Generic clock generator 3 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN3  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 3)
/** @brief Generic clock generator 4 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN4  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 4)
/** @brief Generic clock generator 5 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN5  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 5)
/** @brief Generic clock generator 6 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN6  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 6)
/** @brief Generic clock generator 7 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN7  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 7)
/** @brief Generic clock generator 8 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN8  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 8)
/** @brief Generic clock generator 9 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN9  MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 9)
/** @brief Generic clock generator 10 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN10 MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 10)
/** @brief Generic clock generator 11 subsystem ID */
#define CLOCK_MCHP_GCLKGEN_ID_GEN11 MCHP_CLOCK_DERIVE_ID(6, 0x3f, 0x3f, 0x3f, 11)
/** @brief Maximum GCLKGEN instance ID */
#define CLOCK_MCHP_GCLKGEN_ID_MAX   (11)

/** @brief FREQM 0 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_FREQM_MSR0   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 2, 0)
/** @brief FREQM 1 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_FREQM_MSR1   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 3, 1)
/** @brief FREQM reference peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_FREQM_REF    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 4, 2)
/** @brief EIC peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EIC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 5, 3)
/** @brief EVSYS channel 0 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH0    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 6, 4)
/** @brief EVSYS channel 1 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH1    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 7, 5)
/** @brief EVSYS channel 2 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH2    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 8, 6)
/** @brief EVSYS channel 3 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH3    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 9, 7)
/** @brief EVSYS channel 4 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH4    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 10, 8)
/** @brief EVSYS channel 5 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH5    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 11, 9)
/** @brief EVSYS channel 6 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH6    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 12, 10)
/** @brief EVSYS channel 7 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH7    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 13, 11)
/** @brief EVSYS channel 8 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH8    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 14, 12)
/** @brief EVSYS channel 9 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH9    MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 15, 13)
/** @brief EVSYS channel 10 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH10   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 16, 14)
/** @brief EVSYS channel 11 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CH11   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 17, 15)
/** @brief SDMMC0 slow peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDMMC0_SLOW  MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 16)
/** @brief SDMMC1 slow peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDMMC1_SLOW  MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 17)
/** @brief SERCOM0 slow peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM0_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 18)
/** @brief SERCOM1 slow peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM1_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 19)
/** @brief SERCOM4 slow peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM4_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 20)
/** @brief SERCOM2 slow peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM2_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 21)
/** @brief SERCOM3 slow peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM3_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 22)
/** @brief SERCOM5 slow peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM5_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 23)
/** @brief SERCOM6 slow peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM6_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 24)
/** @brief SERCOM7 slow peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM7_SLOW MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 18, 25)
/** @brief SERCOM0 core peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM0_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 19, 26)
/** @brief SERCOM1 core peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM1_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 20, 27)
/** @brief SERCOM2 core peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM2_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 21, 28)
/** @brief SERCOM3 core peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM3_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 22, 29)
/** @brief TCC0 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 23, 30)
/** @brief TCC1 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 23, 31)
/** @brief TCC2 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC2         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 24, 32)
/** @brief TCC3 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC3         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 24, 33)
/** @brief SERCOM4 core peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM4_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 25, 34)
/** @brief SERCOM5 core peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM5_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 26, 35)
/** @brief SERCOM6 core peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM6_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 27, 36)
/** @brief SERCOM7 core peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM7_CORE MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 28, 37)
/** @brief TCC4 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC4         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 29, 38)
/** @brief TCC5 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC5         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 30, 39)
/** @brief TCC6 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC6         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 31, 40)
/** @brief TCC7 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC7         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 32, 41)
/** @brief ADC peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_ADC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 33, 42)
/** @brief AC peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_AC           MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 34, 43)
/** @brief PTC peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_PTC          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 35, 44)
/** @brief SPI IXS peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SPI_IXS      MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 36, 45)
/** @brief CCL peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_CCL          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 37, 46)
/** @brief PDEC peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_PDEC         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 38, 47)
/** @brief CAN0 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN0         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 39, 48)
/** @brief CAN1 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_CAN1         MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 40, 49)
/** @brief Ethernet TX peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_ETH_TX       MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 41, 50)
/** @brief Ethernet TSU peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_ETH_TSU      MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 42, 51)
/** @brief SQI peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SQI          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 43, 52)
/** @brief SDMMC0 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDMMC0       MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 44, 53)
/** @brief SDMMC1 peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_SDMMC1       MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 45, 54)
/** @brief USB peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_USB          MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 46, 55)
/** @brief CPU0 trace peripheral clock ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_CPU0_TRACE   MCHP_CLOCK_DERIVE_ID(7, 0x3f, 0x3f, 47, 56)
/** @brief Maximum GCLKPERIPH instance ID */
#define CLOCK_MCHP_GCLKPERIPH_ID_MAX          (56)

/** @brief MCLK domain clock subsystem ID */
#define CLOCK_MCHP_MCLKCPU_ID  MCHP_CLOCK_DERIVE_ID(8, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum MCLK instance ID */
#define CLOCK_MCHP_MCLKCPU_MAX (0)

/** @brief DSU AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_DSU_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 3, 0x3f, 0)
/** @brief FCR AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCR_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 4, 0x3f, 1)
/** @brief FCW AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCW_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 5, 0x3f, 2)
/** @brief PAC AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_PAC_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 6, 0x3f, 3)
/** @brief DMA0 AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMA0_AHB   MCHP_CLOCK_DERIVE_ID(9, 0, 8, 0x3f, 4)
/** @brief DMA1 AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMA1_AHB   MCHP_CLOCK_DERIVE_ID(9, 0, 9, 0x3f, 5)
/** @brief PRM AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_PRM_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 12, 0x3f, 6)
/** @brief CAN0 AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN0_AHB   MCHP_CLOCK_DERIVE_ID(9, 0, 13, 0x3f, 7)
/** @brief CAN1 AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_CAN1_AHB   MCHP_CLOCK_DERIVE_ID(9, 0, 14, 0x3f, 8)
/** @brief Ethernet AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_ETH_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 15, 0x3f, 9)
/** @brief SQI AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SQI_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 16, 0x3f, 10)
/** @brief SDMMC0 AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SDMMC0_AHB MCHP_CLOCK_DERIVE_ID(9, 0, 17, 0x3f, 11)
/** @brief SDMMC1 AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SDMMC1_AHB MCHP_CLOCK_DERIVE_ID(9, 0, 18, 0x3f, 12)
/** @brief USB FS AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_USBFS_AHB  MCHP_CLOCK_DERIVE_ID(9, 0, 19, 0x3f, 13)
/** @brief USB HS AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_USBHS_AHB  MCHP_CLOCK_DERIVE_ID(9, 0, 20, 0x3f, 14)
/** @brief EBI AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_EBI_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 21, 0x3f, 15)
/** @brief HSM AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_HSM_AHB    MCHP_CLOCK_DERIVE_ID(9, 0, 22, 0x3f, 16)

/** @brief DSU APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_DSU_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 0, 0x3f, 17)
/** @brief FCR APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCR_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 1, 0x3f, 18)
/** @brief FCW APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_FCW_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 2, 0x3f, 19)
/** @brief RSTC APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_RSTC_APB       MCHP_CLOCK_DERIVE_ID(9, 1, 5, 0x3f, 20)
/** @brief OSCCTRL APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_OSCCTRL_APB    MCHP_CLOCK_DERIVE_ID(9, 1, 6, 0x3f, 21)
/** @brief OSC32KCTRL APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_OSC32KCTRL_APB MCHP_CLOCK_DERIVE_ID(9, 1, 7, 0x3f, 22)
/** @brief FREQM APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_FREQM_APB      MCHP_CLOCK_DERIVE_ID(9, 1, 10, 0x3f, 23)
/** @brief WDT APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_WDT_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 11, 0x3f, 24)
/** @brief RTC APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_RTC_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 12, 0x3f, 25)
/** @brief EIC APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_EIC_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 13, 0x3f, 26)
/** @brief PAC APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_PAC_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 14, 0x3f, 27)
/** @brief TRAM APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TRAM_APB       MCHP_CLOCK_DERIVE_ID(9, 1, 15, 0x3f, 28)
/** @brief MBIST interface APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_MBISTINTF_APB  MCHP_CLOCK_DERIVE_ID(9, 1, 18, 0x3f, 29)
/** @brief TDM APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TDM_APB        MCHP_CLOCK_DERIVE_ID(9, 1, 19, 0x3f, 30)

/** @brief PORT APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_PORT_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 0, 0x3f, 31)
/** @brief DMA0 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMA0_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 1, 0x3f, 32)
/** @brief DMA1 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_DMA1_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 2, 0x3f, 33)
/** @brief PRM APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_PRM_APB     MCHP_CLOCK_DERIVE_ID(9, 2, 4, 0x3f, 34)
/** @brief IDAU APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_IDAU_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 5, 0x3f, 35)
/** @brief EVSYS APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_EVSYS_APB   MCHP_CLOCK_DERIVE_ID(9, 2, 6, 0x3f, 36)
/** @brief SERCOM0 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM0_APB MCHP_CLOCK_DERIVE_ID(9, 2, 7, 0x3f, 37)
/** @brief SERCOM1 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM1_APB MCHP_CLOCK_DERIVE_ID(9, 2, 8, 0x3f, 38)
/** @brief SERCOM2 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM2_APB MCHP_CLOCK_DERIVE_ID(9, 2, 9, 0x3f, 39)
/** @brief SERCOM3 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM3_APB MCHP_CLOCK_DERIVE_ID(9, 2, 10, 0x3f, 40)
/** @brief TCC0 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC0_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 11, 0x3f, 41)
/** @brief TCC1 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC1_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 12, 0x3f, 42)
/** @brief TCC2 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC2_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 13, 0x3f, 43)
/** @brief TCC3 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC3_APB    MCHP_CLOCK_DERIVE_ID(9, 2, 14, 0x3f, 44)

/** @brief SERCOM4 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM4_APB MCHP_CLOCK_DERIVE_ID(9, 3, 0, 0x3f, 45)
/** @brief SERCOM5 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM5_APB MCHP_CLOCK_DERIVE_ID(9, 3, 1, 0x3f, 46)
/** @brief SERCOM6 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM6_APB MCHP_CLOCK_DERIVE_ID(9, 3, 2, 0x3f, 47)
/** @brief SERCOM7 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_SERCOM7_APB MCHP_CLOCK_DERIVE_ID(9, 3, 3, 0x3f, 48)
/** @brief TCC4 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC4_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 4, 0x3f, 49)
/** @brief TCC5 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC5_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 5, 0x3f, 50)
/** @brief TCC6 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC6_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 6, 0x3f, 51)
/** @brief TCC7 APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TCC7_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 7, 0x3f, 52)
/** @brief ADC APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_ADC_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 8, 0x3f, 53)
/** @brief AC APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_AC_APB      MCHP_CLOCK_DERIVE_ID(9, 3, 9, 0x3f, 54)
/** @brief PTC APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_PTC_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 10, 0x3f, 55)
/** @brief I2S APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_I2S_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 11, 0x3f, 56)
/** @brief PCC APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_PCC_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 12, 0x3f, 57)
/** @brief CCL APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_CCL_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 13, 0x3f, 58)
/** @brief PDEC APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_PDEC_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 14, 0x3f, 59)
/** @brief Ethernet APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_ETH_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 15, 0x3f, 60)
/** @brief TRNG APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_TRNG_APB    MCHP_CLOCK_DERIVE_ID(9, 3, 16, 0x3f, 61)
/** @brief USB APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_USB_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 17, 0x3f, 62)
/** @brief EBI APB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_EBI_APB     MCHP_CLOCK_DERIVE_ID(9, 3, 18, 0x3f, 63)
/** @brief BSDAP AHB bus clock ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_BSDAP_AHB   MCHP_CLOCK_DERIVE_ID(9, 3, 19, 0x3f, 64)
/** @brief Maximum MCLKPERIPH instance ID */
#define CLOCK_MCHP_MCLKPERIPH_ID_MAX         (64)

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CK_SG_GC_CLOCK_H_ */
