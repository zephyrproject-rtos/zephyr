/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if CONFIG_MP_MAX_NUM_CPUS <= 1
#error "Test requires a system with more than 1 CPU"
#endif

#define IPI_TEST_INTERVAL_DURATION 30

#define NUM_WORK_THREADS (CONFIG_MP_MAX_NUM_CPUS - 1)
#define WORK_STACK_SIZE  4096

#define PRIMITIVE_STACK_SIZE 4096

static K_THREAD_STACK_ARRAY_DEFINE(work_stack, NUM_WORK_THREADS, WORK_STACK_SIZE);
static K_THREAD_STACK_DEFINE(primitive_stack, PRIMITIVE_STACK_SIZE);

static struct k_thread work_thread[NUM_WORK_THREADS];
static unsigned long work_array[NUM_WORK_THREADS][1024];
static volatile unsigned long work_counter[NUM_WORK_THREADS];

static struct k_thread primitive_thread;
static volatile unsigned long primitives_issued;

static atomic_t ipi_cpu_bitmap;

void z_trace_sched_ipi(void)
{
	atomic_or(&ipi_cpu_bitmap, BIT(_current_cpu->id));
}

void work_entry(void *p1, void *p2, void *p3)
{
	unsigned int index = POINTER_TO_UINT(p1);
	unsigned long *array = p2;
	unsigned long counter;

	while (1) {
		for (unsigned int i = 0; i < 1024; i++) {
			counter = work_counter[index]++;

			array[i] = (array[i] + counter) ^ array[i];
		}
	}
}

void primitive_entry(void *p1, void *p2, void *p3)
{
	unsigned int desired_ipi_set;
	unsigned int value;
	int key;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/*
	 * All other CPUs are executing cooperative threads and are not
	 * expected to switch in a new thread. Select a CPU targeted for IPIs.
	 */

#ifdef CONFIG_IPI_METRIC_PRIMITIVE_DIRECTED
	key = arch_irq_lock();
	desired_ipi_set = (_current_cpu->id == 0) ? BIT(1) : BIT(0);
	arch_irq_unlock(key);
#else
	desired_ipi_set = (1 << arch_num_cpus()) - 1;
	key = arch_irq_lock();
	desired_ipi_set ^= BIT(_current_cpu->id);
	arch_irq_unlock(key);
#endif

	while (1) {
		atomic_set(&ipi_cpu_bitmap, 0);
#ifdef CONFIG_IPI_METRIC_PRIMITIVE_DIRECTED
		arch_sched_directed_ipi(desired_ipi_set);
#else
		arch_sched_broadcast_ipi();
#endif

		primitives_issued++;

		/*
		 * Loop until all the expected CPUs have flagged that they
		 * have processed the schedule IPI from above.
		 */

		while (1) {
			value = (unsigned int)atomic_get(&ipi_cpu_bitmap);

			/*
			 * Note: z_trace_sched_ipi(), which is used to track
			 * which CPUs processed an IPI, is not just called as a
			 * result of the primitives arch_sched_directed_ipi()
			 * or arch_sched_broadcast_ipi() above. Schedule IPIs
			 * will also be sent when ticks are announced such as
			 * when the k_sleep() in report() expires and this
			 * benchmark can not control which CPUs will receive
			 * those IPIs. To account for this, a mask is applied.
			 */
			if ((value & desired_ipi_set) == desired_ipi_set) {
				break;
			}

			key = arch_irq_lock();
			arch_spin_relax();
			arch_irq_unlock(key);
		}
	}
}

void report(void)
{
	unsigned int elapsed_time = IPI_TEST_INTERVAL_DURATION;
	unsigned int i;
	unsigned long total;
	unsigned long counter[NUM_WORK_THREADS];
	unsigned long last_counter[NUM_WORK_THREADS] = {};
	unsigned long last_issued = 0;
	unsigned long interval_issued;

	while (1) {
		k_sleep(K_SECONDS(IPI_TEST_INTERVAL_DURATION));

		total = 0;

		for (i = 0; i < NUM_WORK_THREADS; i++) {
			counter[i] = work_counter[i] - last_counter[i];
			total += counter[i];
			last_counter[i] = work_counter[i];
		}

		interval_issued = primitives_issued - last_issued;

		printf("**** IPI-Metric %s IPI Test **** Elapsed Time: %u\n",
		       IS_ENABLED(CONFIG_IPI_METRIC_PRIMITIVE_DIRECTED) ?
		       "Directed" : "Broadcast", elapsed_time);

		printf("  Schedule IPIs Issued: %lu\n", interval_issued);
		last_issued = primitives_issued;

		printf("  Total Work: %lu\n", total);
		for (i = 0; i < NUM_WORK_THREADS; i++) {
			printf("   - Work Counter #%u: %lu\n",
			       i, counter[i]);
		}

		elapsed_time += IPI_TEST_INTERVAL_DURATION;
	}
}

int main(void)
{
	unsigned int i;

	for (i = 0; i < NUM_WORK_THREADS; i++) {
		k_thread_create(&work_thread[i], work_stack[i],
				WORK_STACK_SIZE, work_entry,
				UINT_TO_POINTER(i), work_array[i], NULL,
				-1, 0, K_NO_WAIT);
	}

	/* Create the primitive thread. */

	k_thread_create(&primitive_thread, primitive_stack,
			PRIMITIVE_STACK_SIZE, primitive_entry,
			UINT_TO_POINTER(i), NULL, NULL,
			10, 0, K_NO_WAIT);

	report();
}
