/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	generic/zynqmp_r5/sys.c
 * @brief	machine specific system primitives implementation.
 */

#include <metal/compiler.h>
#include <metal/io.h>
#include <metal/sys.h>
#include <stdint.h>
#include "xil_cache.h"
#include "xil_exception.h"
#include "xil_mmu.h"
#include "xil_mpu.h"
#include "xreg_cortexr5.h"
#include "xscugic.h"

#define MPU_REGION_SIZE_MIN 0x20

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
	size_t rsize = MPU_REGION_SIZE_MIN;
	metal_phys_addr_t base_pa;

	if (!flags)
		return va;
	while(1) {
		if (rsize < size) {
			rsize <<= 1;
			continue;
		} else {
			base_pa = pa & ~(rsize - 1);
			if ((base_pa + rsize) < (pa + size)) {
				rsize <<= 1;
				continue;
			}
			break;
		}
	}
	Xil_SetMPURegion(base_pa, rsize, flags);
	return va;
}
