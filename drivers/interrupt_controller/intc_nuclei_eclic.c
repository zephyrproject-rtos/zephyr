/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nuclie's Extended Core Interrupt Controller
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <soc.h>
#include <arch/riscv/csr.h>

#include <sw_isr_table.h>

static inline void eclic_init(uint32_t num_irq)
{
	ECLIC_SetMth(0);
	ECLIC_SetCfgNlbits(__ECLIC_INTCTLBITS);
}

static inline void eclic_mode_enable(void)
{
	uint32_t mtvec_value = csr_read(mtvec);

	mtvec_value = mtvec_value & 0xFFFFFFC0;
	mtvec_value = mtvec_value | 0x00000003;
	csr_write(mtvec, mtvec_value);
}

static inline void eclic_global_interrupt_enable(void)
{
	/* set machine interrupt enable bit */
	csr_set(mstatus, MSTATUS_MIE);
}

/**
 * @brief Initialize the Platform Level Interrupt Controller
 * @return N/A
 */
static int _eclic_init(const struct device *dev)
{
	/* Initialze ECLIC */
	eclic_init(SOC_ECLIC_NUM_INTERRUPTS);
	eclic_mode_enable();
	eclic_global_interrupt_enable();

	return 0;
}

SYS_INIT(_eclic_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
