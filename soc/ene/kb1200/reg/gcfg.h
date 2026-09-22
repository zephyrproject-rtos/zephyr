/*
 * Copyright (c) 2023 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_GCFG_H
#define ENE_KB1200_GCFG_H

/**
 *  Structure type to access General Configuration (GCFG).
 */
struct gcfg_regs {
	volatile uint8_t IDV;            /*Version ID Register */
	volatile uint8_t Reserved0;      /*Reserved */
	volatile uint16_t IDC;           /*Chip ID Register */
	volatile uint32_t FWID;          /*Firmware ID Register */
	volatile uint32_t MCURST;        /*MCU Reset Control Register */
	volatile uint32_t RSTFLAG;       /*Reset Pending Flag Register */
	volatile uint32_t GPIOALT;       /*GPIO Alternate Register */
	volatile uint8_t VCCSTA;         /*VCC Status Register */
	volatile uint8_t Reserved1[3];   /*Reserved */
	volatile uint16_t GPIOMUX;       /*GPIO MUX Control Register */
	volatile uint16_t Reserved2;     /*Reserved */
	volatile uint16_t I2CSPMS;       /*I2CS Pin Map Selection Register */
	volatile uint16_t Reserved3;     /*Reserved */
	volatile uint8_t CLKCFG;         /*Clock Configuration Register */
	volatile uint8_t Reserved4[3];   /*Reserved */
	volatile uint32_t DPLLFREQ;      /*DPLL Frequency Register */
	volatile uint32_t Reserved5;     /*Reserved */
	volatile uint32_t GCFGMISC;      /*Misc. Register */
	volatile uint8_t EXTIE;          /*Extended Command Interrupt Enable Register */
	volatile uint8_t Reserved6[3];   /*Reserved */
	volatile uint8_t EXTPF;          /*Extended Command Pending Flag Register */
	volatile uint8_t Reserved7[3];   /*Reserved */
	volatile uint32_t EXTARG;        /*Extended Command Argument0/1/2 Register */
	volatile uint8_t EXTCMD;         /*Extended Command Port Register */
	volatile uint8_t Reserved8[3];   /*Reserved */
	volatile uint32_t ADCOTR;        /*ADCO Register */
	volatile uint32_t IDSR;          /*IDSR Register */
	volatile uint32_t Reserved9[14]; /*Reserved */
	volatile uint32_t TRAPMODE;
	volatile uint32_t CLK1UCFG;
	volatile uint32_t LDO15TRIM;
	volatile uint32_t Reserved10;
	volatile uint32_t WWTR;
	volatile uint32_t ECMISC2;
	volatile uint32_t DPLLCTRL;
};

#define GCFG_CLKCFG_96M 0x00000004
#define GCFG_CLKCFG_48M 0x00000014
#define GCFG_CLKCFG_24M 0x00000024

#endif /* ENE_KB1200_GCFG_H */
