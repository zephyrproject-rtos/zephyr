/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_pic32cm_pl_clock.h
 * @brief List clock subsystem IDs for pic32cm_jh family.
 *
 * Clock subsystem IDs. To be used in devicetree nodes, and as argument for clock API.
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CM_PL_CLOCK_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CM_PL_CLOCK_H_

/* Clock subsystem Types */
/** @brief Subsystem type: On-chip oscillator high frequency (OSCHF). */
#define SUBSYS_TYPE_OSCHF      (0)
/** @brief Subsystem type: Real-Time Counter (RTC). */
#define SUBSYS_TYPE_RTC        (1)
/** @brief Subsystem type: External 32 kHz crystal oscillator (XOSC32K). */
#define SUBSYS_TYPE_XOSC32K    (2)
/** @brief Subsystem type: Internal 32 kHz oscillator (OSC32K). */
#define SUBSYS_TYPE_OSC32K     (3)
/** @brief Subsystem type: Generic Clock Generator (GCLK GEN). */
#define SUBSYS_TYPE_GCLKGEN    (4)
/** @brief Subsystem type: Generic Clock Peripheral Channel (GCLK PERIPH). */
#define SUBSYS_TYPE_GCLKPERIPH (5)
/** @brief Subsystem type: Main Clock - CPU domain (MCLK CPU). */
#define SUBSYS_TYPE_MCLKCPU    (6)
/** @brief Subsystem type: Main Clock - peripheral domain (MCLK PERIPH). */
#define SUBSYS_TYPE_MCLKPERIPH (7)
/**
 * @brief Maximum valid subsystem type value.
 *
 * This is the highest assigned subsystem type ID.
 */
#define SUBSYS_TYPE_MAX        (7)

/** @brief Main bus type: Advanced High-performance Bus (AHB). */
#define MBUS_AHB  (0)
/** @brief Main bus type: Advanced Peripheral Bus A (APBA). */
#define MBUS_APBA (1)
/** @brief Main bus type: Advanced Peripheral Bus B (APBB). */
#define MBUS_APBB (2)
/** @brief Main bus type: Advanced Peripheral Bus C (APBC). */
#define MBUS_APBC (3)
/** @brief Maximum value of Main Bus types */
#define MBUS_MAX  (3)

/** @brief XOSC32K instance: 1 kHz crystal oscillator (XOSC1K). */
#define INST_XOSC32K_XOSC1K  0
/** @brief XOSC32K instance: 32.768 kHz crystal oscillator (XOSC32K). */
#define INST_XOSC32K_XOSC32K 1

/** @brief OSC32K instance: 1 kHz internal oscillator (OSC1K). */
#define INST_OSC32K_OSC1K  0
/** @brief OSC32K instance: 32.768 kHz internal oscillator (OSC32K). */
#define INST_OSC32K_OSC32K 1

/**
 * @brief Encode clock type, MCLK bus, MCLK mask bit, GCLK PCH and instance number.
 *
 * Encodes clock subsystem selection fields into a single packed 32-bit identifier.
 *
 * Bit fields:
 * - Bits [7:0]   : @p inst (8 bits)
 * - Bits [13:8]  : @p gclkperiph (6 bits, valid 0..12)
 * - Bits [19:14] : @p mclkmaskbit (6 bits, valid 0..31)
 * - Bits [25:20] : @p mclkbus (6 bits), one of:
 *   - @c MBUS_AHB  (0)
 *   - @c MBUS_APBA (1)
 *   - @c MBUS_APBB (2)
 *   - @c MBUS_APBC (3)
 * - Bits [31:26] : @p type (6 bits), one of:
 *   - @c SUBSYS_TYPE_OSCHF      (0)
 *   - @c SUBSYS_TYPE_RTC        (1)
 *   - @c SUBSYS_TYPE_XOSC32K    (2)
 *   - @c SUBSYS_TYPE_OSC32K     (3)
 *   - @c SUBSYS_TYPE_GCLKGEN    (4)
 *   - @c SUBSYS_TYPE_GCLKPERIPH (5)
 *   - @c SUBSYS_TYPE_MCLKCPU    (6)
 *   - @c SUBSYS_TYPE_MCLKPERIPH (7)
 *
 * @param type        Clock subsystem type.
 * @param mclkbus     Select from the AHBx/APBx buses (see @c MBUS_*).
 * @param mclkmaskbit Module mask bit on the selected AHBx/APBx bus (0..31).
 * @param gclkperiph  GCLK peripheral channel number @c m in @c PCHCTRLm (0..12).
 * @param inst        Instance number of the specified clock type.
 *
 * @return Packed 32-bit clock subsystem identifier.
 */
#define MCHP_CLOCK_DERIVE_ID(type, mclkbus, mclkmaskbit, gclkperiph, inst)                         \
	(((type) << 26) | ((mclkbus) << 20) | ((mclkmaskbit) << 14) | ((gclkperiph) << 8) | inst)

/** @brief Clock ID for the OSCHF subsystem (no MCLK/GCLK dependency fields; set to 0x3f). */
#define CLOCK_MCHP_OSCHF_ID     MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_OSCHF, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum OSCHF clock instance ID (highest valid OSCHF instance index). */
#define CLOCK_MCHP_OSCHF_ID_MAX (0)

/** @brief Clock ID for the RTC subsystem (no MCLK/GCLK dependency fields; set to 0x3f). */
#define CLOCK_MCHP_RTC_ID     MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_RTC, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum RTC clock instance ID (highest valid RTC instance index). */
#define CLOCK_MCHP_RTC_ID_MAX (0)

/** @brief Clock ID for XOSC32K instance XOSC1K (no MCLK/GCLK dependency fields; set to 0x3f). */
#define CLOCK_MCHP_XOSC32K_ID_XOSC1K                                                               \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_XOSC32K, 0x3f, 0x3f, 0x3f, INST_XOSC32K_XOSC1K)
/** @brief Clock ID for XOSC32K instance XOSC32K (no MCLK/GCLK dependency fields; set to 0x3f). */
#define CLOCK_MCHP_XOSC32K_ID_XOSC32K                                                              \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_XOSC32K, 0x3f, 0x3f, 0x3f, INST_XOSC32K_XOSC32K)
/** @brief Maximum XOSC32K clock instance ID (highest valid XOSC32K instance index). */
#define CLOCK_MCHP_XOSC32K_ID_MAX (1)

/** @brief Clock ID for OSC32K instance OSC1K (no MCLK/GCLK dependency fields; set to 0x3f). */
#define CLOCK_MCHP_OSC32K_ID_OSC1K                                                                 \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_OSC32K, 0x3f, 0x3f, 0x3f, INST_OSC32K_OSC1K)
/** @brief Clock ID for OSC32K instance OSC32K (no MCLK/GCLK dependency fields; set to 0x3f). */
#define CLOCK_MCHP_OSC32K_ID_OSC32K                                                                \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_OSC32K, 0x3f, 0x3f, 0x3f, INST_OSC32K_OSC32K)
/** @brief Maximum OSC32K clock instance ID (highest valid OSC32K instance index). */
#define CLOCK_MCHP_OSC32K_ID_MAX (1)

/** @brief Clock ID for Generic Clock Generator 0 (GEN0). */
#define CLOCK_MCHP_GCLKGEN_ID_GEN0 MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKGEN, 0x3f, 0x3f, 0x3f, 0)
/** @brief Clock ID for Generic Clock Generator 1 (GEN1). */
#define CLOCK_MCHP_GCLKGEN_ID_GEN1 MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKGEN, 0x3f, 0x3f, 0x3f, 1)
/** @brief Clock ID for Generic Clock Generator 2 (GEN2). */
#define CLOCK_MCHP_GCLKGEN_ID_GEN2 MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKGEN, 0x3f, 0x3f, 0x3f, 2)
/** @brief Clock ID for Generic Clock Generator 3 (GEN3). */
#define CLOCK_MCHP_GCLKGEN_ID_GEN3 MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKGEN, 0x3f, 0x3f, 0x3f, 3)
/** @brief Maximum GCLK generator clock instance ID (highest valid generator index). */
#define CLOCK_MCHP_GCLKGEN_ID_MAX  (3)
/** @brief Maximum GCLK I/O generator index */
#define GCLK_IO_MAX                CLOCK_MCHP_GCLKGEN_ID_MAX

/** @brief GCLK peripheral clock ID for EIC (PCHCTRL0). */
#define CLOCK_MCHP_GCLKPERIPH_ID_EIC MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 0, 0)
/** @brief GCLK peripheral clock ID for EVSYS channel 0 (PCHCTRL1). */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CHANNEL_0                                                   \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 1, 1)
/** @brief GCLK peripheral clock ID for EVSYS channel 1 (PCHCTRL2). */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CHANNEL_1                                                   \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 2, 2)
/** @brief GCLK peripheral clock ID for EVSYS channel 2 (PCHCTRL3). */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CHANNEL_2                                                   \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 3, 3)
/** @brief GCLK peripheral clock ID for EVSYS channel 3 (PCHCTRL4). */
#define CLOCK_MCHP_GCLKPERIPH_ID_EVSYS_CHANNEL_3                                                   \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 4, 4)
/** @brief GCLK peripheral clock ID for SERCOM0 slow clock (PCHCTRL5). */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM0_SLOW                                                      \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 5, 5)
/** @brief GCLK peripheral clock ID for SERCOM0 core clock (PCHCTRL6). */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM0_CORE                                                      \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 6, 6)
/** @brief GCLK peripheral clock ID for SERCOM1 slow clock (PCHCTRL7). */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM1_SLOW                                                      \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 7, 7)
/** @brief GCLK peripheral clock ID for SERCOM1 core clock (PCHCTRL8). */
#define CLOCK_MCHP_GCLKPERIPH_ID_SERCOM1_CORE                                                      \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 8, 8)
/** @brief GCLK peripheral clock ID for TC0/TC1 (PCHCTRL9). */
#define CLOCK_MCHP_GCLKPERIPH_ID_TC0_TC1                                                           \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 9, 9)
/** @brief GCLK peripheral clock ID for TC2 (PCHCTRL10). */
#define CLOCK_MCHP_GCLKPERIPH_ID_TC2                                                               \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 10, 10)
/** @brief GCLK peripheral clock ID for TCC0 (PCHCTRL11). */
#define CLOCK_MCHP_GCLKPERIPH_ID_TCC0                                                              \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 11, 11)
/** @brief GCLK peripheral clock ID for CCL (PCHCTRL12). */
#define CLOCK_MCHP_GCLKPERIPH_ID_CCL                                                               \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_GCLKPERIPH, 0x3f, 0x3f, 12, 12)
/** @brief Maximum GCLK peripheral clock channel index (highest valid PCHCTRLm index). */
#define CLOCK_MCHP_GCLKPERIPH_ID_MAX (12)

/** @brief Maximum GCLK peripheral channel index */
#define GCLK_PH_MAX CLOCK_MCHP_GCLKPERIPH_ID_MAX

/** @brief Clock ID for the CPU MCLK domain (no bus/mask/GCLK fields; set to 0x3f). */
#define CLOCK_MCHP_MCLKCPU_ID  MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKCPU, 0x3f, 0x3f, 0x3f, 0)
/** @brief Maximum CPU MCLK clock instance ID (highest valid CPU-domain instance index). */
#define CLOCK_MCHP_MCLKCPU_MAX (0)

/* MCLKPERIPH_TYPE ids */
/** @brief MCLK peripheral clock ID for the AHB->APBA bridge (mask bit 0). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AHB_APBA                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_AHB, 0, 0x3f, 0)
/** @brief MCLK peripheral clock ID for the AHB->APBB bridge (mask bit 1). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AHB_APBB                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_AHB, 1, 0x3f, 1)
/** @brief MCLK peripheral clock ID for the AHB->APBC bridge (mask bit 2). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AHB_APBC                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_AHB, 2, 0x3f, 2)
/** @brief MCLK peripheral clock ID for DSU on AHB (mask bit 3). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AHB_DSU                                                           \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_AHB, 3, 0x3f, 3)
/** @brief MCLK peripheral clock ID for HMATRIXHS on AHB (mask bit 4). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AHB_HMATRIXHS                                                     \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_AHB, 4, 0x3f, 4)
/** @brief MCLK peripheral clock ID for NVMCTRL on AHB (mask bit 5). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AHB_NVMCTRL                                                       \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_AHB, 5, 0x3f, 5)
/** @brief MCLK peripheral clock ID for HSRAM on AHB (mask bit 6). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AHB_HSRAM                                                         \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_AHB, 6, 0x3f, 6)
/** @brief MCLK peripheral clock ID for DMAC on AHB (mask bit 7). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AHB_DMAC                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_AHB, 7, 0x3f, 7)
/** @brief MCLK peripheral clock ID for PAC on AHB (mask bit 8). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AHB_PAC                                                           \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_AHB, 8, 0x3f, 8)
/** @brief MCLK peripheral clock ID for BROM on AHB (mask bit 9). */
#define CLOCK_MCHP_MCLKPERIPH_ID_AHB_BROM                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_AHB, 9, 0x3f, 9)

/** MCLKPERIPH derive ID for PAC on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_PAC                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 0, 0x3f, 10)
/** MCLKPERIPH derive ID for PM on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_PM                                                           \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 1, 0x3f, 11)
/** MCLKPERIPH derive ID for MCLK on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_MCLK                                                         \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 2, 0x3f, 12)
/** MCLKPERIPH derive ID for RSTC on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_RSTC                                                         \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 3, 0x3f, 13)
/** MCLKPERIPH derive ID for OSCCTRL on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_OSCCTRL                                                      \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 4, 0x3f, 14)
/** MCLKPERIPH derive ID for OSC32KCTRL on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_OSC32KCTRL                                                   \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 5, 0x3f, 15)
/** MCLKPERIPH derive ID for SUPC on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_SUPC                                                         \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 6, 0x3f, 16)
/** MCLKPERIPH derive ID for GCLK on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_GCLK                                                         \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 7, 0x3f, 17)
/** MCLKPERIPH derive ID for WDT on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_WDT                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 8, 0x3f, 18)
/** MCLKPERIPH derive ID for RTC on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_RTC                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 9, 0x3f, 19)
/** MCLKPERIPH derive ID for EIC on APBA bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBA_EIC                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBA, 10, 0x3f, 20)

/** MCLKPERIPH derive ID for PORT on APBB bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBB_PORT                                                         \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBB, 0, 0x3f, 21)
/** MCLKPERIPH derive ID for DSU on APBB bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBB_DSU                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBB, 1, 0x3f, 22)
/** MCLKPERIPH derive ID for NVMCTRL on APBB bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBB_NVMCTRL                                                      \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBB, 2, 0x3f, 23)
/** MCLKPERIPH derive ID for DMAC on APBB bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBB_DMAC                                                         \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBB, 3, 0x3f, 24)
/** MCLKPERIPH derive ID for MTB on APBB bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBB_MTB                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBB, 4, 0x3f, 25)
/** MCLKPERIPH derive ID for HMATRIXHS on APBB bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBB_HMATRIXHS                                                    \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBB, 5, 0x3f, 26)

/** MCLKPERIPH derive ID for EVSYS on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_EVSYS                                                        \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 0, 0x3f, 27)
/** MCLKPERIPH derive ID for SERCOM0 on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_SERCOM0                                                      \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 1, 0x3f, 28)
/** MCLKPERIPH derive ID for SERCOM1 on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_SERCOM1                                                      \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 2, 0x3f, 29)
/** MCLKPERIPH derive ID for TC0 on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_TC0                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 3, 0x3f, 30)
/** MCLKPERIPH derive ID for TC1 on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_TC1                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 4, 0x3f, 31)
/** MCLKPERIPH derive ID for TC2 on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_TC2                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 5, 0x3f, 32)
/** MCLKPERIPH derive ID for TCC0 on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_TCC0                                                         \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 6, 0x3f, 33)
/** MCLKPERIPH derive ID for ADC0 on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_ADC0                                                         \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 7, 0x3f, 34)
/** MCLKPERIPH derive ID for AC on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_AC                                                           \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 8, 0x3f, 35)
/** MCLKPERIPH derive ID for CCL on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_CCL                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 9, 0x3f, 36)
/** MCLKPERIPH derive ID for PTC on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_PTC                                                          \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 10, 0x3f, 37)
/** MCLKPERIPH derive ID for SYSCTRL on APBC bus. */
#define CLOCK_MCHP_MCLKPERIPH_ID_APBC_SYSCTRL                                                      \
	MCHP_CLOCK_DERIVE_ID(SUBSYS_TYPE_MCLKPERIPH, MBUS_APBC, 11, 0x3f, 38)
/** Maximum valid MCLKPERIPH derive ID value. */
#define CLOCK_MCHP_MCLKPERIPH_ID_MAX (38)

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_MCHP_PIC32CM_PL_CLOCK_H_ */
