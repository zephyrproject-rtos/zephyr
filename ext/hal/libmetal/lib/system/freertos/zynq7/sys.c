/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	freertos/zynq7/sys.c
 * @brief	machine specific system primitives implementation.
 */

#include <metal/compiler.h>
#include <metal/io.h>
#include <metal/sys.h>
#include <stdint.h>
#include "xil_cache.h"
#include "xil_exception.h"
#include "xil_mmu.h"
#include "xscugic.h"

/* Translation table is 16K in size */
#define     ARM_AR_MEM_TTB_SIZE                    16*1024

/* Each TTB descriptor covers a 1MB region */
#define     ARM_AR_MEM_TTB_SECT_SIZE               1024*1024

/* Mask off lower bits of addr */
#define     ARM_AR_MEM_TTB_SECT_SIZE_MASK          (~(ARM_AR_MEM_TTB_SECT_SIZE-1UL))

/* default value setting for disabling interrupts */
static unsigned int int_old_val = XIL_EXCEPTION_ALL;

void sys_irq_restore_enable(void)
{
	Xil_ExceptionEnableMask(~int_old_val);
}

void sys_irq_save_disable(void)
{
	int_old_val = mfcpsr() & XIL_EXCEPTION_ALL;

	if (XIL_EXCEPTION_ALL != int_old_val) {
		Xil_ExceptionDisableMask(XIL_EXCEPTION_ALL);
	}
}

void metal_machine_cache_flush(void *addr, unsigned int len)
{
	if (!addr && !len)
		Xil_DCacheFlush();
	else
		Xil_DCacheFlushRange((intptr_t)addr, len);
}

void metal_machine_cache_invalidate(void *addr, unsigned int len)
{
	if (!addr && !len)
		Xil_DCacheInvalidate();
	else
		Xil_DCacheInvalidateRange((intptr_t)addr, len);
}

/**
 * @brief poll function until some event happens
 */
void metal_weak metal_generic_default_poll(void)
{
	asm volatile("wfi");
}

void *metal_machine_io_mem_map(void *va, metal_phys_addr_t pa,
				      size_t size, unsigned int flags)
{
	unsigned int section_offset;
	unsigned int ttb_addr;

	if (!flags)
		return va;
	/* Ensure the virtual and physical addresses are aligned on a
	   section boundary */
	pa &= ARM_AR_MEM_TTB_SECT_SIZE_MASK;

	/* Loop through entire region of memory (one MMU section at a time).
	   Each section requires a TTB entry. */
	for (section_offset = 0; section_offset < size;
	     section_offset += ARM_AR_MEM_TTB_SECT_SIZE) {

		/* Calculate translation table entry for this memory section */
		ttb_addr = (pa + section_offset);

		/* Write translation table entry value to entry address */
		Xil_SetTlbAttributes(ttb_addr, flags);
	}

	return va;
}
