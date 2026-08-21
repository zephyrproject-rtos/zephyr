/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_rstc_g1.h
 * @brief Microchip RSTC G1 reset controller header
 *
 * This header includes the  Microchip RSTC G1 macro definitions.
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_RESET_MCHP_RSTC_G1_H_
#define INCLUDE_ZEPHYR_DRIVERS_RESET_MCHP_RSTC_G1_H_

/**
 * @enum rstc_g1_rcause
 * @brief Reset cause flags for Microchip RSTC G1.
 *
 * This enumeration defines the possible reset causes as indicated by the
 * RSTC_RCAUSE register in the Microchip RSTC G1 reset controller.
 */
#ifdef CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_SG_GC
/*
 * PIC32CM SG/GC assigns RCAUSE differently and makes the register 16 bits
 * wide. Four brown-out detectors sit between POR and EXT (PORCORE,
 * BORVDDREG, BORVDDA, BORVDDIO), which pushes EXT, WDT and SYST up by one
 * and puts BACKUP out of reach of an 8-bit read. Reading this part with the
 * assignment below reports an external reset as a watchdog reset.
 */
enum rstc_g1_rcause {
	RSTC_G1_RCAUSE_POR = 0,       /* Power-on Reset */
	RSTC_G1_RCAUSE_PORCORE = 1,   /* Core Power-on Reset */
	RSTC_G1_RCAUSE_BORVDDREG = 2, /* Brown-Out VDDREG Detector Reset */
	RSTC_G1_RCAUSE_BORVDDA = 3,   /* Brown-Out VDDA Detector Reset */
	RSTC_G1_RCAUSE_BORVDDIO = 4,  /* Brown-Out VDDIO Detector Reset */
	RSTC_G1_RCAUSE_EXT = 5,       /* External Reset */
	RSTC_G1_RCAUSE_WDT = 6,       /* Watchdog Reset */
	RSTC_G1_RCAUSE_SYST = 7,      /* System Reset Request */
	RSTC_G1_RCAUSE_BACKUP = 8,    /* Backup Reset */
	RSTC_G1_RCAUSE_LOCKUP = 9     /* CPU Lockup Reset */
};

/** This family resets the device when the core locks up and reports it. */
#define RSTC_G1_RCAUSE_HAS_LOCKUP 1

/** Reset-cause bits that all report through RESET_BROWNOUT. */
#define RSTC_G1_RCAUSE_BROWNOUT_MASK                                                               \
	(BIT(RSTC_G1_RCAUSE_PORCORE) | BIT(RSTC_G1_RCAUSE_BORVDDREG) |                             \
	 BIT(RSTC_G1_RCAUSE_BORVDDA) | BIT(RSTC_G1_RCAUSE_BORVDDIO))

/** Read the whole RCAUSE register at @p addr with this family's access width. */
#define RSTC_G1_RCAUSE_READ(addr) (*(volatile uint16_t *)(addr))
#else
enum rstc_g1_rcause {
	RSTC_G1_RCAUSE_POR = 0,   /* Power-on Reset */
	RSTC_G1_RCAUSE_BOD12 = 1, /* Brown-Out 1.2V Detector Reset */
	RSTC_G1_RCAUSE_BOD33 = 2, /* Brown-Out 3.3V Detector Reset */
	RSTC_G1_RCAUSE_NVM = 3,   /* NVM Reset */
	RSTC_G1_RCAUSE_EXT = 4,   /* External Reset */
	RSTC_G1_RCAUSE_WDT = 5,   /* Watchdog Reset */
	RSTC_G1_RCAUSE_SYST = 6,  /* System Reset Request */
	RSTC_G1_RCAUSE_BACKUP = 7 /* Backup Reset */
};

/** Reset-cause bits that all report through RESET_BROWNOUT. */
#define RSTC_G1_RCAUSE_BROWNOUT_MASK (BIT(RSTC_G1_RCAUSE_BOD12) | BIT(RSTC_G1_RCAUSE_BOD33))

/** Read the whole RCAUSE register at @p addr with this family's access width. */
#define RSTC_G1_RCAUSE_READ(addr)    (*(volatile uint8_t *)(addr))
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_SG_GC */

#ifdef CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_JH
/* Reserved reset-cause bits on PIC32CM JH */
#define RSTC_RESERVED_BIT_3     BIT(3)
#define RSTC_RESERVED_BIT_7     BIT(7)
#define RSTC_UNSUPPORTED_RCAUSE ((RSTC_RESERVED_BIT_3) | (RSTC_RESERVED_BIT_7))
#else
#define RSTC_UNSUPPORTED_RCAUSE 0U
#endif /* CONFIG_SOC_FAMILY_MICROCHIP_PIC32CM_JH */

#endif /* INCLUDE_ZEPHYR_DRIVERS_RESET_MCHP_RSTC_G1_H_ */
