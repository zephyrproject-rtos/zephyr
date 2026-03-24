/*
 * Copyright (c) 2025 NVIDIA Corporation
 *
 * Convenience macros for assembly code
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Convenience macros for assembly code. */


/*
 * Helper macro which stores the value of a register to a the address contained
 * in a pointer register plus an immediate offset.
 */

.macro op_store_reg reg, off, ptr_reg
	l.sw \off(\ptr_reg), \reg
.endm


/*
 * Helper macro which loads a value to a register from an address contained in
 * a pointer register plus an immediate offset.
 */

.macro op_load_reg reg, off, ptr_reg
	l.lwz \reg, \off(\ptr_reg)
.endm
