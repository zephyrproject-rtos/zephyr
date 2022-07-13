/*
 * Copyright (c) 2022 Baumer (www.baumer.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>


#define EXECUTION_TRACE_LENGTH 6

#define IRQ_A_PRIO  1   /* lower priority */
#define IRQ_B_PRIO  0   /* higher priority */


#define CHECK_STEP(pos, val) zassert_equal(	\
	execution_trace[pos],			\
	val,					\
	"Expected %s for step %d but got %s",	\
	execution_step_str(val),		\
	pos,					\
	execution_step_str(execution_trace[pos]))


enum execution_step {
	STEP_MAIN_BEGIN,
	STEP_MAIN_END,
	STEP_ISR_A_BEGIN,
	STEP_ISR_A_END,
	STEP_ISR_B_BEGIN,
	STEP_ISR_B_END,
};

static volatile enum execution_step execution_trace[EXECUTION_TRACE_LENGTH];
static volatile int execution_trace_pos;

static int irq_a;
static int irq_b;

static const char *execution_step_str(enum execution_step s)
{
	const char *res = "invalid";

	switch (s) {
	case STEP_MAIN_BEGIN:
		res = "STEP_MAIN_BEGIN";
		break;
	case STEP_MAIN_END:
		res = "STEP_MAIN_END";
		break;
	case STEP_ISR_A_BEGIN:
		res = "STEP_ISR_A_BEGIN";
		break;
	case STEP_ISR_A_END:
		res = "STEP_ISR_A_END";
		break;
	case STEP_ISR_B_BEGIN:
		res = "STEP_ISR_B_BEGIN";
		break;
	case STEP_ISR_B_END:
		res = "STEP_ISR_B_END";
		break;
	default:
		break;
	}
	return res;
}


static void execution_trace_add(enum execution_step s)
{
	__ASSERT(execution_trace_pos < EXECUTION_TRACE_LENGTH,
		 "Execution trace overflow");
	execution_trace[execution_trace_pos] = s;
	execution_trace_pos++;
}



void isr_a_handler(const void *args)
{
	ARG_UNUSED(args);
	execution_trace_add(STEP_ISR_A_BEGIN);

	/* Set higher prior irq b pending */
	NVIC_SetPendingIRQ(irq_b);
	__DSB();
	__ISB();

	execution_trace_add(STEP_ISR_A_END);
}


void isr_b_handler(const void *args)
{
	ARG_UNUSED(args);
	execution_trace_add(STEP_ISR_B_BEGIN);
	execution_trace_add(STEP_ISR_B_END);
}


static int find_unused_irq(int start)
{
	int i;

	for (i = start - 1; i >= 0; i--) {
		if (NVIC_GetEnableIRQ(i) == 0) {
			/*
			 * Interrupts configured statically with IRQ_CONNECT(.)
			 * are automatically enabled. NVIC_GetEnableIRQ()
			 * returning false, here, implies that the IRQ line is
			 * either not implemented or it is not enabled, thus,
			 * currently not in use by Zephyr.
			 */

			/* Set the NVIC line to pending. */
			NVIC_SetPendingIRQ(i);

			if (NVIC_GetPendingIRQ(i)) {
				/*
				 * If the NVIC line is pending, it is
				 * guaranteed that it is implemented; clear the
				 * line.
				 */
				NVIC_ClearPendingIRQ(i);

				if (!NVIC_GetPendingIRQ(i)) {
					/*
					 * If the NVIC line can be successfully
					 * un-pended, it is guaranteed that it
					 * can be used for software interrupt
					 * triggering. Return the NVIC line
					 * number.
					 */
					break;
				}
			}
		}
	}

	zassert_true(i >= 0,
		     "No available IRQ line to configure as zero-latency\n");

	TC_PRINT("Available IRQ line: %u\n", i);
	return i;
}

ZTEST(arm_irq_zero_latency_levels, test_arm_zero_latency_levels)
{
	/*
	 * Confirm that a zero-latency interrupt with lower priority will be
	 * interrupted by a zero-latency interrupt with higher priority.
	 */

	if (!IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)) {
		TC_PRINT("Skipped (Cortex-M Mainline only)\n");
		return;
	}

	/* Determine two NVIC IRQ lines that are not currently in use. */
	irq_a = find_unused_irq(CONFIG_NUM_IRQS);
	irq_b = find_unused_irq(irq_a);

	/* Configure IRQ A as zero-latency interrupt with prio 1 */
	arch_irq_connect_dynamic(irq_a, IRQ_A_PRIO, isr_a_handler,
				 NULL, IRQ_ZERO_LATENCY);
	NVIC_ClearPendingIRQ(irq_a);
	NVIC_EnableIRQ(irq_a);

	/* Configure irq_b as zero-latency interrupt with prio 0 */
	arch_irq_connect_dynamic(irq_b, IRQ_B_PRIO, isr_b_handler,
				 NULL, IRQ_ZERO_LATENCY);
	NVIC_ClearPendingIRQ(irq_b);
	NVIC_EnableIRQ(irq_b);

	/* Lock interrupts */
	unsigned int key = irq_lock();

	execution_trace_add(STEP_MAIN_BEGIN);

	/* Trigger irq_a */
	NVIC_SetPendingIRQ(irq_a);
	__DSB();
	__ISB();

	execution_trace_add(STEP_MAIN_END);

	/* Confirm that irq_a interrupted main and irq_b interrupted irq_a */
	CHECK_STEP(0, STEP_MAIN_BEGIN);
	CHECK_STEP(1, STEP_ISR_A_BEGIN);
	CHECK_STEP(2, STEP_ISR_B_BEGIN);
	CHECK_STEP(3, STEP_ISR_B_END);
	CHECK_STEP(4, STEP_ISR_A_END);
	CHECK_STEP(5, STEP_MAIN_END);

	/* Unlock interrupts */
	irq_unlock(key);
}

ZTEST_SUITE(arm_irq_zero_latency_levels, NULL, NULL, NULL, NULL, NULL);
