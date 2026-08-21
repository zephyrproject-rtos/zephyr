/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_MBX_H
#define ENE_KB106X_MBX_H

/**
 * Structure type to access MialBox1/2 (MBX).
 */

struct mbx_regs {
	volatile uint32_t mbx_cfg;       /* Configuration Register */
	volatile uint8_t mbx_ie;         /* Interrupt Enable Register */
	volatile uint8_t reserved_0[3];  /* Reserved */
	volatile uint8_t mbx_pf;         /* Event Pending Flag Register */
	volatile uint8_t reserved_1[3];  /* Reserved */
	volatile uint32_t mbx_rba;       /* SRAM Base Address Register */
	volatile uint32_t mbx_ainfo;     /* Access Information Register */
	volatile uint32_t mbx_pc;        /* Protection Control Register */
	volatile uint32_t reserved_2[58]; /* Reserved */
};

#define MBX_FUNCTION_ENABLE BIT(0)

#define MBX_NOTIFY_EVENT BIT(0)

#define MBX_IO_BASE_POS 16

#endif /* ENE_KB106X_MBX_H */
