/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_XBI_H
#define ENE_KB106X_XBI_H

/**
 * brief  Structure type to access X-Bus Interface (XBI).
 */
struct xbi_regs {
	volatile uint32_t EFCFG;       /* Configuration Register */
	volatile uint8_t EFSTA;        /* Status Register */
	volatile uint8_t Reserved0[3]; /* Reserved */
	volatile uint32_t EFADDR;      /* Address Register */
	volatile uint16_t EFCMD;       /* Command Register */
	volatile uint16_t Reserved1;   /* Reserved */
	volatile uint32_t EFDAT;       /* Data Register */
	volatile uint8_t EFPORF;       /* POR Flag Register */
	volatile uint8_t Reserved2[3]; /* Reserved */
	volatile uint8_t EFBCNT;       /* Burst Conuter Register */
};

/* EFCMD support command */
#define CMD_SECTOR_ERASE_SINGLE        0x0010
#define CMD_SECTOR_ERASE_INCREASE      0x0110
#define CMD_PROGRAM_SINGLE             0x0021
#define CMD_PROGRAM_INCREASE           0x0121
#define CMD_BURSTPROGRAM_SINGLE        0x0027
#define CMD_BURSTPROGRAM_INCREASE      0x0127
#define CMD_READ32_SINGLE              0x0034
#define CMD_READ32_INCREASE            0x0134
#define CMD_NVR_READ32_SINGLE          0x0074
#define CMD_NVR_READ32_INCREASE        0x0174
#define CMD_SET_CONFIGURATION_SINGLE   0x0080
#define CMD_SET_CONFIGURATION_INCREASE 0x0180

/* Trim - NVR0 */
#define ENE_TRIM_NVR0_ADDRESS        0x00000000
#define ENE_TRIM_NVR0_OSC32K_ADDRESS 0x0000000C
#define ENE_TRIM_VALID_FLAG          0x5A
#define LAB_TRIM_VALID_FLAG          0xAB

#endif /* ENE_KB106X_XBI_H */
