/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/check.h>
#include <zephyr/arch/cpu.h>

#include <soc.h>
#include <adsp_boot.h>
#include <adsp_power.h>
#include <adsp_ipc_regs.h>
#include <adsp_memory.h>
#include <adsp_interrupt.h>
#include <zephyr/irq.h>

#define CORE_POWER_CHECK_NUM 32
#define ACE_INTC_IRQ DT_IRQN(DT_NODELABEL(ace_intc))

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

#define DFIDCCP			0x2020
#define CAP_INST_SHIFT		24
#define CAP_INST_MASK		BIT_MASK(4)

unsigned int soc_num_cpus;

static __imr int soc_num_cpus_init(void)
{
	/* Need to set soc_num_cpus early to arch_num_cpus() works properly */
	soc_num_cpus = ((sys_read32(DFIDCCP) >> CAP_INST_SHIFT) & CAP_INST_MASK) + 1;
	soc_num_cpus = MIN(CONFIG_MP_MAX_NUM_CPUS, soc_num_cpus);

	return 0;
}
SYS_INIT(soc_num_cpus_init, EARLY, 1);

void soc_mp_init(void)
{
	IRQ_CONNECT(ACE_IRQ_TO_ZEPHYR(ACE_INTL_IDCA), 0, ipc_isr, 0, 0);

	irq_enable(ACE_IRQ_TO_ZEPHYR(ACE_INTL_IDCA));

	unsigned int num_cpus = arch_num_cpus();

	for (int i = 0; i < num_cpus; i++) {
		/* DINT has one bit per IPC, unmask only IPC "Ax" on core "x" */
		ACE_DINT[i].ie[ACE_INTL_IDCA] = BIT(i);

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
		DSPCS.bootctl[cpu_num].battr |= DSPBR_BATTR_LPSCTL_BATTR_SLAVE_CORE;
	}

	/* Setting the Power Active bit to the off state before powering up the core. This step is
	 * required by the HW if we are starting core for a second time. Without this sequence, the
	 * core will not power on properly when doing transition D0->D3->D0.
	 */
	DSPCS.capctl[cpu_num].ctl &= ~DSPCS_CTL_SPA;

	/* Checking current power status of the core. */
	while (((DSPCS.capctl[cpu_num].ctl & DSPCS_CTL_CPA) == DSPCS_CTL_CPA)) {
		k_busy_wait(HW_STATE_CHECK_DELAY);
	}

	DSPCS.capctl[cpu_num].ctl |= DSPCS_CTL_SPA;

	/* Waiting for power up */
	while (((DSPCS.capctl[cpu_num].ctl & DSPCS_CTL_CPA) != DSPCS_CTL_CPA) &&
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
	DSPCS.bootctl[cpu].bctl |=
		DSPBR_BCTL_WAITIPCG | DSPBR_BCTL_WAITIPPG;
}

void arch_sched_ipi(void)
{
	uint32_t curr = arch_proc_id();

	/* Signal agent B[n] to cause an interrupt from agent A[n] */
	unsigned int num_cpus = arch_num_cpus();

	for (int core = 0; core < num_cpus; core++) {
		if (core != curr && soc_cpus_active[core]) {
			IDC[core].agents[1].ipc.idr = INTEL_ADSP_IPC_BUSY;
		}
	}
}

#if CONFIG_MP_MAX_NUM_CPUS > 1
int soc_adsp_halt_cpu(int id)
{
	int retry = CORE_POWER_CHECK_NUM;

	CHECKIF(arch_proc_id() != 0) {
		return -EINVAL;
	}

	CHECKIF(id <= 0 || id >= arch_num_cpus()) {
		return -EINVAL;
	}

	CHECKIF(soc_cpus_active[id]) {
		return -EINVAL;
	}

	DSPCS.capctl[id].ctl &= ~DSPCS_CTL_SPA;

	/* Waiting for power off */
	while (((DSPCS.capctl[id].ctl & DSPCS_CTL_CPA) == DSPCS_CTL_CPA) &&
	       (retry > 0)) {
		k_busy_wait(HW_STATE_CHECK_DELAY);
		retry--;
	}

	if (retry == 0) {
		__ASSERT(false, "%s secondary core has not powered down", __func__);
		return -EINVAL;
	}

	ACE_PWRCTL->wpdsphpxpg &= ~BIT(id);
	return 0;
}
#endif
