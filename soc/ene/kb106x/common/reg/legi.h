/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_LEGI_H
#define ENE_KB106X_LEGI_H

/**
 * Structure type to access Legacy Interface/Port - 68h/6Ch and 78h/7Ch (LEGI).
 */

struct legi_regs {
	volatile uint32_t legi_cfg;     /*Configuration Register */
	volatile uint8_t legi_ie;       /*Interrupt Enable Register */
	volatile uint8_t reserved_0[3]; /*Reserved */
	volatile uint8_t legi_pf;       /*Event Pending Flag Register */
	volatile uint8_t reserved1[3]; /*Reserved */
	volatile uint32_t Reserved_2;   /*Reserved */
	volatile uint8_t legi_sts;      /*Status Register */
	volatile uint8_t reserved_3[3]; /*Reserved */
	volatile uint8_t legi_cmd;      /*Command Port Register */
	volatile uint8_t reserved_4[3]; /*Reserved */
	volatile uint8_t legi_odp;      /*Output Data Port Register */
	volatile uint8_t reserved_5[3]; /*Reserved */
	volatile uint8_t legi_idp;      /*Input Data Port Register */
	volatile uint8_t reserved_6[3]; /*Reserved */
};

#define LEGI_FUNCTION_ENABLE BIT(0)
#define LEGI_IBF_EVENT       BIT(1)
#define LEGI_OBE_EVENT       BIT(0)

#define LEGI_INDICATOR_FLAG BIT(6)

#define LEGI_BASE_POS   16
#define LEGI_OFFSET_POS 8

#define LEGISTS_OBF              0x01
#define LEGISTS_IBF              0x02
#define LEGISTS_BUSY             0x80
#define LEGISTS_ADDRESS_CMD_PORT 0x08

#endif /* ENE_KB106X_LEGI_H */
