/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_VCC0_H
#define ENE_KB106X_VCC0_H

/**
 * brief  Structure type to access VCC0 Power Domain Module (VCC0).
 */
struct vcc0_regs {
	volatile uint16_t PAREGPLC;     /*VCC0 PLC Register */
	volatile uint16_t PAREGFLAG;    /*VCC0 Flag Register */
	volatile uint8_t PB8SCR;        /*Power Button Override Control Register */
	volatile uint8_t Reserved0[3];  /*Reserved */
	volatile uint32_t PASCR;        /*VCC0 Scratch Register */
	volatile uint32_t Reserved1[5]; /*Reserved */
	volatile uint8_t Reserved2;     /*Reserved */
	volatile uint8_t IOSCCFG_T;     /*OSC32K Configuration Register */
	volatile uint16_t Reserved3;    /*Reserved */
	volatile uint32_t Reserved4[7]; /*Reserved */
	volatile uint8_t HIBTMRCFG;     /*Hibernation Timer Configuration Register */
	volatile uint8_t Reserved5[3];  /*Reserved */
	volatile uint8_t HIBTMRIE;      /*Hibernation Timer Interrupt Enable Register */
	volatile uint8_t Reserved6[3];  /*Reserved */
	volatile uint8_t HIBTMRPF;      /*Hibernation Timer Pending Flag Register */
	volatile uint8_t Reserved7[3];  /*Reserved */
	volatile uint32_t HIBTMRMAT;    /*Hibernation Timer Match Value Register */
	volatile uint32_t HIBTMRCNT;    /*Hibernation Timer count Value Register */
};

#endif /* ENE_KB106X_VCC0_H */
