/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <cavs-idc.h>
#include <adsp_memory.h>
#include <adsp_shim.h>
#include <zephyr/irq.h>
#include <zephyr/pm/pm.h>
#include <zephyr/cache.h>

/* IDC power up message to the ROM firmware.  This isn't documented
 * anywhere, it's basically just a magic number (except the high bit,
 * which signals the hardware)
 */
#define IDC_MSG_POWER_UP				  \
	(BIT(31) |     /* Latch interrupt in ITC write */ \
	 (0x1 << 24) | /* "ROM control version" = 1 */	  \
	 (0x2 << 0))   /* "Core wake version" = 2 */

#define IDC_CORE_MASK(num_cpus) (BIT(num_cpus) - 1)

__imr void soc_mp_startup(uint32_t cpu)
{
	/* We got here via an IDC interrupt.  Clear the TFC high bit
	 * (by writing a one!) to acknowledge and clear the latched
	 * hardware interrupt (so we don't have to service it as a
	 * spurious IPI when we enter user code).  Remember: this
	 * could have come from any core, clear all of them.
	 */
	unsigned int num_cpus = arch_num_cpus();

	for (int i = 0; i < num_cpus; i++) {
		IDC[cpu].core[i].tfc = BIT(31);
	}

	/* Interrupt must be enabled while running on current core */
	irq_enable(DT_IRQN(INTEL_ADSP_IDC_DTNODE));

}

void soc_start_core(int cpu_num)
{
	uint32_t curr_cpu = arch_proc_id();

	/* On cAVS v2.5, MP startup works differently.  The core has
	 * no ROM, and starts running immediately upon receipt of an
	 * IDC interrupt at the start of LPSRAM at 0xbe800000.  Note
	 * that means we don't need to bother constructing a "message"
	 * below, it will be ignored.  But it's left in place for
	 * simplicity and compatibility.
	 *
	 * All we need to do is place a single jump at that address to
	 * our existing MP entry point.  Unfortunately Xtensa makes
	 * this difficult, as the region is beyond the range of a
	 * relative jump instruction, so we need an immediate, which
	 * can only be backwards-referenced.  So we hand-assemble a
	 * tiny trampoline here ("jump over the immediate address,
	 * load it, jump to it").
	 *
	 * Long term we want to have this in linkable LP-SRAM memory
	 * such that the standard system bootstrap out of IMR can
	 * place it there.  But this is fine for now.
	 */
	void **lpsram = sys_cache_uncached_ptr_get(
			(__sparse_force void __sparse_cache *)LP_SRAM_BASE);
	uint8_t tramp[] = {
		0x06, 0x01, 0x00, /* J <PC+8>  (jump to L32R) */
		0,                /* (padding to align entry_addr) */
		0, 0, 0, 0,       /* (entry_addr goes here) */
		0x01, 0xff, 0xff, /* L32R a0, <entry_addr> */
		0xa0, 0x00, 0x00, /* JX a0 */
	};

	memcpy(lpsram, tramp, ARRAY_SIZE(tramp));
#if CONFIG_PM
	extern void dsp_restore_vector(void);

	/* We need to find out what type of booting is taking place here. Secondary cores
	 * can be disabled and enabled multiple times during runtime. During kernel
	 * initialization, the next pm state is set to ACTIVE. This way we can determine
	 * whether the core is being turned on again or for the first time.
	 */
	if (pm_state_next_get(cpu_num)->state == PM_STATE_ACTIVE)
		lpsram[1] = z_soc_mp_asm_entry;
	else
		lpsram[1] = dsp_restore_vector;
#else
	lpsram[1] = z_soc_mp_asm_entry;
#endif


	/* Disable automatic power and clock gating for that CPU, so
	 * it won't just go back to sleep.  Note that after startup,
	 * the cores are NOT power gated even if they're configured to
	 * be, so by default a core will launch successfully but then
	 * turn itself off when it gets to the WAITI instruction in
	 * the idle thread.
	 */
	CAVS_SHIM.clkctl |= CAVS_CLKCTL_TCPLCG(cpu_num);
	CAVS_SHIM.pwrctl |= CAVS_PWRCTL_TCPDSPPG(cpu_num);

	/* We set the interrupt controller up already, but the ROM on
	 * some platforms will mess it up.
	 */
	CAVS_INTCTRL[cpu_num].l2.clear = CAVS_L2_IDC;
	unsigned int num_cpus = arch_num_cpus();

	for (int c = 0; c < num_cpus; c++) {
		IDC[c].busy_int |= IDC_CORE_MASK(num_cpus);
	}

	/* Send power-up message to the other core.  Start address
	 * gets passed via the IETC scratch register (only 30 bits
	 * available, so it's sent shifted).  The write to ITC
	 * triggers the interrupt, so that comes last.
	 */
	uint32_t ietc = ((long)lpsram[1]) >> 2;

	IDC[curr_cpu].core[cpu_num].ietc = ietc;
	IDC[curr_cpu].core[cpu_num].itc = IDC_MSG_POWER_UP;
}

void arch_sched_ipi(void)
{
	uint32_t curr = arch_proc_id();
	unsigned int num_cpus = arch_num_cpus();

	for (int c = 0; c < num_cpus; c++) {
		if (c != curr && soc_cpus_active[c]) {
			IDC[curr].core[c].itc = BIT(31);
		}
	}
}

void idc_isr(const void *param)
{
	ARG_UNUSED(param);

#ifdef CONFIG_SMP
	/* Right now this interrupt is only used for IPIs */
	z_sched_ipi();
#endif

	/* ACK the interrupt to all the possible sources.  This is a
	 * level-sensitive interrupt triggered by a logical OR of each
	 * of the ITC/TFC high bits, INCLUDING the one "from this
	 * CPU".
	 */

	unsigned int num_cpus = arch_num_cpus();

	for (int i = 0; i < num_cpus; i++) {
		IDC[arch_proc_id()].core[i].tfc = BIT(31);
	}
}

__imr void soc_mp_init(void)
{
	IRQ_CONNECT(DT_IRQN(INTEL_ADSP_IDC_DTNODE), 0, idc_isr, NULL, 0);

	/* Every CPU should be able to receive an IDC interrupt from
	 * every other CPU, but not to be back-interrupted when the
	 * target core clears the busy bit.
	 */
	unsigned int num_cpus = arch_num_cpus();

	for (int core = 0; core < num_cpus; core++) {
		IDC[core].busy_int |= IDC_CORE_MASK(num_cpus);
		IDC[core].done_int &= ~IDC_CORE_MASK(num_cpus);

		/* Also unmask the IDC interrupt for every core in the
		 * L2 mask register.
		 */
		CAVS_INTCTRL[core].l2.clear = CAVS_L2_IDC;
	}

	/* Clear out any existing pending interrupts that might be present */
	for (int i = 0; i < num_cpus; i++) {
		for (int j = 0; j < num_cpus; j++) {
			IDC[i].core[j].tfc = BIT(31);
		}
	}

	soc_cpus_active[0] = true;
}

int soc_adsp_halt_cpu(int id)
{
	unsigned int irq_mask;

	if (id == 0 || id == arch_curr_cpu()->id) {
		return -EINVAL;
	}

	irq_mask = CAVS_L2_IDC;

#ifdef CONFIG_INTEL_ADSP_TIMER
	/*
	 * Mask timer interrupt for this CPU so it won't wake up
	 * by itself once WFI (wait for interrupt) instruction
	 * runs.
	 */
	irq_mask |= CAVS_L2_DWCT0;
#endif

	CAVS_INTCTRL[id].l2.set = irq_mask;

	/* Stop sending IPIs to this core */
	soc_cpus_active[id] = false;

	/* Turn off the "prevent power/clock gating" bits, enabling
	 * low power idle
	 */
	CAVS_SHIM.pwrctl &= ~CAVS_PWRCTL_TCPDSPPG(id);
	CAVS_SHIM.clkctl &= ~CAVS_CLKCTL_TCPLCG(id);

	/* If possible, wait for the other CPU to reach an idle state
	 * before returning.  On older hardware this doesn't work
	 * because power is controlled by the host, so synchronization
	 * needs to be part of the application layer.
	 */
	while ((CAVS_SHIM.pwrsts & CAVS_PWRSTS_PDSPPGS(id))) {
	}
	return 0;
}
