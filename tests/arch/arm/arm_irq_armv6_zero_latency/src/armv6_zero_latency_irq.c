/*
 * Copyright (c) Baumer Electric AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <arch/arm/aarch32/irq.h>

#if defined(CONFIG_ZERO_LATENCY_IRQS_ARMV6_M)

#define ISR1_OFFSET 0
#define ISR2_OFFSET 1
#define ISR3_OFFSET 2

#define THREAD_STACK_SIZE 500
#define THREAD_PRIORITY_A 1
#define THREAD_PRIORITY_B 0

K_THREAD_STACK_DEFINE(stack_area_A, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_area_B, THREAD_STACK_SIZE);

static volatile int test_flag_zero_latency, test_flag_normal_irq;
k_tid_t thread_A, thread_B;
struct k_thread thread_data_A, thread_data_B;
static struct k_sem task_sem, thread_A_sem, thread_B_sem;

/* static function prototypes */
static void instruction_barrier(void);
static void entry_point_thread_A(void);
static void entry_point_thread_B(void);

void arm_zero_latency_isr_handler(const void *args)
{
	ARG_UNUSED(args);

	test_flag_zero_latency++;
}

void arm_normal_isr_handler(const void *args)
{
	ARG_UNUSED(args);

	test_flag_normal_irq++;
}

/* Test the interrupt locking */
void test_armv6_zl_irqs_locking(void)
{
	uint32_t key;

	/* Configure the IRQ lines. */
	IRQ_DIRECT_CONNECT(ISR1_OFFSET, 0, arm_zero_latency_isr_handler, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(ISR3_OFFSET, 0, arm_normal_isr_handler, 0);
	irq_enable(ISR1_OFFSET);
	irq_enable(ISR3_OFFSET);

	/* Reset the test flags */
	test_flag_normal_irq = 0;
	test_flag_zero_latency = 0;

	key = irq_lock();

	/* Set the IRQ's to pending state. */
	NVIC_SetPendingIRQ(ISR1_OFFSET);
	NVIC_SetPendingIRQ(ISR3_OFFSET);
	instruction_barrier();

	/* Confirm test flag is NOT set by ISR handler but zero latency test flag is set. */
	zassert_true(test_flag_normal_irq == 0, "Test flag set by ISR\n");
	zassert_true(test_flag_zero_latency == 1, "zero latency flag not set by ISR\n");

	irq_unlock(key);

	/* Confirm test flag is now set by the ISR handler. */
	zassert_true(test_flag_normal_irq == 1, "Test flag not set by ISR\n");
	zassert_true(test_flag_zero_latency == 1, "zero latency flag not set by ISR\n");

	irq_disable(ISR1_OFFSET);
	irq_disable(ISR3_OFFSET);
}

/* Test the nesting of interrupt locking */
void test_armv6_zl_irqs_lock_nesting(void)
{
	uint32_t key1, key2;

	/* Configure the IRQ lines. */
	IRQ_DIRECT_CONNECT(ISR1_OFFSET, 0, arm_zero_latency_isr_handler, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(ISR3_OFFSET, 0, arm_normal_isr_handler, 0);
	irq_enable(ISR1_OFFSET);
	irq_enable(ISR3_OFFSET);

	/* Reset the test flags */
	test_flag_normal_irq = 0;
	test_flag_zero_latency = 0;

	key1 = irq_lock();
	key2 = irq_lock();

	/* Set the IRQ's to pending state. */
	NVIC_SetPendingIRQ(ISR1_OFFSET);
	NVIC_SetPendingIRQ(ISR3_OFFSET);

	instruction_barrier();

	/* Confirm test flag is NOT set by ISR handler but zero latency test flag is set. */
	zassert_true(test_flag_normal_irq == 0, "Test flag set by ISR\n");
	zassert_true(test_flag_zero_latency == 1, "zero latency test flag not set by ISR\n");

	irq_unlock(key2);

	/* Confirm test flag is still NOT set by ISR handler. */
	zassert_true(test_flag_normal_irq == 0, "Test flag set by ISR\n");

	irq_unlock(key1);

	/* Confirm test flag is now set by the ISR handler. */
	zassert_true(test_flag_normal_irq == 1, "Test flag not set by ISR\n");

	irq_disable(ISR1_OFFSET);
	irq_disable(ISR3_OFFSET);
}

/* Test if multiple zero latency irq's can be defined */
void test_armv6_zl_irqs_multiple(void)
{
	uint32_t key;
	/* Configure the IRQ lines. */
	IRQ_DIRECT_CONNECT(ISR1_OFFSET, 0, arm_zero_latency_isr_handler, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(ISR2_OFFSET, 0, arm_zero_latency_isr_handler, IRQ_ZERO_LATENCY);
	irq_enable(ISR1_OFFSET);
	irq_enable(ISR2_OFFSET);

	/* Reset the test flag */
	test_flag_zero_latency = 0;

	key = irq_lock();

	/* Set the IRQ's to pending state. */
	NVIC_SetPendingIRQ(ISR1_OFFSET);
	NVIC_SetPendingIRQ(ISR2_OFFSET);
	instruction_barrier();

	/* Confirm that zero latency test flag is set twice. */
	zassert_true(test_flag_zero_latency == 2, "zero latency test flag not set by ISR\n");

	irq_unlock(key);

	/* Confirm that zero latency test flag is set twice. */
	zassert_true(test_flag_zero_latency == 2, "zero latency test flag not set by ISR\n");

	irq_disable(ISR1_OFFSET);
	irq_disable(ISR2_OFFSET);
}

/* Test the enabling of irq while locking */
void test_armv6_zl_irqs_enable(void)
{
	uint32_t key;

	/* Configure the IRQ lines. */
	IRQ_DIRECT_CONNECT(ISR1_OFFSET, 0, arm_zero_latency_isr_handler, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(ISR3_OFFSET, 0, arm_normal_isr_handler, 0);
	NVIC_ClearPendingIRQ(ISR3_OFFSET);
	NVIC_ClearPendingIRQ(ISR1_OFFSET);

	/* Reset the test flags */
	test_flag_normal_irq = 0;
	test_flag_zero_latency = 0;

	key = irq_lock();

	/* Enable the IRQ and set it to pending state. */
	irq_enable(ISR3_OFFSET);
	NVIC_SetPendingIRQ(ISR3_OFFSET);
	irq_enable(ISR1_OFFSET);
	NVIC_SetPendingIRQ(ISR1_OFFSET);

	instruction_barrier();

	/* Confirm that test flag is not set, but the zero latency flag is set */
	zassert_true(test_flag_normal_irq == 0, "test flag set by ISR\n");
	zassert_true(test_flag_zero_latency == 1, "test flag not set by ISR\n");

	irq_unlock(key);

	/* Confirm that test flag is set */
	zassert_true(test_flag_normal_irq == 1, "test flag not set by ISR\n");

	irq_disable(ISR3_OFFSET);
	irq_disable(ISR1_OFFSET);
}

/* Test the disabeling of irq while locking */
void test_armv6_zl_irqs_disable(void)
{
	uint32_t key;

	/* Configure the IRQ lines. */
	IRQ_DIRECT_CONNECT(ISR1_OFFSET, 0, arm_zero_latency_isr_handler, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(ISR3_OFFSET, 0, arm_normal_isr_handler, 0);
	irq_enable(ISR3_OFFSET);
	irq_enable(ISR1_OFFSET);

	/* Reset the test flags */
	test_flag_normal_irq = 0;
	test_flag_zero_latency = 0;

	/* Lock interrupts */
	key = irq_lock();

	/* Disable the IRQ and set it to pending state. */
	irq_disable(ISR3_OFFSET);
	NVIC_SetPendingIRQ(ISR3_OFFSET);
	irq_disable(ISR1_OFFSET);
	NVIC_SetPendingIRQ(ISR1_OFFSET);

	instruction_barrier();

	/* Confirm that the zero latency flag is not set. */
	zassert_true(test_flag_zero_latency == 0, "test flag set by ISR\n");

	irq_unlock(key);

	/* Confirm that test flags are not set. */
	zassert_true(test_flag_zero_latency == 0, "test flag set by ISR\n");
	zassert_true(test_flag_normal_irq == 0, "test flag set by ISR\n");
}


/* Test the thread specificity */
void test_armv6_zl_irqs_thread_specificity(void)
{
	/* Configure the IRQ lines. */
	IRQ_DIRECT_CONNECT(ISR1_OFFSET, 0, arm_zero_latency_isr_handler, IRQ_ZERO_LATENCY);
	IRQ_DIRECT_CONNECT(ISR3_OFFSET, 0, arm_normal_isr_handler, 0);
	irq_enable(ISR3_OFFSET);
	irq_enable(ISR1_OFFSET);

	/* Reset the test flags */
	test_flag_normal_irq = 0;
	test_flag_zero_latency = 0;

	/* Create threads */
	thread_A = k_thread_create(&thread_data_A, stack_area_A,
				K_THREAD_STACK_SIZEOF(stack_area_A),
				(k_thread_entry_t)entry_point_thread_A,
				NULL, NULL, NULL,
				THREAD_PRIORITY_A, 0, K_NO_WAIT);

	thread_B = k_thread_create(&thread_data_B, stack_area_B,
				K_THREAD_STACK_SIZEOF(stack_area_B),
				(k_thread_entry_t)entry_point_thread_B,
				NULL, NULL, NULL,
				THREAD_PRIORITY_B, 0, K_NO_WAIT);

	k_sem_init(&task_sem, 0, 1);
	k_sem_init(&thread_A_sem, 0, 1);
	k_sem_init(&thread_B_sem, 0, 1);

	/* Wait for thread B to activate us */
	k_sem_take(&task_sem, K_FOREVER);

	irq_disable(ISR3_OFFSET);
	irq_disable(ISR1_OFFSET);
}

static void entry_point_thread_A(void)
{
	uint32_t key_a;

	/* Wait for thread B to activate us */
	k_sem_take(&thread_A_sem, K_FOREVER);

	/* Confirm that the the pending interrupt was handled,
	 * because there is no lock in this thread.
	 */
	zassert_true(test_flag_normal_irq == 2,
		"lock is not thread specific! lock is still enabled by thread B\n");

	key_a = irq_lock();

	NVIC_SetPendingIRQ(ISR3_OFFSET);
	NVIC_SetPendingIRQ(ISR1_OFFSET);
	instruction_barrier();

	/* Confirm that test flag is not set, but the zero latency flag is set */
	zassert_true(test_flag_normal_irq == 2, "IRQ lock does not work!\n");
	zassert_true(test_flag_zero_latency == 3, "zl test flag was not set by isr\n");

	irq_unlock(key_a);

	/* Confirm that test flag is set */
	zassert_true(test_flag_normal_irq == 3, "IRQ unlock does not work!\n");

	k_sem_give(&thread_B_sem);
	return; /* Terminate thread. */
}

static void entry_point_thread_B(void)
{
	uint32_t key_b;

	NVIC_SetPendingIRQ(ISR3_OFFSET);
	NVIC_SetPendingIRQ(ISR1_OFFSET);
	instruction_barrier();

	/* Confirm that flags are set */
	zassert_true(test_flag_normal_irq == 1, "test flag was not set by isr\n");
	zassert_true(test_flag_zero_latency == 1, "test flag was not set by isr\n");

	key_b = irq_lock();

	NVIC_SetPendingIRQ(ISR3_OFFSET);
	NVIC_SetPendingIRQ(ISR1_OFFSET);
	instruction_barrier();

	/* Confirm that test flag is not set, but the zero latency flag is set */
	zassert_true(test_flag_normal_irq == 1, "IRQ lock does not work!\n");
	zassert_true(test_flag_zero_latency == 2, "zl test flag was not set by isr\n");

	/* Activate thread A */
	k_sem_give(&thread_A_sem);
	/* Wait for thread A to activate us */
	k_sem_take(&thread_B_sem, K_FOREVER);

	/* Thread was woken by thread A */
	NVIC_SetPendingIRQ(ISR3_OFFSET);
	NVIC_SetPendingIRQ(ISR1_OFFSET);
	instruction_barrier();

	/* Confirm that test flag is not set, but the zero latency flag is set.
	 * this means that the lock is still enabled after thread switching.
	 */
	zassert_true(test_flag_normal_irq == 3,
		"lock is not thread specific!lock should still be enabled\n");
	zassert_true(test_flag_zero_latency == 4, "zl test flag was not set by isr\n");

	irq_unlock(key_b);

	/* Confirm that test flag is set */
	zassert_true(test_flag_normal_irq == 4, "IRQ unlock does not work!\n");

	k_sem_give(&task_sem);
	return;	/* Terminate thread. */
}

static void instruction_barrier(void)
{
	/*
	 * Instruction barrier to make sure the NVIC IRQ is
	 * set to pending state before test flag is checked.
	 */
	__ISB();
}

#else

void test_armv6_zl_irqs_locking(void)
{
	TC_PRINT("Skipped (ARMv6_M only)\n");
}

void test_armv6_zl_irqs_lock_nesting(void)
{
	TC_PRINT("Skipped (ARMv6_M only)\n");
}

void test_armv6_zl_irqs_multiple(void)
{
	TC_PRINT("Skipped (ARMv6_M only)\n");
}

void test_armv6_zl_irqs_enable(void)
{
	TC_PRINT("Skipped (ARMv6_M only)\n");
}

void test_armv6_zl_irqs_disable(void)
{
	TC_PRINT("Skipped (ARMv6_M only)\n");
}

void test_armv6_zl_irqs_thread_specificity(void)
{
	TC_PRINT("Skipped (ARMv6_M only)\n");
}

#endif /* CONFIG_ZERO_LATENCY_IRQS_ARMV6_M */
/**
 * @}
 */
