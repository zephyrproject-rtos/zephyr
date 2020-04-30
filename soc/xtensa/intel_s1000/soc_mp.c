/*
 * Copyright (c) 2018-2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <sys/sys_io.h>
#include <sys/__assert.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(soc_mp, CONFIG_SOC_LOG_LEVEL);

#include "soc.h"
#include "memory.h"

#ifdef CONFIG_SCHED_IPI_SUPPORTED
#include <drivers/ipm.h>
#include <ipm/ipm_cavs_idc.h>

static const struct device *idc;
#endif
extern void __start(void);

struct cpustart_rec {
	uint32_t		cpu;
	arch_cpustart_t	fn;
	char		*stack_top;
	void		*arg;
	uint32_t		vecbase;
	uint32_t		alive;
	/* padding to cache line */
	uint8_t		padding[XCHAL_DCACHE_LINESIZE - 6 * 4];
};

static __aligned(XCHAL_DCACHE_LINESIZE)
struct cpustart_rec start_rec;

static void *mp_top;

static void mp_entry2(void)
{
	volatile int ps, ie;

	/* Copy over VECBASE from the main CPU for an initial value
	 * (will need to revisit this if we ever allow a user API to
	 * change interrupt vectors at runtime).  Make sure interrupts
	 * are locally disabled, then synthesize a PS value that will
	 * enable them for the user code to pass to irq_unlock()
	 * later.
	 */
	__asm__ volatile("rsr.PS %0" : "=r"(ps));
	ps &= ~(PS_EXCM_MASK | PS_INTLEVEL_MASK);
	__asm__ volatile("wsr.PS %0" : : "r"(ps));

	ie = 0;
	__asm__ volatile("wsr.INTENABLE %0" : : "r"(ie));
	__asm__ volatile("wsr.VECBASE %0" : : "r"(start_rec.vecbase));
	__asm__ volatile("rsync");

	/* Set up the CPU pointer. */
	_cpu_t *cpu = &_kernel.cpus[start_rec.cpu];

	__asm__ volatile(
		"wsr." CONFIG_XTENSA_KERNEL_CPU_PTR_SR " %0" : : "r"(cpu));

#ifdef CONFIG_IPM_CAVS_IDC
	/* Interrupt must be enabled while running on current core */
	irq_enable(XTENSA_IRQ_NUMBER(DT_IRQN(DT_INST(0, intel_cavs_idc))));
#endif /* CONFIG_IPM_CAVS_IDC */

	start_rec.alive = 1;
	SOC_DCACHE_FLUSH(&start_rec, sizeof(start_rec));

	start_rec.fn(start_rec.arg);

#if CONFIG_MP_NUM_CPUS == 1
	/* CPU#1 can be under manual control running custom functions
	 * instead of participating in general thread execution.
	 * Put the CPU into idle after those functions return
	 * so this won't return.
	 */
	for (;;) {
		k_cpu_idle();
	}
#endif
}

/* Defines a locally callable "function" named mp_stack_switch().  The
 * first argument (in register a2 post-ENTRY) is the new stack pointer
 * to go into register a1.  The second (a3) is the entry point.
 * Because this never returns, a0 is used as a scratch register then
 * set to zero for the called function (a null return value is the
 * signal for "top of stack" to the debugger).
 */
void mp_stack_switch(void *stack, void *entry);
__asm__("\n"
	".align 4		\n"
	"mp_stack_switch:	\n\t"

	"entry a1, 16		\n\t"

	"movi a0, 0		\n\t"

	"jx a3			\n\t");

/* Carefully constructed to use no stack beyond compiler-generated ABI
 * instructions. Stack pointer is pointing to __stack at this point.
 */
void z_mp_entry(void)
{
	mp_stack_switch(mp_top, mp_entry2);
}

void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	volatile struct soc_dsp_shim_regs *dsp_shim_regs =
		(volatile struct soc_dsp_shim_regs *)SOC_DSP_SHIM_REG_BASE;
	volatile struct soc_global_regs *soc_glb_regs =
		(volatile struct soc_global_regs *)SOC_S1000_GLB_CTRL_BASE;
	uint32_t vecbase;

	__ASSERT(cpu_num == 1, "Intel S1000 supports only two CPUs!");

	/* Setup data to boot core #1 */
	__asm__ volatile("rsr.VECBASE %0\n\t" : "=r"(vecbase));

	start_rec.cpu = cpu_num;
	start_rec.fn = fn;
	start_rec.stack_top = Z_THREAD_STACK_BUFFER(stack) + sz;
	start_rec.arg = arg;
	start_rec.vecbase = vecbase;
	start_rec.alive = 0;

	mp_top = Z_THREAD_STACK_BUFFER(stack) + sz;

	SOC_DCACHE_FLUSH(&start_rec, sizeof(start_rec));

#ifdef CONFIG_SCHED_IPI_SUPPORTED
	idc = device_get_binding(DT_LABEL(DT_INST(0, intel_cavs_idc)));
#endif

	/*
	 * SoC Boot ROM has hard-coded address for boot vector in LP-SRAM,
	 * and will jump unconditionally to it. So power up the LP-SRAM
	 * and set the vector.
	 */
	sys_write32(0x0, SOC_L2RAM_LOCAL_MEM_REG_LSPGCTL);
	*((uint32_t *)LPSRAM_BOOT_VECTOR_ADDR) = (uint32_t)__start;

	/* Disable power gating for DSP core #cpu_num */
	dsp_shim_regs->pwrctl |= SOC_PWRCTL_DISABLE_PWR_GATING_DSP1;

	/*
	 * Since we do not know the status of the core,
	 * power it down and force it into reset and stall.
	 */
	soc_glb_regs->cavs_dsp1power_control |=
		SOC_S1000_GLB_CTRL_DSP1_PWRCTL_CRST |
		SOC_S1000_GLB_CTRL_DSP1_PWRCTL_CSTALL;

	soc_glb_regs->cavs_dsp1power_control &=
		~SOC_S1000_GLB_CTRL_DSP1_PWRCTL_SPA;

	/* Wait for core power down */
	while ((soc_glb_regs->cavs_dsp1power_control &
		SOC_S1000_GLB_CTRL_DSP1_PWRCTL_CPA) != 0) {
	};

	/* Now power up the core */
	soc_glb_regs->cavs_dsp1power_control |=
		SOC_S1000_GLB_CTRL_DSP1_PWRCTL_SPA;

	/* Wait for core power up*/
	while ((soc_glb_regs->cavs_dsp1power_control &
		SOC_S1000_GLB_CTRL_DSP1_PWRCTL_CPA) == 0) {
	};

	/* Then step out of reset, and un-stall */
	soc_glb_regs->cavs_dsp1power_control &=
		~SOC_S1000_GLB_CTRL_DSP1_PWRCTL_CRST;

	soc_glb_regs->cavs_dsp1power_control &=
		~SOC_S1000_GLB_CTRL_DSP1_PWRCTL_CSTALL;

	do {
		SOC_DCACHE_INVALIDATE(&start_rec, sizeof(start_rec));
	} while (start_rec.alive == 0);
}

#ifdef CONFIG_SCHED_IPI_SUPPORTED
FUNC_ALIAS(soc_sched_ipi, arch_sched_ipi, void);
void soc_sched_ipi(void)
{
	if (likely(idc != NULL)) {
		ipm_send(idc, 0, IPM_CAVS_IDC_MSG_SCHED_IPI_ID,
			 IPM_CAVS_IDC_MSG_SCHED_IPI_DATA, 0);
	}
}
#endif
