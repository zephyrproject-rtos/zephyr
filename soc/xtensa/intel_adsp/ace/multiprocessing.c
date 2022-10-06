/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/check.h>

#include <soc.h>
#include <ace_v1x-regs.h>
#include <adsp_ipc_regs.h>
#include <adsp_memory.h>

#define CORE_POWER_CHECK_NUM 32

static void ipc_isr(void *arg)
{
	uint32_t cpu_id = arch_proc_id();

	/*
	 * Clearing the BUSY bits in both TDR and TDA are needed to
	 * complete an IDC message. If we do only one (and not both),
	 * the other side will not be able to send another IDC
	 * message as the hardware still thinks you are processing
	 * the IDC message (and thus will not send another one).
	 * On TDR, it is to write one to clear, while on TDA, it is
	 * to write zero to clear.
	 */
	IDC[cpu_id].agents[0].ipc.tdr = BIT(31);
	IDC[cpu_id].agents[0].ipc.tda = 0;

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
		IDC[i].agents[0].ipc.ctl = BIT(0); /* IPCTBIE */
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
		ACE_PWRCTL->wpdsphpxpg |= BIT(cpu_num);

		while ((ACE_PWRSTS->dsphpxpgs & BIT(cpu_num)) == 0) {
			k_busy_wait(HW_STATE_CHECK_DELAY);
		}

		/* Tell the ACE ROM that it should use secondary core flow */
		DFDSPBRCP.bootctl[cpu_num].battr |= DFDSPBRCP_BATTR_LPSCTL_BATTR_SLAVE_CORE;
	}

	DFDSPBRCP.capctl[cpu_num].ctl |= DFDSPBRCP_CTL_SPA;

	/* Waiting for power up */
	while (((DFDSPBRCP.capctl[cpu_num].ctl & DFDSPBRCP_CTL_CPA) != DFDSPBRCP_CTL_CPA) &&
	       (retry > 0)) {
		k_busy_wait(HW_STATE_CHECK_DELAY);
		retry--;
	}

	if (retry == 0) {
		__ASSERT(false, "%s secondary core has not powered up", __func__);
	}
}

void soc_mp_startup(uint32_t cpu)
{
	/* Must have this enabled always */
	z_xtensa_irq_enable(ACE_INTC_IRQ);

	/* Prevent idle from powering us off */
	DFDSPBRCP.bootctl[cpu].bctl |=
		DFDSPBRCP_BCTL_WAITIPCG | DFDSPBRCP_BCTL_WAITIPPG;
	/* checking if WDT was stopped during D3 transition */
	if (DFDSPBRCP.bootctl[cpu].wdtcs & DFDSPBRCP_WDT_RESUME) {
		DFDSPBRCP.bootctl[cpu].wdtcs = DFDSPBRCP_WDT_RESUME;
		/* TODO: delete this IF when FW starts using imr restore vector */
	}
}

void arch_sched_ipi(void)
{
	uint32_t curr = arch_proc_id();

	/* Signal agent B[n] to cause an interrupt from agent A[n] */
	for (int core = 0; core < CONFIG_MP_NUM_CPUS; core++) {
		if (core != curr && soc_cpus_active[core]) {
			IDC[core].agents[1].ipc.idr = INTEL_ADSP_IPC_BUSY;
		}
	}
}

int soc_adsp_halt_cpu(int id)
{
	int retry = CORE_POWER_CHECK_NUM;

	CHECKIF(arch_proc_id() != 0) {
		return -EINVAL;
	}

	CHECKIF(id <= 0 || id >= CONFIG_MP_NUM_CPUS) {
		return -EINVAL;
	}

	CHECKIF(!soc_cpus_active[id]) {
		return -EINVAL;
	}

	DFDSPBRCP.capctl[id].ctl &= ~DFDSPBRCP_CTL_SPA;

	/* Waiting for power off */
	while (((DFDSPBRCP.capctl[id].ctl & DFDSPBRCP_CTL_CPA) == DFDSPBRCP_CTL_CPA) &&
	       (retry > 0)) {
		k_busy_wait(HW_STATE_CHECK_DELAY);
		retry--;
	}

	if (retry == 0) {
		__ASSERT(false, "%s secondary core has not powered down", __func__);
		return -EINVAL;
	}

	/* Stop sending IPIs to this core */
	soc_cpus_active[id] = false;

	return 0;
}
