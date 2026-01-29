/*
 * Copyright (C) 2018-2024 Alibaba Group Holding Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_SMP

#include <smp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <csi_core.h>
#include <soc.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/riscv/csr.h>

/* clang-format off */

void xuantie_csr_copy(void)
{
	if (CONFIG_RV_BOOT_HART == __get_MHARTID()) {
		/* Load from boot core */
		xuantie_regs.pmpaddr0 = csr_read(CSR_PMPADDR0);
		xuantie_regs.pmpaddr1 = csr_read(CSR_PMPADDR1);
		xuantie_regs.pmpaddr2 = csr_read(CSR_PMPADDR2);
		xuantie_regs.pmpaddr3 = csr_read(CSR_PMPADDR3);
		xuantie_regs.pmpaddr4 = csr_read(CSR_PMPADDR4);
		xuantie_regs.pmpaddr5 = csr_read(CSR_PMPADDR5);
		xuantie_regs.pmpaddr6 = csr_read(CSR_PMPADDR6);
		xuantie_regs.pmpaddr7 = csr_read(CSR_PMPADDR7);
		xuantie_regs.pmpcfg0  = csr_read(CSR_PMPCFG0);
		xuantie_regs.mcor     = csr_read(CSR_MCOR);
		xuantie_regs.msmpr    = csr_read(CSR_MSMPR);
		xuantie_regs.mhcr     = csr_read(CSR_MHCR);
		xuantie_regs.mccr2    = csr_read(CSR_MCCR2);
		xuantie_regs.mhint    = csr_read(CSR_MHINT);
		xuantie_regs.mtvec    = csr_read(CSR_MTVEC);
		xuantie_regs.mie      = csr_read(CSR_MIE);
		xuantie_regs.mstatus  = csr_read(CSR_MSTATUS);
		xuantie_regs.mxstatus = csr_read(CSR_MXSTATUS);
	} else {
		/* Store to other core */
		/* csr_write(CSR_PMPADDR0, xuantie_regs.pmpaddr0); */
		/* csr_write(CSR_PMPADDR1, xuantie_regs.pmpaddr1); */
		/* csr_write(CSR_PMPADDR2, xuantie_regs.pmpaddr2); */
		/* csr_write(CSR_PMPADDR3, xuantie_regs.pmpaddr3); */
		/* csr_write(CSR_PMPADDR4, xuantie_regs.pmpaddr4); */
		/* csr_write(CSR_PMPADDR5, xuantie_regs.pmpaddr5); */
		/* csr_write(CSR_PMPADDR6, xuantie_regs.pmpaddr6); */
		/* csr_write(CSR_PMPADDR7, xuantie_regs.pmpaddr7); */
		/* csr_write(CSR_PMPCFG0,  xuantie_regs.pmpcfg0); */
		csr_write(CSR_MCOR,     xuantie_regs.mcor);
		csr_write(CSR_MSMPR,    xuantie_regs.msmpr);
		csr_write(CSR_MHCR,     xuantie_regs.mhcr);
		csr_write(CSR_MHINT,    xuantie_regs.mhint);
		/* csr_write(CSR_MTVEC,    xuantie_regs.mtvec); */
		/* csr_write(CSR_MIE,      xuantie_regs.mie); */
		csr_write(CSR_MSTATUS,  xuantie_regs.mstatus);
		csr_write(CSR_MXSTATUS, xuantie_regs.mxstatus);
	}
}

/* clang-format on */

int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	uint32_t mrmr;
	int cpu_num = cpuid;

	xuantie_csr_copy();
	*(unsigned long *)((unsigned long)XIAOHUI_SRESET_BASE + XIAOHUI_SRESET_ADDR_OFFSET +
					   ((cpu_num - 1) << 3)) = (unsigned long)entry_point;
	__asm__ volatile("fence" ::: "memory");
	mrmr = *(uint32_t *)(XIAOHUI_SRESET_BASE);
	*(uint32_t *)(XIAOHUI_SRESET_BASE) = mrmr | (0x1 << (cpu_num - 1));
	__asm__ volatile("fence" ::: "memory");

	return 0;
}

#endif
