/*
 * Copyright (c) 2017 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_MEMORY_H
#define __INC_MEMORY_H

/* L2 HP SRAM */
#define L2_VECTOR_SIZE				0x1000

/* The reset vector address in SRAM and its size */
#define XCHAL_RESET_VECTOR0_PADDR_SRAM		L2_SRAM_BASE
#define MEM_RESET_TEXT_SIZE			0x268
#define MEM_RESET_LIT_SIZE			0x8

/* This is the base address of all the vectors defined in SRAM */
#define XCHAL_VECBASE_RESET_PADDR_SRAM		(L2_SRAM_BASE + 0x400)
#define MEM_VECBASE_LIT_SIZE			0x178

/* The addresses of the vectors in SRAM.
 * Only the memerror vector continues to point to its ROM address.
 */
#define XCHAL_INTLEVEL2_VECTOR_PADDR_SRAM	(L2_SRAM_BASE + 0x580)
#define XCHAL_INTLEVEL3_VECTOR_PADDR_SRAM	(L2_SRAM_BASE + 0x5C0)
#define XCHAL_INTLEVEL4_VECTOR_PADDR_SRAM	(L2_SRAM_BASE + 0x600)
#define XCHAL_INTLEVEL5_VECTOR_PADDR_SRAM	(L2_SRAM_BASE + 0x640)
#define XCHAL_INTLEVEL6_VECTOR_PADDR_SRAM	(L2_SRAM_BASE + 0x680)
#define XCHAL_INTLEVEL7_VECTOR_PADDR_SRAM	(L2_SRAM_BASE + 0x6C0)
#define XCHAL_KERNEL_VECTOR_PADDR_SRAM		(L2_SRAM_BASE + 0x700)
#define XCHAL_USER_VECTOR_PADDR_SRAM		(L2_SRAM_BASE + 0x740)
#define XCHAL_DOUBLEEXC_VECTOR_PADDR_SRAM	(L2_SRAM_BASE + 0x7C0)

/* Vector and literal sizes */
#define MEM_VECT_LIT_SIZE			0x8
#define MEM_VECT_TEXT_SIZE			0x38
#define MEM_VECT_SIZE				(MEM_VECT_TEXT_SIZE +\
						MEM_VECT_LIT_SIZE)

/* The memerror vector address is copied as is from core-isa.h */
#define XCHAL_MEMERROR_VECTOR_PADDR		0xBEFE0400

#define MEM_ERROR_TEXT_SIZE			0x180
#define MEM_ERROR_LIT_SIZE			0x8

/* text and data share the same L2 HP SRAM on Intel S1000 */
#define TEXT_BASE				(L2_SRAM_BASE + L2_VECTOR_SIZE)
#define TEXT_SIZE				0x16000

/* size of the Interrupt Descriptor Table (IDT) */
#define IDT_SIZE				0x2000

/* initialized data */
#define DATA_SIZE				0x10000

/* bss data */
#define BSS_DATA_SIZE				0x8000

#endif /* __INC_MEMORY_H */
