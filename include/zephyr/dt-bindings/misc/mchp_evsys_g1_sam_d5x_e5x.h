/*
 * Copyright (c) 2026 Muhammed Asif P
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Event generator and user identifiers for the Microchip SAM D5x/E5x EVSYS.
 * @ingroup mchp_evsys_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MISC_MCHP_EVSYS_G1_SAM_D5X_E5X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MISC_MCHP_EVSYS_G1_SAM_D5X_E5X_H_

/**
 * @def EVSYS_MCHP_EVUSER_MAX
 * @brief Maximum number of EVSYS user identifiers supported by this binding.
 */
#define EVSYS_MCHP_EVUSER_MAX 67

/**
 * @def EVSYS_MCHP_EVGEN_NONE
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_NONE 0x00

/**
 * @def EVSYS_MCHP_EVGEN_OSCCTRL_XOSC_FAIL0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_OSCCTRL_XOSC_FAIL0 0x01
/**
 * @def EVSYS_MCHP_EVGEN_OSCCTRL_XOSC_FAIL1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_OSCCTRL_XOSC_FAIL1 0x02

/**
 * @def EVSYS_MCHP_EVGEN_OSC32KCTRL_XOSC32K_FAIL
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_OSC32KCTRL_XOSC32K_FAIL 0x03

/* 0x04 – 0x0B : RTC Period x = 0..7 */
/**
 * @def EVSYS_MCHP_EVGEN_RTC_PER0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_PER0 0x04
/**
 * @def EVSYS_MCHP_EVGEN_RTC_PER1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_PER1 0x05
/**
 * @def EVSYS_MCHP_EVGEN_RTC_PER2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_PER2 0x06
/**
 * @def EVSYS_MCHP_EVGEN_RTC_PER3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_PER3 0x07
/**
 * @def EVSYS_MCHP_EVGEN_RTC_PER4
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_PER4 0x08
/**
 * @def EVSYS_MCHP_EVGEN_RTC_PER5
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_PER5 0x09
/**
 * @def EVSYS_MCHP_EVGEN_RTC_PER6
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_PER6 0x0A
/**
 * @def EVSYS_MCHP_EVGEN_RTC_PER7
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_PER7 0x0B

/* 0x0C – 0x0F : RTC Compare x = 0..3 */
/**
 * @def EVSYS_MCHP_EVGEN_RTC_CMP0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_CMP0 0x0C
/**
 * @def EVSYS_MCHP_EVGEN_RTC_CMP1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_CMP1 0x0D
/**
 * @def EVSYS_MCHP_EVGEN_RTC_CMP2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_CMP2 0x0E
/**
 * @def EVSYS_MCHP_EVGEN_RTC_CMP3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_CMP3 0x0F

/**
 * @def EVSYS_MCHP_EVGEN_RTC_TAMPER
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_TAMPER 0x10
/**
 * @def EVSYS_MCHP_EVGEN_RTC_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_RTC_OVF    0x11

/* 0x12 – 0x21 : EIC External Interrupt x = 0..15 */
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT0  0x12
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT1  0x13
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT2  0x14
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT3  0x15
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT4
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT4  0x16
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT5
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT5  0x17
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT6
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT6  0x18
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT7
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT7  0x19
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT8
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT8  0x1A
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT9
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT9  0x1B
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT10
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT10 0x1C
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT11
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT11 0x1D
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT12
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT12 0x1E
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT13
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT13 0x1F
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT14
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT14 0x20
/**
 * @def EVSYS_MCHP_EVGEN_EIC_EXTINT15
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_EIC_EXTINT15 0x21

/* 0x22 – 0x25 : DMA Channel x = 0..3 */
/**
 * @def EVSYS_MCHP_EVGEN_DMAC_CH0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_DMAC_CH0 0x22
/**
 * @def EVSYS_MCHP_EVGEN_DMAC_CH1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_DMAC_CH1 0x23
/**
 * @def EVSYS_MCHP_EVGEN_DMAC_CH2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_DMAC_CH2 0x24
/**
 * @def EVSYS_MCHP_EVGEN_DMAC_CH3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_DMAC_CH3 0x25

/* 0x26 */
/**
 * @def EVSYS_MCHP_EVGEN_PAC_ACCERR
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_PAC_ACCERR 0x26

/* 0x29 – 0x31 : TCC0 */
/**
 * @def EVSYS_MCHP_EVGEN_TCC0_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC0_OVF 0x29
/**
 * @def EVSYS_MCHP_EVGEN_TCC0_TRG
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC0_TRG 0x2A
/**
 * @def EVSYS_MCHP_EVGEN_TCC0_CNT
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC0_CNT 0x2B
/**
 * @def EVSYS_MCHP_EVGEN_TCC0_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC0_MC0 0x2C
/**
 * @def EVSYS_MCHP_EVGEN_TCC0_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC0_MC1 0x2D
/**
 * @def EVSYS_MCHP_EVGEN_TCC0_MC2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC0_MC2 0x2E
/**
 * @def EVSYS_MCHP_EVGEN_TCC0_MC3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC0_MC3 0x2F
/**
 * @def EVSYS_MCHP_EVGEN_TCC0_MC4
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC0_MC4 0x30
/**
 * @def EVSYS_MCHP_EVGEN_TCC0_MC5
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC0_MC5 0x31

/* 0x32 – 0x38 : TCC1 */
/**
 * @def EVSYS_MCHP_EVGEN_TCC1_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC1_OVF 0x32
/**
 * @def EVSYS_MCHP_EVGEN_TCC1_TRG
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC1_TRG 0x33
/**
 * @def EVSYS_MCHP_EVGEN_TCC1_CNT
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC1_CNT 0x34
/**
 * @def EVSYS_MCHP_EVGEN_TCC1_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC1_MC0 0x35
/**
 * @def EVSYS_MCHP_EVGEN_TCC1_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC1_MC1 0x36
/**
 * @def EVSYS_MCHP_EVGEN_TCC1_MC2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC1_MC2 0x37
/**
 * @def EVSYS_MCHP_EVGEN_TCC1_MC3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC1_MC3 0x38

/* 0x39 – 0x3E : TCC2 */
/**
 * @def EVSYS_MCHP_EVGEN_TCC2_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC2_OVF 0x39
/**
 * @def EVSYS_MCHP_EVGEN_TCC2_TRG
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC2_TRG 0x3A
/**
 * @def EVSYS_MCHP_EVGEN_TCC2_CNT
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC2_CNT 0x3B
/**
 * @def EVSYS_MCHP_EVGEN_TCC2_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC2_MC0 0x3C
/**
 * @def EVSYS_MCHP_EVGEN_TCC2_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC2_MC1 0x3D
/**
 * @def EVSYS_MCHP_EVGEN_TCC2_MC2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC2_MC2 0x3E

/* 0x3F – 0x43 : TCC3 */
/**
 * @def EVSYS_MCHP_EVGEN_TCC3_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC3_OVF 0x3F
/**
 * @def EVSYS_MCHP_EVGEN_TCC3_TRG
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC3_TRG 0x40
/**
 * @def EVSYS_MCHP_EVGEN_TCC3_CNT
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC3_CNT 0x41
/**
 * @def EVSYS_MCHP_EVGEN_TCC3_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC3_MC0 0x42
/**
 * @def EVSYS_MCHP_EVGEN_TCC3_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC3_MC1 0x43

/* 0x44 – 0x48 : TCC4 */
/**
 * @def EVSYS_MCHP_EVGEN_TCC4_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC4_OVF 0x44
/**
 * @def EVSYS_MCHP_EVGEN_TCC4_TRG
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC4_TRG 0x45
/**
 * @def EVSYS_MCHP_EVGEN_TCC4_CNT
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC4_CNT 0x46
/**
 * @def EVSYS_MCHP_EVGEN_TCC4_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC4_MC0 0x47
/**
 * @def EVSYS_MCHP_EVGEN_TCC4_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TCC4_MC1 0x48

/* 0x49 – 0x4B : TC0 */
/**
 * @def EVSYS_MCHP_EVGEN_TC0_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC0_OVF 0x49
/**
 * @def EVSYS_MCHP_EVGEN_TC0_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC0_MC0 0x4A
/**
 * @def EVSYS_MCHP_EVGEN_TC0_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC0_MC1 0x4B

/* 0x4C – 0x4E : TC1 */
/**
 * @def EVSYS_MCHP_EVGEN_TC1_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC1_OVF 0x4C
/**
 * @def EVSYS_MCHP_EVGEN_TC1_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC1_MC0 0x4D
/**
 * @def EVSYS_MCHP_EVGEN_TC1_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC1_MC1 0x4E

/* 0x4F – 0x51 : TC2 */
/**
 * @def EVSYS_MCHP_EVGEN_TC2_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC2_OVF 0x4F
/**
 * @def EVSYS_MCHP_EVGEN_TC2_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC2_MC0 0x50
/**
 * @def EVSYS_MCHP_EVGEN_TC2_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC2_MC1 0x51

/* 0x52 – 0x54 : TC3 */
/**
 * @def EVSYS_MCHP_EVGEN_TC3_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC3_OVF 0x52
/**
 * @def EVSYS_MCHP_EVGEN_TC3_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC3_MC0 0x53
/**
 * @def EVSYS_MCHP_EVGEN_TC3_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC3_MC1 0x54

/* 0x55 - 0x57: TC4 */
/**
 * @def EVSYS_MCHP_EVGEN_TC4_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC4_OVF 0x55
/**
 * @def EVSYS_MCHP_EVGEN_TC4_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC4_MC0 0x56
/**
 * @def EVSYS_MCHP_EVGEN_TC4_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC4_MC1 0x57

/* 0x58 – 0x5A : TC5 */
/**
 * @def EVSYS_MCHP_EVGEN_TC5_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC5_OVF 0x58
/**
 * @def EVSYS_MCHP_EVGEN_TC5_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC5_MC0 0x59
/**
 * @def EVSYS_MCHP_EVGEN_TC5_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC5_MC1 0x5A

/* 0x5B – 0x5D : TC6 */
/**
 * @def EVSYS_MCHP_EVGEN_TC6_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC6_OVF 0x5B
/**
 * @def EVSYS_MCHP_EVGEN_TC6_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC6_MC0 0x5C
/**
 * @def EVSYS_MCHP_EVGEN_TC6_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC6_MC1 0x5D

/* 0x5E – 0x60 : TC7 */
/**
 * @def EVSYS_MCHP_EVGEN_TC7_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC7_OVF 0x5E
/**
 * @def EVSYS_MCHP_EVGEN_TC7_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC7_MC0 0x5F
/**
 * @def EVSYS_MCHP_EVGEN_TC7_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TC7_MC1 0x60

/* 0x61 – 0x66 : PDEC */
/**
 * @def EVSYS_MCHP_EVGEN_PDEC_OVF
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_PDEC_OVF 0x61
/**
 * @def EVSYS_MCHP_EVGEN_PDEC_ERR
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_PDEC_ERR 0x62
/**
 * @def EVSYS_MCHP_EVGEN_PDEC_DIR
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_PDEC_DIR 0x63
/**
 * @def EVSYS_MCHP_EVGEN_PDEC_VLC
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_PDEC_VLC 0x64
/**
 * @def EVSYS_MCHP_EVGEN_PDEC_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_PDEC_MC0 0x65
/**
 * @def EVSYS_MCHP_EVGEN_PDEC_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_PDEC_MC1 0x66

/* 0x67 – 0x68 : ADC0 */
/**
 * @def EVSYS_MCHP_EVGEN_ADC0_RESRDY
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_ADC0_RESRDY 0x67
/**
 * @def EVSYS_MCHP_EVGEN_ADC0_WINMON
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_ADC0_WINMON 0x68

/* 0x69 – 0x6A : ADC1 */
/**
 * @def EVSYS_MCHP_EVGEN_ADC1_RESRDY
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_ADC1_RESRDY 0x69
/**
 * @def EVSYS_MCHP_EVGEN_ADC1_WINMON
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_ADC1_WINMON 0x6A

/* 0x6B – 0x6C : AC Comparator x = 0..1 */
/**
 * @def EVSYS_MCHP_EVGEN_AC_COMP0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_AC_COMP0 0x6B
/**
 * @def EVSYS_MCHP_EVGEN_AC_COMP1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_AC_COMP1 0x6C

/* 0x6D : AC Window */
/**
 * @def EVSYS_MCHP_EVGEN_AC_WIN
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_AC_WIN 0x6D

/* 0x6E – 0x6F : DAC EMPTY x = 0..1 */
/**
 * @def EVSYS_MCHP_EVGEN_DAC_EMPTY0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_DAC_EMPTY0 0x6E
/**
 * @def EVSYS_MCHP_EVGEN_DAC_EMPTY1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_DAC_EMPTY1 0x6F

/* 0x70 – 0x71 : DAC RESRDY x = 0..1 */
/**
 * @def EVSYS_MCHP_EVGEN_DAC_RESRDY0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_DAC_RESRDY0 0x70
/**
 * @def EVSYS_MCHP_EVGEN_DAC_RESRDY1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_DAC_RESRDY1 0x71

/* 0x72 : GMAC */
/**
 * @def EVSYS_MCHP_EVGEN_GMAC_TSU_CMP
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_GMAC_TSU_CMP 0x72

/* 0x73 : TRNG */
/**
 * @def EVSYS_MCHP_EVGEN_TRNG_READY
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_TRNG_READY 0x73

/* 0x74 – 0x77 : CCL LUTOUT x = 0..3 */
/**
 * @def EVSYS_MCHP_EVGEN_CCL_LUTOUT0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_CCL_LUTOUT0 0x74
/**
 * @def EVSYS_MCHP_EVGEN_CCL_LUTOUT1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_CCL_LUTOUT1 0x75
/**
 * @def EVSYS_MCHP_EVGEN_CCL_LUTOUT2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_CCL_LUTOUT2 0x76
/**
 * @def EVSYS_MCHP_EVGEN_CCL_LUTOUT3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVGEN_CCL_LUTOUT3 0x77

/* The m is the register offset number in the user register, it is not the value which is to be
 * written it is the actual register offset. Check the evsys.h header file to find its offset
 * written. These are the register offsets of each of the channel user registers.
 */

/* USER 0 : RTC Tamper */
/**
 * @def EVSYS_MCHP_EVUSER_RTC_TAMPER
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_RTC_TAMPER 0x120

/* USER 1..4 : PORT Event 0..3 */
/**
 * @def EVSYS_MCHP_EVUSER_PORT_EV0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_PORT_EV0 0x124
/**
 * @def EVSYS_MCHP_EVUSER_PORT_EV1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_PORT_EV1 0x128
/**
 * @def EVSYS_MCHP_EVUSER_PORT_EV2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_PORT_EV2 0x12C
/**
 * @def EVSYS_MCHP_EVUSER_PORT_EV3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_PORT_EV3 0x130

/* USER 5..12 : DMAC Channel 0..7 */
/**
 * @def EVSYS_MCHP_EVUSER_DMAC_CH0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_DMAC_CH0 0x134
/**
 * @def EVSYS_MCHP_EVUSER_DMAC_CH1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_DMAC_CH1 0x138
/**
 * @def EVSYS_MCHP_EVUSER_DMAC_CH2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_DMAC_CH2 0x13C
/**
 * @def EVSYS_MCHP_EVUSER_DMAC_CH3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_DMAC_CH3 0x140
/**
 * @def EVSYS_MCHP_EVUSER_DMAC_CH4
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_DMAC_CH4 0x144
/**
 * @def EVSYS_MCHP_EVUSER_DMAC_CH5
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_DMAC_CH5 0x148
/**
 * @def EVSYS_MCHP_EVUSER_DMAC_CH6
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_DMAC_CH6 0x14C
/**
 * @def EVSYS_MCHP_EVUSER_DMAC_CH7
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_DMAC_CH7 0x150

/* USER 13 : Reserved (skipped) */

/* USER 14..16 : CM4 Trace */
/**
 * @def EVSYS_MCHP_EVUSER_CM4_TRACE_START
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_CM4_TRACE_START 0x158
/**
 * @def EVSYS_MCHP_EVUSER_CM4_TRACE_STOP
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_CM4_TRACE_STOP  0x15C
/**
 * @def EVSYS_MCHP_EVUSER_CM4_TRACE_TRIG
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_CM4_TRACE_TRIG  0x160

/* USER 17..24 : TCC0 */
/**
 * @def EVSYS_MCHP_EVUSER_TCC0_EV0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC0_EV0 0x164
/**
 * @def EVSYS_MCHP_EVUSER_TCC0_EV1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC0_EV1 0x168
/**
 * @def EVSYS_MCHP_EVUSER_TCC0_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC0_MC0 0x16C
/**
 * @def EVSYS_MCHP_EVUSER_TCC0_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC0_MC1 0x170
/**
 * @def EVSYS_MCHP_EVUSER_TCC0_MC2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC0_MC2 0x174
/**
 * @def EVSYS_MCHP_EVUSER_TCC0_MC3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC0_MC3 0x178
/**
 * @def EVSYS_MCHP_EVUSER_TCC0_MC4
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC0_MC4 0x17C
/**
 * @def EVSYS_MCHP_EVUSER_TCC0_MC5
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC0_MC5 0x180

/* USER 25..30 : TCC1 */
/**
 * @def EVSYS_MCHP_EVUSER_TCC1_EV0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC1_EV0 0x184
/**
 * @def EVSYS_MCHP_EVUSER_TCC1_EV1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC1_EV1 0x188
/**
 * @def EVSYS_MCHP_EVUSER_TCC1_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC1_MC0 0x18C
/**
 * @def EVSYS_MCHP_EVUSER_TCC1_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC1_MC1 0x190
/**
 * @def EVSYS_MCHP_EVUSER_TCC1_MC2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC1_MC2 0x194
/**
 * @def EVSYS_MCHP_EVUSER_TCC1_MC3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC1_MC3 0x198

/* USER 31..35 : TCC2 */
/**
 * @def EVSYS_MCHP_EVUSER_TCC2_EV0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC2_EV0 0x19C
/**
 * @def EVSYS_MCHP_EVUSER_TCC2_EV1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC2_EV1 0x1A0
/**
 * @def EVSYS_MCHP_EVUSER_TCC2_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC2_MC0 0x1A4
/**
 * @def EVSYS_MCHP_EVUSER_TCC2_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC2_MC1 0x1A8
/**
 * @def EVSYS_MCHP_EVUSER_TCC2_MC2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC2_MC2 0x1AC

/* USER 36..39 : TCC3 */
/**
 * @def EVSYS_MCHP_EVUSER_TCC3_EV0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC3_EV0 0x1B0
/**
 * @def EVSYS_MCHP_EVUSER_TCC3_EV1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC3_EV1 0x1B4
/**
 * @def EVSYS_MCHP_EVUSER_TCC3_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC3_MC0 0x1B8
/**
 * @def EVSYS_MCHP_EVUSER_TCC3_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC3_MC1 0x1BC

/* USER 40..43 : TCC4 */
/**
 * @def EVSYS_MCHP_EVUSER_TCC4_EV0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC4_EV0 0x1C0
/**
 * @def EVSYS_MCHP_EVUSER_TCC4_EV1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC4_EV1 0x1C4
/**
 * @def EVSYS_MCHP_EVUSER_TCC4_MC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC4_MC0 0x1C8
/**
 * @def EVSYS_MCHP_EVUSER_TCC4_MC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TCC4_MC1 0x1CC

/* USER 44..51 : TC0..7 EVU */
/**
 * @def EVSYS_MCHP_EVUSER_TC0_EVU
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TC0_EVU 0x1D0
/**
 * @def EVSYS_MCHP_EVUSER_TC1_EVU
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TC1_EVU 0x1D4
/**
 * @def EVSYS_MCHP_EVUSER_TC2_EVU
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TC2_EVU 0x1D8
/**
 * @def EVSYS_MCHP_EVUSER_TC3_EVU
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TC3_EVU 0x1DC
/**
 * @def EVSYS_MCHP_EVUSER_TC4_EVU
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TC4_EVU 0x1E0
/**
 * @def EVSYS_MCHP_EVUSER_TC5_EVU
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TC5_EVU 0x1E4
/**
 * @def EVSYS_MCHP_EVUSER_TC6_EVU
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TC6_EVU 0x1E8
/**
 * @def EVSYS_MCHP_EVUSER_TC7_EVU
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_TC7_EVU 0x1EC

/* USER 52..54 : PDEC EVU */
/**
 * @def EVSYS_MCHP_EVUSER_PDEC_EVU0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_PDEC_EVU0 0x1F0
/**
 * @def EVSYS_MCHP_EVUSER_PDEC_EVU1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_PDEC_EVU1 0x1F4
/**
 * @def EVSYS_MCHP_EVUSER_PDEC_EVU2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_PDEC_EVU2 0x1F8

/* USER 55..56 : ADC0 */
/**
 * @def EVSYS_MCHP_EVUSER_ADC0_START
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_ADC0_START 0x1FC
/**
 * @def EVSYS_MCHP_EVUSER_ADC0_SYNC
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_ADC0_SYNC  0x200
/* USER 57..58 : ADC1 */
/**
 * @def EVSYS_MCHP_EVUSER_ADC1_START
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_ADC1_START 0x204
/**
 * @def EVSYS_MCHP_EVUSER_ADC1_SYNC
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_ADC1_SYNC  0x208

/* USER 59..60 : AC SOC x = 0..1 */
/**
 * @def EVSYS_MCHP_EVUSER_AC_SOC0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_AC_SOC0 0x20C
/**
 * @def EVSYS_MCHP_EVUSER_AC_SOC1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_AC_SOC1 0x210

/* USER 61..62 : DAC START 0..1 */
/**
 * @def EVSYS_MCHP_EVUSER_DAC_START0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_DAC_START0 0x214
/**
 * @def EVSYS_MCHP_EVUSER_DAC_START1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_DAC_START1 0x218

/* USER 63..66 : CCL LUTIN 0..3 */
/**
 * @def EVSYS_MCHP_EVUSER_CCL_LUTIN0
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_CCL_LUTIN0 0x21C
/**
 * @def EVSYS_MCHP_EVUSER_CCL_LUTIN1
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_CCL_LUTIN1 0x220
/**
 * @def EVSYS_MCHP_EVUSER_CCL_LUTIN2
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_CCL_LUTIN2 0x224
/**
 * @def EVSYS_MCHP_EVUSER_CCL_LUTIN3
 * @brief Microchip SAM D5x/E5x EVSYS identifier.
 */
#define EVSYS_MCHP_EVUSER_CCL_LUTIN3 0x228

/* Remaining USER indices reserved */
#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MISC_MCHP_EVSYS_G1_SAM_D5X_E5X_H_ */
