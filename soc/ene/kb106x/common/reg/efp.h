/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_EFP_H
#define ENE_KB106X_EFP_H

/**
 * brief  Structure type to eFlash Protect.
 */
struct efp_regs {
	volatile uint32_t EFPADDR;     /*EFP0 Protect Address Register */
	volatile uint32_t EFPCR;       /*EFP0 Protect Control Register */
	volatile uint32_t Reserved[2]; /*Reserved */
};

#define EFP_END_UNIT_POS      16
#define EFP_LOCK_POS          1
#define EFP_PROTECT_TYPE_MASK 0x3FF0

#endif /* ENE_KB106X_EFP_H */
