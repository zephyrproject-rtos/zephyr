/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_EC_H
#define ENE_KB106X_EC_H

/**
 *  Structure type to access EC Interface (ECI).
 */
struct eci_regs {
	volatile uint32_t eci_cfg;      /*Configuration Register */
	volatile uint8_t eci_ie;        /*Interrupt Enable Register */
	volatile uint8_t reserved_0[3]; /*Reserved */
	volatile uint8_t eci_pf;        /*Event Pending Flag Register */
	volatile uint8_t reserved_1[3]; /*Reserved */
	volatile uint32_t Reserved_2;   /*Reserved */
	volatile uint8_t eci_sts;       /*Status Register */
	volatile uint8_t reserved_3[3]; /*Reserved */
	volatile uint8_t eci_cmd;       /*Command Register */
	volatile uint8_t reserved_4[3]; /*Reserved */
	volatile uint8_t eci_odp;       /*Output Data Port Register */
	volatile uint8_t reserved_5[3]; /*Reserved */
	volatile uint8_t eci_idp;       /*Input Dtat Port Register */
	volatile uint8_t reserved_6[3]; /*Reserved */
	volatile uint8_t eci_scid;      /*SCI ID Write Port Register */
	volatile uint8_t reserved_7[3]; /*Reserved */
};

#define ECI_FUNCTION_ENABLE BIT(0)
#define ECI_BASE_POS        16
#define ECI_BASE_MASK       0xFFFF

#define ECI_OBE_EVENT BIT(0)
#define ECI_IBF_EVENT BIT(1)

#define ECISTS_OBF              BIT(0)
#define ECISTS_IBF              BIT(1)
#define ECISTS_ADDRESS_CMD_PORT BIT(3)
#define ECISTS_BURST_MODE       BIT(4)
#define ECISTS_SCI_QUERY_FLAG   BIT(5)

#endif /* ENE_KB106X_EC_H */
