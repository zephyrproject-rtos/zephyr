/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ZEPHYR_SOC_INTEL_ADSP_VECTORS
#define _ZEPHYR_SOC_INTEL_ADSP_VECTORS

/* Definitions for our linker script to place and size all the bits of
 * the interrupt vector tables.  Originally these were SOF-derived and
 * part of a platform abstraction layer, but all current hardware has
 * the same values.
 *
 * FIXME: really, we want to be computing this from the core-isa.h
 * file, where the offsets from VECBASE are available as
 * "XCHAL_*_VECOFS" symbols, etc...  There's no reason for us to be
 * managing these numbers (nor especially for us to be usurping the
 * XCHAL_ namespace to do it!).
 */

/* This is the base address of all the vectors defined in SRAM */
#define XCHAL_VECBASE_RESET_PADDR_SRAM \
	(L2_SRAM_BASE + CONFIG_HP_SRAM_RESERVE)

#define MEM_VECBASE_LIT_SIZE                   0x178

/* The addresses of the vectors in SRAM.
 * Only the memerror vector continues to point to its ROM address.
 */
#define XCHAL_INTLEVEL2_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x180)

#define XCHAL_INTLEVEL3_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x1C0)

#define XCHAL_INTLEVEL4_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x200)

#define XCHAL_INTLEVEL5_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x240)

#define XCHAL_INTLEVEL6_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x280)

#define XCHAL_INTLEVEL7_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x2C0)

#define XCHAL_KERNEL_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x300)

#define XCHAL_USER_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x340)

#define XCHAL_DOUBLEEXC_VECTOR_PADDR_SRAM \
	(XCHAL_VECBASE_RESET_PADDR_SRAM + 0x3C0)

#define VECTOR_TBL_SIZE				0x0400

/* Vector and literal sizes */
#define MEM_VECT_LIT_SIZE			0x8
#define MEM_VECT_TEXT_SIZE			0x38

#define MEM_ERROR_TEXT_SIZE			0x180
#define MEM_ERROR_LIT_SIZE			0x8

#endif /* _ZEPHYR_SOC_INTEL_ADSP_VECTORS */
