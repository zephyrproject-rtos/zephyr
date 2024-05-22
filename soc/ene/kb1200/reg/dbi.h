/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_DBI_H
#define ENE_KB1200_DBI_H

/**
 *  Structure type to access Debug Port Interface - 0x80/0x81 (DBI).
 */
struct dbi_regs {
	volatile uint32_t DBICFG;       /*Configuration Register */
	volatile uint8_t DBIIE;         /*Interrupt Enable Register */
	volatile uint8_t Reserved0[3];  /*Reserved */
	volatile uint8_t DBIPF;         /*Event Pending Flag Register */
	volatile uint8_t Reserved1[3];  /*Reserved */
	volatile uint32_t Reserved2;    /*Reserved */
	volatile uint8_t DBIODP;        /*Output Data Port Register */
	volatile uint8_t Reserved3[3];  /*Reserved */
	volatile uint8_t DBIIDP;        /*Input Data Port Register */
	volatile uint8_t Reserved4[3];  /*Reserved */
	volatile uint32_t Reserved5[2]; /*Reserved */
};

#define DBI_FUNCTION_ENABLE 0x01
#define DBI_RX_EVENT        0x01

#endif /* ENE_KB1200_DBI_H */
