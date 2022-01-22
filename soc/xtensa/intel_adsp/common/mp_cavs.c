/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <cavs-idc.h>
#include <cavs-mem.h>
#include <cavs-shim.h>

/* IDC power up message to the ROM firmware.  This isn't documented
 * anywhere, it's basically just a magic number (except the high bit,
 * which signals the hardware)
 */
#define IDC_MSG_POWER_UP				  \
	(BIT(31) |     /* Latch interrupt in ITC write */ \
	 (0x1 << 24) | /* "ROM control version" = 1 */	  \
	 (0x2 << 0))   /* "Core wake version" = 2 */

#define IDC_ALL_CORES (BIT(CONFIG_MP_NUM_CPUS) - 1)

#define CAVS15_ROM_IDC_DELAY 500

extern void z_sched_ipi(void);
extern void z_reinit_idle_thread(int i);
extern void z_smp_start_cpu(int id);

static struct k_spinlock mplock;

__imr void soc_mp_startup(uint32_t cpu)
{
	/* We got here via an IDC interrupt.  Clear the TFC high bit
	 * (by writing a one!) to acknowledge and clear the latched
	 * hardware interrupt (so we don't have to service it as a
	 * spurious IPI when we enter user code).  Remember: this
	 * could have come from any core, clear all of them.
	 */
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		IDC[cpu].core[i].tfc = BIT(31);
	}

	/* Interrupt must be enabled while running on current core */
	irq_enable(DT_IRQN(DT_INST(0, intel_cavs_idc)));

	/* Unfortunately the interrupt controller doesn't understand
	 * that each CPU has its own mask register (the timer has a
	 * similar hook).  Needed only on hardware with ROMs that
	 * disable this; otherwise our own code in soc_idc_init()
	 * already has it unmasked.
	 */
	if (!IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V25)) {
		CAVS_INTCTRL[cpu].l2.clear = CAVS_L2_IDC;
	}
}

static ALWAYS_INLINE uint32_t prid(void)
{
	uint32_t prid;

	__asm__ volatile("rsr %0, PRID" : "=r"(prid));
	return prid;
}

void soc_start_core(int cpu_num)
{
	uint32_t curr_cpu = prid();

#ifdef CONFIG_SOC_SERIES_INTEL_CAVS_V25
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
	void **lpsram = z_soc_uncached_ptr((void *)LP_SRAM_BASE);
	uint8_t tramp[] = {
		0x06, 0x01, 0x00, /* J <PC+8>  (jump to L32R) */
		0,                /* (padding to align entry_addr) */
		0, 0, 0, 0,       /* (entry_addr goes here) */
		0x01, 0xff, 0xff, /* L32R a0, <entry_addr> */
		0xa0, 0x00, 0x00, /* JX a0 */
	};

	memcpy(lpsram, tramp, ARRAY_SIZE(tramp));
	lpsram[1] = z_soc_mp_asm_entry;
#endif

	/* Disable automatic power and clock gating for that CPU, so
	 * it won't just go back to sleep.  Note that after startup,
	 * the cores are NOT power gated even if they're configured to
	 * be, so by default a core will launch successfully but then
	 * turn itself off when it gets to the WAITI instruction in
	 * the idle thread.
	 */
	if (!IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V15)) {
		CAVS_SHIM.clkctl |= CAVS_CLKCTL_TCPLCG(cpu_num);
	}
	CAVS_SHIM.pwrctl |= CAVS_PWRCTL_TCPDSPPG(cpu_num);

	/* Older devices boot from a ROM and needs some time to
	 * complete initialization and be waiting for the IDC we're
	 * about to send.
	 */
	if (!IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V25)) {
		k_busy_wait(CAVS15_ROM_IDC_DELAY);
	}

	/* We set the interrupt controller up already, but the ROM on
	 * some platforms will mess it up.
	 */
	CAVS_INTCTRL[cpu_num].l2.clear = CAVS_L2_IDC;
	for (int c = 0; c < CONFIG_MP_NUM_CPUS; c++) {
		IDC[c].busy_int |= IDC_ALL_CORES;
	}

	/* Send power-up message to the other core.  Start address
	 * gets passed via the IETC scratch register (only 30 bits
	 * available, so it's sent shifted).  The write to ITC
	 * triggers the interrupt, so that comes last.
	 */
	uint32_t ietc = ((long) z_soc_mp_asm_entry) >> 2;

	IDC[curr_cpu].core[cpu_num].ietc = ietc;
	IDC[curr_cpu].core[cpu_num].itc = IDC_MSG_POWER_UP;
}

void arch_sched_ipi(void)
{
	uint32_t curr = prid();

	for (int c = 0; c < CONFIG_MP_NUM_CPUS; c++) {
		if (c != curr && soc_cpus_active[c]) {
			IDC[curr].core[c].itc = BIT(31);
		}
	}
}

void idc_isr(void *param)
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
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		IDC[prid()].core[i].tfc = BIT(31);
	}
}

__imr void soc_mp_init(void)
{
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(idc)), 0, idc_isr, NULL, 0);

	/* Every CPU should be able to receive an IDC interrupt from
	 * every other CPU, but not to be back-interrupted when the
	 * target core clears the busy bit.
	 */
	for (int core = 0; core < CONFIG_MP_NUM_CPUS; core++) {
		IDC[core].busy_int |= IDC_ALL_CORES;
		IDC[core].done_int &= ~IDC_ALL_CORES;

		/* Also unmask the IDC interrupt for every core in the
		 * L2 mask register.
		 */
		CAVS_INTCTRL[core].l2.clear = CAVS_L2_IDC;
	}

	/* Clear out any existing pending interrupts that might be present */
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		for (int j = 0; j < CONFIG_MP_NUM_CPUS; j++) {
			IDC[i].core[j].tfc = BIT(31);
		}
	}

	soc_cpus_active[0] = true;
}

/**
 * @brief Restart halted SMP CPU
 *
 * Relaunches a CPU that has entered an idle power state via
 * soc_halt_cpu().  Returns -EINVAL if the CPU is not in a power-gated
 * idle state.  Upon successful return, the CPU is online and
 * available to run any Zephyr thread.
 *
 * @param id CPU to start, in the range [1:CONFIG_MP_NUM_CPUS)
 */
__imr int soc_relaunch_cpu(int id)
{
	int ret = 0;
	k_spinlock_key_t k = k_spin_lock(&mplock);

	if (id < 1 || id >= CONFIG_MP_NUM_CPUS) {
		ret = -EINVAL;
		goto out;
	}

	if (CAVS_SHIM.pwrsts & CAVS_PWRSTS_PDSPPGS(id)) {
		ret = -EINVAL;
		goto out;
	}

	__ASSERT_NO_MSG(!soc_cpus_active[id]);

	CAVS_INTCTRL[id].l2.clear = CAVS_L2_IDC;
	z_reinit_idle_thread(id);
	z_smp_start_cpu(id);

 out:
	k_spin_unlock(&mplock, k);
	return ret;
}

/**
 * @brief Halts and offlines a running CPU
 *
 * Enables power gating on the specified CPU, which cannot be the
 * current CPU or CPU 0.  The CPU must be idle; no application threads
 * may be runnable on it when this function is called (or at least the
 * CPU must be guaranteed to reach idle in finite time without
 * deadlock).  Actual CPU shutdown can only happen in the context of
 * the idle thread, and synchronization is an application
 * responsibility.  This function will hang if the other CPU fails to
 * reach idle.
 *
 * @param id CPU to halt, not current cpu or cpu 0
 * @return 0 on success, -EINVAL on error
 */
__imr int soc_halt_cpu(int id)
{
	int ret = 0;
	k_spinlock_key_t k = k_spin_lock(&mplock);

	if (id == 0 || id == _current_cpu->id) {
		ret = -EINVAL;
		goto out;
	}

	/* Stop sending IPIs to this core */
	soc_cpus_active[id] = false;

	/* Turn off the "prevent power/clock gating" bits, enabling
	 * low power idle
	 */
	CAVS_SHIM.pwrctl &= ~CAVS_PWRCTL_TCPDSPPG(id);
	CAVS_SHIM.clkctl &= ~CAVS_CLKCTL_TCPLCG(id);

	/* Wait for the CPU to reach an idle state before returing */
	while (CAVS_SHIM.pwrsts & CAVS_PWRSTS_PDSPPGS(id)) {
	}

 out:
	k_spin_unlock(&mplock, k);
	return ret;
}
