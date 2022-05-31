/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <ksched.h>
#include <soc.h>
#include <rv_smp_defs.h>
volatile struct {
	arch_cpustart_t fn;
	void *arg;
} riscv_cpu_init[CONFIG_MP_NUM_CPUS];

/*
 * Collection of flags to control wake up of harts. This is trickier than
 * expected due to the fact that the wfi can be triggered when in the
 * debugger so we have to stage things carefully to ensure we only wake
 * up at the correct time.
 *
 * initial implementation which assumes any monitor hart is hart id 0 and
 * SMP harts have contiguous hart IDs. CONFIG_SMP_BASE_CPU will have minimum
 * value of 1 for systems with monitor hart and zero otherwise.
 */

#if (CONFIG_MP_TOTAL_NUM_CPUS > CONFIG_MP_NUM_CPUS)
	#define WAKE_FLAG_COUNT (CONFIG_MP_TOTAL_NUM_CPUS)
#else
	#define WAKE_FLAG_COUNT (CONFIG_MP_NUM_CPUS)
#endif

/* we will index directly off of mhartid so need to be careful... */
volatile __noinit ulong_t hart_wake_flags[WAKE_FLAG_COUNT];
volatile void *riscv_cpu_sp;
/*
 * _curr_cpu is used to record the struct of _cpu_t of each cpu.
 * for efficient usage in assembly
 */
volatile _cpu_t *_curr_cpu[CONFIG_MP_NUM_CPUS];

void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	int hart_num = cpu_num + CONFIG_SMP_BASE_CPU;

	/* Used to avoid empty loops which can cause debugger issues
	 * and also for retry count on interrupt to keep sending every now and again...
	 */
	volatile int counter = 0;

	_curr_cpu[cpu_num] = &(_kernel.cpus[cpu_num]);
	riscv_cpu_init[cpu_num].fn = fn;
	riscv_cpu_init[cpu_num].arg = arg;

	/* set the initial sp of target sp through riscv_cpu_sp
	 * Controlled sequencing of start up will ensure
	 * only one secondary cpu can read it per time
	 */
	riscv_cpu_sp = Z_THREAD_STACK_BUFFER(stack) + sz;

	/* wait slave cpu to start */
	while (hart_wake_flags[hart_num] != RV_WAKE_WAIT) {
		counter++;
	}

	hart_wake_flags[hart_num] = RV_WAKE_GO;

	/*raise soft interrupt for hart(x) where x== hart ID*/
	RISCV_CLINT->MSIP[hart_num] = 0x01U;

	while (hart_wake_flags[hart_num] != RV_WAKE_DONE) {
		counter++;
		if (0 == (counter % 64)) {
			RISCV_CLINT->MSIP[hart_num] = 0x01U;	/* Another nudge... */
	}
	}

	RISCV_CLINT->MSIP[hart_num] = 0x00U;	/* Clear int now we are done */
}

void z_riscv_secondary_cpu_init(int cpu_num)
{
#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
	soc_interrupt_init();
#endif
#ifdef CONFIG_RISCV_PMP
	z_riscv_pmp_init();
#endif
#if defined(CONFIG_SCHED_IPI_SUPPORTED)
	irq_enable(RISCV_MACHINE_SOFT_IRQ);
#endif
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	__asm__("mv tp, %0" : : "r" (z_idle_threads[cpu_num].tls));
#endif
	riscv_cpu_init[cpu_num].fn(riscv_cpu_init[cpu_num].arg);
}
/*
{
#ifdef CONFIG_64BIT
	return (uintptr_t *)(uint64_t)(RISCV_MSIP_BASE + (hart_id * 4));
#else
	return (uintptr_t *)(RISCV_MSIP_BASE + (hart_id * 4));
#endif
}
*/
#ifdef CONFIG_SMP
void arch_sched_ipi(void)
{
	unsigned int key;
	ulong_t i;

	key = arch_irq_lock();

	/* broadcast sched_ipi request to other cores
	 * if the target is current core, hardware will ignore it
	 */
	for (i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		/*raise soft interrupt for hart(x) where x== hart ID*/
		RISCV_CLINT->MSIP[i + CONFIG_SMP_BASE_CPU] = 0x01U;
	}

	arch_irq_unlock(key);
}

static void sched_ipi_handler(const void *unused)
{
	ulong_t hart_id;

	ARG_UNUSED(unused);
	/* Index off of hart id to select correct register */
	__asm__ volatile("csrr %0, mhartid" : "=r" (hart_id));

	/* Clear soft interrupt for hart(x) where x== hart ID*/
	RISCV_CLINT->MSIP[hart_id] = 0x00U;
	z_sched_ipi();
}

static int riscv_smp_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(RISCV_MACHINE_SOFT_IRQ, 0, sched_ipi_handler, NULL, 0);
	irq_enable(RISCV_MACHINE_SOFT_IRQ);

	return 0;
}

SYS_INIT(riscv_smp_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* CONFIG_SMP */
