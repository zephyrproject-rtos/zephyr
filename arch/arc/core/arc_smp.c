/*
 * Copyright (c) 2019 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief codes required for ARC multicore and Zephyr smp support
 *
 */
#include <device.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <ksched.h>
#include <soc.h>
#include <init.h>


#ifndef IRQ_ICI
#define IRQ_ICI 19
#endif

#define ARCV2_ICI_IRQ_PRIORITY 1

volatile struct {
	arch_cpustart_t fn;
	void *arg;
} arc_cpu_init[CONFIG_MP_NUM_CPUS];

/*
 * arc_cpu_wake_flag is used to sync up master core and slave cores
 * Slave core will spin for arc_cpu_wake_flag until master core sets
 * it to the core id of slave core. Then, slave core clears it to notify
 * master core that it's waken
 *
 */
volatile uint32_t arc_cpu_wake_flag;

volatile char *arc_cpu_sp;
/*
 * _curr_cpu is used to record the struct of _cpu_t of each cpu.
 * for efficient usage in assembly
 */
volatile _cpu_t *_curr_cpu[CONFIG_MP_NUM_CPUS];

/* Called from Zephyr initialization */
void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	_curr_cpu[cpu_num] = &(_kernel.cpus[cpu_num]);
	arc_cpu_init[cpu_num].fn = fn;
	arc_cpu_init[cpu_num].arg = arg;

	/* set the initial sp of target sp through arc_cpu_sp
	 * arc_cpu_wake_flag will protect arc_cpu_sp that
	 * only one slave cpu can read it per time
	 */
	arc_cpu_sp = Z_THREAD_STACK_BUFFER(stack) + sz;

	arc_cpu_wake_flag = cpu_num;

	/* wait slave cpu to start */
	while (arc_cpu_wake_flag != 0) {
		;
	}
}

/* the C entry of slave cores */
void z_arc_slave_start(int cpu_num)
{
	arch_cpustart_t fn;

#ifdef CONFIG_SMP
	z_icache_setup();
	z_irq_setup();

	z_arc_connect_ici_clear();
	z_irq_priority_set(IRQ_ICI, ARCV2_ICI_IRQ_PRIORITY, 0);
	irq_enable(IRQ_ICI);
#endif
	/* call the function set by arch_start_cpu */
	fn = arc_cpu_init[cpu_num].fn;

	fn(arc_cpu_init[cpu_num].arg);
}

#ifdef CONFIG_SMP

static void sched_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	z_arc_connect_ici_clear();
	z_sched_ipi();
}

/* arch implementation of sched_ipi */
void arch_sched_ipi(void)
{
	uint32_t i;

	/* broadcast sched_ipi request to other cores
	 * if the target is current core, hardware will ignore it
	 */
	for (i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		z_arc_connect_ici_generate(i);
	}
}

static int arc_smp_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	struct arc_connect_bcr bcr;

	/* necessary master core init */
	_curr_cpu[0] = &(_kernel.cpus[0]);

	bcr.val = z_arc_v2_aux_reg_read(_ARC_V2_CONNECT_BCR);

	if (bcr.ipi) {
	/* register ici interrupt, just need master core to register once */
		z_arc_connect_ici_clear();
		IRQ_CONNECT(IRQ_ICI, ARCV2_ICI_IRQ_PRIORITY,
		    sched_ipi_handler, NULL, 0);

		irq_enable(IRQ_ICI);
	} else {
		__ASSERT(0,
			"ARC connect has no inter-core interrupt\n");
		return -ENODEV;
	}

	if (bcr.gfrc) {
		/* global free running count init */
		z_arc_connect_gfrc_enable();

		/* when all cores halt, gfrc halt */
		z_arc_connect_gfrc_core_set((1 << CONFIG_MP_NUM_CPUS) - 1);
		z_arc_connect_gfrc_clear();
	} else {
		__ASSERT(0,
			"ARC connect has no global free running counter\n");
		return -ENODEV;
	}

	return 0;
}

SYS_INIT(arc_smp_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
