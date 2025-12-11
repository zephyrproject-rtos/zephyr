/*
 * Copyright (c) 2025 Microchip Technology Inc.
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

#endif /* INCLUDE_ZEPHYR_DRIVERS_RESET_MCHP_RSTC_G1_H_ */
