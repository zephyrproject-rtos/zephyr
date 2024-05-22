/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_ECI_H
#define ENE_KB1200_ECI_H

/**
 *  Structure type to access EC Interface (ECI).
 */
struct eci_regs {
	volatile uint32_t ECICFG;      /*Configuration Register */
	volatile uint8_t ECIIE;        /*Interrupt Enable Register */
	volatile uint8_t Reserved0[3]; /*Reserved */
	volatile uint8_t ECIPF;        /*Event Pending Flag Register */
	volatile uint8_t Reserved1[3]; /*Reserved */
	volatile uint32_t Reserved2;   /*Reserved */
	volatile uint8_t ECISTS;       /*Status Register */
	volatile uint8_t Reserved3[3]; /*Reserved */
	volatile uint8_t ECICMD;       /*Command Register */
	volatile uint8_t Reserved4[3]; /*Reserved */
	volatile uint8_t ECIODP;       /*Output Data Port Register */
	volatile uint8_t Reserved5[3]; /*Reserved */
	volatile uint8_t ECIIDP;       /*Input Dtat Port Register */
	volatile uint8_t Reserved6[3]; /*Reserved */
	volatile uint8_t ECISCID;      /*SCI ID Write Port Register */
	volatile uint8_t Reserved7[3]; /*Reserved */
};

#define ECI_FUNCTION_ENABLE 0x01

#define ECI_OBF_EVENT 0x01
#define ECI_IBF_EVENT 0x02

#define ECISTS_OBF              0x01
#define ECISTS_IBF              0x02
#define ECISTS_ADDRESS_CMD_PORT 0x08

#endif /* ENE_KB1200_ECI_H */
