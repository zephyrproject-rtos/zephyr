/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WATCHDOG_WDT_NPCM_REGISTERS_H_
#define ZEPHYR_DRIVERS_WATCHDOG_WDT_NPCM_REGISTERS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Timer and Watchdog (TWD) device registers
 */
struct twd_reg {
	/* 0x000: Timer and Watchdog Configuration */
	volatile uint8_t twcfg;
	volatile uint8_t reserved1;
	/* 0x002: Timer and Watchdog Clock Prescaler */
	volatile uint8_t twcp;
	volatile uint8_t reserved2;
	/* 0x004: TWD Timer 0 */
	volatile uint16_t twdt0;
	/* 0x006: TWDT0 Control and Status */
	volatile uint8_t t0csr;
	volatile uint8_t reserved3;
	/* 0x008: Watchdog Count */
	volatile uint8_t wdcnt;
	volatile uint8_t reserved4;
	/* 0x00A: Watchdog Service Data Match */
	volatile uint8_t wdsdm;
	volatile uint8_t reserved5;
	/* 0x00C: TWD Timer 0 Counter */
	volatile uint16_t twmt0;
	/* 0x00E: Watchdog Counter */
	volatile uint8_t twmwd;
	volatile uint8_t reserved6;
	/* 0x010: Watchdog Clock Prescaler */
	volatile uint8_t wdcp;
};

/* TWCFG register fields */
#define NPCM_TWCFG_LTWCFG 0
#define NPCM_TWCFG_LTWCP  1
#define NPCM_TWCFG_LTWDT0 2
#define NPCM_TWCFG_LWDCNT 3
#define NPCM_TWCFG_WDCT0I 4
#define NPCM_TWCFG_WDSDME 5

/* T0CSR register fields */
#define NPCM_T0CSR_RST       0
#define NPCM_T0CSR_TC        1
#define NPCM_T0CSR_WDLTD     3
#define NPCM_T0CSR_WDRST_STS 4
#define NPCM_T0CSR_WD_RUN    5
#define NPCM_T0CSR_T0EN      6
#define NPCM_T0CSR_TESDIS    7

/* SCFG Debug Freeze Enable 3 register */
#define NPCM_SCFG_DBGFRZEN3_OFFSET  0x078
#define NPCM_DBGFRZEN3_GLBL_FRZ_DIS 7

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_WATCHDOG_WDT_NPCM_REGISTERS_H_ */
