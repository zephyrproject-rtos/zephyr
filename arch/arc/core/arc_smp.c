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
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <ksched.h>
#include <zephyr/init.h>

#define MP_PRIMARY_CPU_ID 0

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
	arc_cpu_sp = Z_KERNEL_STACK_BUFFER(stack) + sz;

	arc_cpu_wake_flag = cpu_num;

	/* wait slave cpu to start */
	while (arc_cpu_wake_flag != 0U) {
		;
	}
}

#ifdef CONFIG_SMP
static void arc_connect_debug_mask_update(int cpu_num)
{
	uint32_t core_mask = 1 << cpu_num;

	/*
	 * MDB debugger may modify debug_select and debug_mask registers on start, so we can't
	 * rely on debug_select reset value.
	 */
	if (cpu_num != MP_PRIMARY_CPU_ID) {
		core_mask |= z_arc_connect_debug_select_read();
	}

	z_arc_connect_debug_select_set(core_mask);
	/* Debugger halts cores at all conditions:
	 * ARC_CONNECT_CMD_DEBUG_MASK_H: Core global halt.
	 * ARC_CONNECT_CMD_DEBUG_MASK_AH: Actionpoint halt.
	 * ARC_CONNECT_CMD_DEBUG_MASK_BH: Software breakpoint halt.
	 * ARC_CONNECT_CMD_DEBUG_MASK_SH: Self halt.
	 */
	z_arc_connect_debug_mask_set(core_mask,	(ARC_CONNECT_CMD_DEBUG_MASK_SH
		| ARC_CONNECT_CMD_DEBUG_MASK_BH | ARC_CONNECT_CMD_DEBUG_MASK_AH
		| ARC_CONNECT_CMD_DEBUG_MASK_H));
}
#endif

/* the C entry of slave cores */
void z_arc_slave_start(int cpu_num)
{
	arch_cpustart_t fn;

#ifdef CONFIG_SMP
	struct arc_connect_bcr bcr;

	bcr.val = z_arc_v2_aux_reg_read(_ARC_V2_CONNECT_BCR);

	if (bcr.dbg) {
		/* configure inter-core debug unit if available */
		arc_connect_debug_mask_update(cpu_num);
	}

	z_irq_setup();

	z_arc_connect_ici_clear();
	z_irq_priority_set(DT_IRQN(DT_NODELABEL(ici)),
			   DT_IRQ(DT_NODELABEL(ici), priority), 0);
	irq_enable(DT_IRQN(DT_NODELABEL(ici)));
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
	for (i = 0U; i < CONFIG_MP_NUM_CPUS; i++) {
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

	if (bcr.dbg) {
		/* configure inter-core debug unit if available */
		arc_connect_debug_mask_update(MP_PRIMARY_CPU_ID);
	}

	if (bcr.ipi) {
	/* register ici interrupt, just need master core to register once */
		z_arc_connect_ici_clear();
		IRQ_CONNECT(DT_IRQN(DT_NODELABEL(ici)),
			    DT_IRQ(DT_NODELABEL(ici), priority),
			    sched_ipi_handler, NULL, 0);

		irq_enable(DT_IRQN(DT_NODELABEL(ici)));
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
