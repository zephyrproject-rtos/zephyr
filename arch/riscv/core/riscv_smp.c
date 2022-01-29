/*
 * Copyright (c) 2021 Microchip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief codes required for Risc-V multicore and Zephyr smp support
 *
 */
#include <device.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <ksched.h>
#include <soc.h>
#include <init.h>
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
 * MPFS
 *
 * initial implementation which assumes there are CONFIG_MP_NUM_CPUS harts
 * which are numbered 1 to 4 as the E51 is hart 0 and we only support SMP
 * on the U54s...
 */

#if defined(CONFIG_SOC_MPFS)
/* we will index directly off of mhartid so need extra for E51 */
volatile __noinit uint64_t hart_wake_flags[5 /* CONFIG_MP_NUM_CPUS + 1*/];
#else
volatile __noinit uint64_t hart_wake_flags[CONFIG_MP_NUM_CPUS];
#endif

volatile char *riscv_cpu_sp;
/*
 * _curr_cpu is used to record the struct of _cpu_t of each cpu.
 * for efficient usage in assembly
 */
volatile _cpu_t *_curr_cpu[CONFIG_MP_NUM_CPUS];

/* Called from Zephyr initialization */
void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
#if defined(CONFIG_SOC_MPFS)
	int hart_num = cpu_num + 1;
#else
	int hart_num = cpu_num; 
#endif
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
	RISCV_CLINT->MSIP[hart_num] = 0x01U;   /*raise soft interrupt for hart(x) where x== hart ID*/

	while (hart_wake_flags[hart_num] != RV_WAKE_DONE) {
		counter++;
		if(0 == (counter % 64)) {
			RISCV_CLINT->MSIP[hart_num] = 0x01U;   /* Another nudge... */
		}
	}

    RISCV_CLINT->MSIP[hart_num] = 0x00U;   /* Clear int now we are done */
}

/* the C entry of slave cores */
void z_riscv_secondary_start(int cpu_num)
{
	arch_cpustart_t fn;
#if 0
#ifdef CONFIG_SMP
	z_icache_setup();
	z_irq_setup();

#endif
#endif
#if defined(CONFIG_SCHED_IPI_SUPPORTED)
    irq_enable(RISCV_MACHINE_SOFT_IRQ);
#endif
	/* call the function set by arch_start_cpu */
	fn = riscv_cpu_init[cpu_num].fn;

	fn(riscv_cpu_init[cpu_num].arg);
}

#if defined(CONFIG_SCHED_IPI_SUPPORTED)
static void sched_ipi_handler(const void *unused)
{
    unsigned int hart_id;

    ARG_UNUSED(unused);

    __asm__ volatile("csrr %0, mhartid" : "=r" (hart_id));
    RISCV_CLINT->MSIP[hart_id] = 0x00U;   /* Clear soft interrupt for hart(x) where x== hart ID*/
    z_sched_ipi();
}

void arch_sched_ipi(void)
{
    uint32_t i;

    /* broadcast sched_ipi request to other cores
     * if the target is current core, hardware will ignore it
     */
    for (i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
#if defined(CONFIG_SOC_MPFS)
        RISCV_CLINT->MSIP[i + 1] = 0x01U;   /*raise soft interrupt for hart(x) where x== hart ID*/
#else
        RISCV_CLINT->MSIP[i] = 0x01U;   /*raise soft interrupt for hart(x) where x== hart ID*/
#endif
    }
}

static int riscv_smp_init(const struct device *dev)
{
    ARG_UNUSED(dev);

    /*
     * Set up handler from main hart and enable IPI interrupt for it.
     * Secondary harts will just enable the interrupt as same isr table is
     * used by all...
     */
    IRQ_CONNECT(RISCV_MACHINE_SOFT_IRQ, 0, sched_ipi_handler, NULL, 0);
    irq_enable(RISCV_MACHINE_SOFT_IRQ);

    return 0;
}

SYS_INIT(riscv_smp_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif


#if 0
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

	if (bcr.dbg) {
	/* configure inter-core debug unit if available */
		uint32_t core_mask = (1 << CONFIG_MP_NUM_CPUS) - 1;
		z_arc_connect_debug_select_set(core_mask);
		/* Debugger halt cores at conditions */
		z_arc_connect_debug_mask_set(core_mask,	(ARC_CONNECT_CMD_DEBUG_MASK_SH
			| ARC_CONNECT_CMD_DEBUG_MASK_BH | ARC_CONNECT_CMD_DEBUG_MASK_AH
			| ARC_CONNECT_CMD_DEBUG_MASK_H));

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


