/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_DBI_H
#define ENE_KB106X_DBI_H

/**
 *  Structure type to access Debug Port Interface - 0x80/0x81 (DBI).
 */
struct dbi_regs {
	volatile uint32_t dbi_cfg;       /*Configuration Register */
	volatile uint8_t dbi_ie;         /*Interrupt Enable Register */
	volatile uint8_t reserved_0[3];  /*Reserved */
	volatile uint8_t dbi_pf;         /*Event Pending Flag Register */
	volatile uint8_t reserved_1[3];  /*Reserved */
	volatile uint32_t Reserved_2;    /*Reserved */
	volatile uint8_t dbi_odp;        /*Output Data Port Register */
	volatile uint8_t reserved_3[3];  /*Reserved */
	volatile uint8_t dbi_idp;        /*Input Data Port Register */
	volatile uint8_t reserved_4[3];  /*Reserved */
	volatile uint32_t reserved_5[2]; /*Reserved */
};

#define DBI_FUNCTION_ENABLE 0x01
#define DBI_BASE_POS        16
#define DBI_BASE_MASK       0xFFFF

#define DBI_RX_EVENT 0x01

#endif /* ENE_KB106X_DBI_H */
