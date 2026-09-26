/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/irq.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/interrupt_controller/gic.h>

/*
 * Verifies arm_gic_irq_set_affinity(): that an SPI can be explicitly routed
 * to an arbitrary core (not just the core that happened to call
 * arm_gic_irq_enable()), analogous to k_thread_cpu_pin() for threads.
 *
 * No real peripheral is needed: arm_gic_irq_set_pending() injects a
 * software-pending SPI directly at the distributor, which the routed-to
 * core's ISR then observes exactly like a real hardware interrupt would.
 */

static volatile int handler_cpu;
static struct k_sem irq_sem;

static void affinity_test_isr(const void *arg)
{
	ARG_UNUSED(arg);

	handler_cpu = arch_curr_cpu()->id;
	k_sem_give(&irq_sem);
}

/*
 * Read a target CPU's MPIDR affinity value using only existing mechanisms:
 * pin a helper thread to that CPU and let it read its own MPIDR via
 * MPIDR_TO_CORE(GET_MPIDR()) - the same value arm_gic_irq_set_affinity()
 * expects - so the test needs no dedicated arch MPIDR-lookup API.
 */
#define WORKER_STACK_SIZE 1024

static K_THREAD_STACK_DEFINE(worker_stack, WORKER_STACK_SIZE);
static struct k_thread worker_thread;
static volatile uint64_t worker_mpidr;

static void mpidr_reader(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	worker_mpidr = MPIDR_TO_CORE(GET_MPIDR());
}

static uint64_t mpidr_of_cpu(int cpu)
{
	worker_mpidr = UINT64_MAX;

	k_thread_create(&worker_thread, worker_stack, WORKER_STACK_SIZE,
			mpidr_reader, NULL, NULL, NULL,
			K_PRIO_COOP(0), 0, K_FOREVER);
	k_thread_cpu_pin(&worker_thread, cpu);
	k_thread_start(&worker_thread);
	k_thread_join(&worker_thread, K_FOREVER);

	return worker_mpidr;
}

/* Find an SPI line implemented by this GIC but not currently in use by
 * Zephyr, mirroring the same approach
 * tests/arch/arm/arm_irq_advanced_features uses for the NVIC.
 */
static int find_unused_spi(void)
{
	for (int irq = CONFIG_NUM_IRQS - 1; irq >= GIC_SPI_INT_BASE; irq--) {
		if (!arm_gic_irq_is_enabled(irq)) {
			return irq;
		}
	}

	return -1;
}

ZTEST(arm64_irq_affinity, test_irq_affinity)
{
	int irq = find_unused_spi();

	zassert_true(irq >= 0, "No free SPI line available to test with");

	printk("irq_affinity: using SPI %d, testing on this build's %d CPUs\n",
	       irq, arch_num_cpus());

	k_sem_init(&irq_sem, 0, 1);
	irq_connect_dynamic(irq, IRQ_DEFAULT_PRIORITY, affinity_test_isr, NULL, 0);

	int num_cpus = arch_num_cpus();

	for (int target = 0; target < num_cpus; target++) {
		uint64_t target_mpidr = mpidr_of_cpu(target);

		zassert_not_equal(target_mpidr, UINT64_MAX,
				   "CPU %d was never brought up", target);

		handler_cpu = -1;

		printk("irq_affinity: routing IRQ %d to CPU %d (mpidr 0x%llx)...\n",
		       irq, target, target_mpidr);

		/*
		 * irq_enable()'s own implicit self-affinity (when built with
		 * CONFIG_ARMV8_A_NS/CONFIG_GIC_SINGLE_SECURITY_STATE) targets
		 * whichever core calls it, i.e. this test thread's own core -
		 * arm_gic_irq_set_affinity() must run *after* it so our
		 * explicit target is the one that sticks.
		 */
		irq_enable(irq);
		arm_gic_irq_set_affinity(irq, target_mpidr);

		arm_gic_irq_set_pending(irq);

		zassert_equal(k_sem_take(&irq_sem, K_MSEC(100)), 0,
			      "IRQ %d never fired when routed to CPU %d", irq, target);
		zassert_equal(handler_cpu, target,
			      "IRQ %d routed to CPU %d but ran on CPU %d",
			      irq, target, handler_cpu);

		printk("irq_affinity: IRQ %d routed to CPU %d -> handled on CPU %d (%s)\n",
		       irq, target, handler_cpu, (handler_cpu == target) ? "MATCH" : "MISMATCH");

		irq_disable(irq);
	}
}

ZTEST_SUITE(arm64_irq_affinity, NULL, NULL, NULL, NULL, NULL);
