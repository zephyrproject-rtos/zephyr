/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <soc.h>
#include <ace_v1x-regs.h>
#include <ace-ipc-regs.h>
#include <cavs-mem.h>

#define CORE_POWER_CHECK_NUM 32
#define CORE_POWER_CHECK_DELAY 256

static void ipc_isr(void *arg)
{
	MTL_P2P_IPC[arch_proc_id()].agents[0].ipc.tdr = BIT(31); /* clear BUSY bit */
#ifdef CONFIG_SMP
	void z_sched_ipi(void);
	z_sched_ipi();
#endif
}

void soc_mp_init(void)
{
	IRQ_CONNECT(MTL_IRQ_TO_ZEPHYR(MTL_INTL_IDCA), 0, ipc_isr, 0, 0);

	irq_enable(MTL_IRQ_TO_ZEPHYR(MTL_INTL_IDCA));

	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		/* DINT has one bit per IPC, unmask only IPC "Ax" on core "x" */
		MTL_DINT[i].ie[MTL_INTL_IDCA] = BIT(i);

		/* Agent A should signal only BUSY interrupts */
		MTL_P2P_IPC[i].agents[0].ipc.ctl = BIT(0); /* IPCTBIE */
	}

	/* Set the core 0 active */
	soc_cpus_active[0] = true;
}

void soc_start_core(int cpu_num)
{
	int retry = CORE_POWER_CHECK_NUM;

	if (cpu_num > 0) {
		/* Initialize the ROM jump address */
		uint32_t *rom_jump_vector = (uint32_t *) ROM_JUMP_ADDR;
		*rom_jump_vector = (uint32_t) z_soc_mp_asm_entry;
		z_xtensa_cache_flush(rom_jump_vector, sizeof(*rom_jump_vector));

		/* Tell the ACE ROM that it should use secondary core flow */
		MTL_PWRBOOT.bootctl[cpu_num].battr |= MTL_PWRBOOT_BATTR_LPSCTL_BATTR_SLAVE_CORE;
	}

	MTL_PWRBOOT.capctl[cpu_num].ctl |= MTL_PWRBOOT_CTL_SPA;

	/* Waiting for power up */
	while (~(MTL_PWRBOOT.capctl[cpu_num].ctl & MTL_PWRBOOT_CTL_CPA) && --retry) {
		k_busy_wait(CORE_POWER_CHECK_DELAY);
	}

	if (!retry) {
		__ASSERT(false, "%s secondary core has not powered up", __func__);
	}
}

void soc_mp_startup(uint32_t cpu)
{
	/* Must have this enabled always */
	z_xtensa_irq_enable(ACE_INTC_IRQ);

	/* Prevent idle from powering us off */
	MTL_PWRBOOT.bootctl[cpu].bctl |=
		MTL_PWRBOOT_BCTL_WAITIPCG | MTL_PWRBOOT_BCTL_WAITIPPG;
}

void arch_sched_ipi(void)
{
	uint32_t curr = arch_proc_id();

	/* Signal agent B[n] to cause an interrupt from agent A[n] */
	for (int core = 0; core < CONFIG_MP_NUM_CPUS; core++) {
		if (core != curr && soc_cpus_active[core]) {
			MTL_P2P_IPC[core].agents[1].ipc.idr = CAVS_IPC_BUSY;
		}
	}
}
