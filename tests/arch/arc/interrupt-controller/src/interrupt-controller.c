/*
 * Copyright (c) 2021 Synopsys, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <soc.h>

#define TEST_IRQ_0_PRIVATE	22
#define TEST_IRQ_1_PRIVATE	17
#define TEST_IRQ_2_PRIVATE	18

#define TEST_IRQ_0_SHARED	26
#define TEST_IRQ_1_SHARED	27
#define TEST_IRQ_2_SHARED	28

#ifndef IRQ_ICI
#define IRQ_ICI 19
#endif

BUILD_ASSERT(CONFIG_NUM_IRQ_PRIO_LEVELS >= 2, "test requires at least 2 priority levels");
BUILD_ASSERT(CONFIG_NUM_IRQS >= TEST_IRQ_2_SHARED, "not enough interrupt lines for test");

#ifdef CONFIG_ARC_CONNECT
/*
 * In case of SMP system all private interrupts are expected to have
 * interrupt number < ARC_CONNECT_IDU_IRQ_START
 * and shared interrupts have
 * interrupt number >= ARC_CONNECT_IDU_IRQ_START
 */
BUILD_ASSERT(IRQ_ICI < ARC_CONNECT_IDU_IRQ_START);
BUILD_ASSERT(TEST_IRQ_0_PRIVATE < ARC_CONNECT_IDU_IRQ_START);
BUILD_ASSERT(TEST_IRQ_1_PRIVATE < ARC_CONNECT_IDU_IRQ_START);
BUILD_ASSERT(TEST_IRQ_2_PRIVATE < ARC_CONNECT_IDU_IRQ_START);

BUILD_ASSERT(TEST_IRQ_0_SHARED >= ARC_CONNECT_IDU_IRQ_START);
BUILD_ASSERT(TEST_IRQ_1_SHARED >= ARC_CONNECT_IDU_IRQ_START);
BUILD_ASSERT(TEST_IRQ_2_SHARED >= ARC_CONNECT_IDU_IRQ_START);
#endif /* CONFIG_ARC_CONNECT */


/* TODO: add locking */
static void intc_prio_check_single(uint32_t irq, uint8_t expected_prio, const char *reason)
{
	uint32_t prio;

	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	prio = z_arc_v2_aux_reg_read(_ARC_V2_IRQ_PRIORITY);
	prio &= _ARC_V2_INT_PRIO_MASK;

	zassert_true(prio == expected_prio,
		"readback priority unexpected irq %u, expected %u, got %u, check reason %s\n",
		irq, expected_prio, prio, reason);
}


static void intc_irq_check_masking_dynamic_single(uint32_t irq)
{
	arch_irq_enable(irq);
	zassert_true(arch_irq_is_enabled(irq), "irq %u, unexpected state after manipulation\n");

	arch_irq_disable(irq);
	zassert_false(arch_irq_is_enabled(irq), "irq %u, unexpected state after manipulation\n");

	arch_irq_enable(irq);
	zassert_true(arch_irq_is_enabled(irq), "irq %u, unexpected state after manipulation\n");

	arch_irq_disable(irq);
	zassert_false(arch_irq_is_enabled(irq), "irq %u, unexpected state after manipulation\n");
}

static void intc_irq_check_masking_dynamic(void)
{
	intc_irq_check_masking_dynamic_single(TEST_IRQ_0_PRIVATE);
	intc_irq_check_masking_dynamic_single(TEST_IRQ_0_SHARED);
}

static void intc_irq_check_default_state_single(uint32_t irq, bool expected)
{
	zassert_true(!!arch_irq_is_enabled(irq) == expected,
		"unexpected irq %u default state\n", irq);
}

static void intc_irq_check_default_state(void)
{
	intc_irq_check_default_state_single(TEST_IRQ_0_PRIVATE, false);
	intc_irq_check_default_state_single(TEST_IRQ_0_SHARED, false);

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	intc_irq_check_default_state_single(IRQ_SEC_TIMER0, true);
#else
	intc_irq_check_default_state_single(IRQ_TIMER0, true);
#endif /* CONFIG_ARC_SECURE_FIRMWARE */

#ifdef CONFIG_ARC_CONNECT
	intc_irq_check_default_state_single(IRQ_ICI, true);
#endif /* CONFIG_ARC_CONNECT */
}

BUILD_ASSERT(CONFIG_NUM_IRQS - CONFIG_GEN_IRQ_START_VECTOR == IRQ_TABLE_SIZE);

static void intc_irq_static_priority_verify(bool print_info)
{
	for (uint32_t irq = CONFIG_GEN_IRQ_START_VECTOR; irq < CONFIG_NUM_IRQS; irq++) {
		uint8_t priority = _irq_priority_table[irq - CONFIG_GEN_IRQ_START_VECTOR];

		if (print_info) {
			TC_PRINT("ISR %u: priority static encoded %u\n", irq, priority);
		}

		zassert_true(priority < CONFIG_NUM_IRQ_PRIO_LEVELS,
			"interrupt priority incorrect\n");
		intc_prio_check_single(irq, priority, "static config vs readback");
	}
}

static void dummy_irq_handler0(const void *unused) {}
static void dummy_irq_handler1(const void *unused) {}
static void dummy_irq_handler2(const void *unused) {}
static void dummy_irq_handler3(const void *unused) {}
static void dummy_irq_handler4(const void *unused) {}
static void dummy_irq_handler5(const void *unused) {}

static void arc_connect_interrupts(void)
{
	/*
	 * NOTE: we connect interrupts *not* in irq number order intentionally to check that
	 * interrupt sorting actually works.
	 */

	IRQ_CONNECT(TEST_IRQ_0_PRIVATE, 1, dummy_irq_handler0, NULL, 0);
	intc_prio_check_single(TEST_IRQ_0_PRIVATE, 1, "passed to connect vs readback");
	IRQ_CONNECT(TEST_IRQ_1_PRIVATE, 0, dummy_irq_handler1, NULL, 0);
	intc_prio_check_single(TEST_IRQ_1_PRIVATE, 0, "passed to connect vs readback");
	IRQ_CONNECT(TEST_IRQ_2_PRIVATE, 1, dummy_irq_handler2, NULL, 0);
	intc_prio_check_single(TEST_IRQ_2_PRIVATE, 1, "passed to connect vs readback");

	/* shared irq in case of SMP */
	IRQ_CONNECT(TEST_IRQ_1_SHARED, 1, dummy_irq_handler4, NULL, 0);
	intc_prio_check_single(TEST_IRQ_1_SHARED, 1, "passed to connect vs readback");
	IRQ_CONNECT(TEST_IRQ_0_SHARED, 0, dummy_irq_handler3, NULL, 0);
	intc_prio_check_single(TEST_IRQ_0_SHARED, 0, "passed to connect vs readback");


	/*
	 * Check that we can pass some complex expression (which is still computable in compile
	 * time) to IRQ_CONNECT. One of the WIP implementation worked only in case of
	 * IRQ_CONNECT priority value was computable by preprocessor, so we want to
	 * test against this flaw.
	 */
#define ARC_IRQ_C_PRIO_PREREC0		(1U << 3)
#define ARC_IRQ_C_PRIO_PREREC1		((ARC_IRQ_C_PRIO_PREREC0) << 3)
#define ARC_IRQ_C_PRIO_PREREC2(x)	((x) >> 6)
#define ARC_IRQ_C_PRIO_PREREC3		(2U - ARC_IRQ_C_PRIO_PREREC2(ARC_IRQ_C_PRIO_PREREC1))
#define ARC_IRQ_C_PRIO			(0x1 + ARC_IRQ_C_PRIO_PREREC3 - 1)
	BUILD_ASSERT(ARC_IRQ_C_PRIO == 1);
	IRQ_CONNECT(TEST_IRQ_2_SHARED, ARC_IRQ_C_PRIO, dummy_irq_handler5, NULL, 0);
	intc_prio_check_single(TEST_IRQ_2_SHARED, ARC_IRQ_C_PRIO, "passed to connect vs readback");
}

void test_initial_state(void)
{
	/*
	 * As all connect stuff is done at compile time the results are expected to be the same
	 * before and after IRQ_CONNECT call in arc_connect_interrupts().
	 */
	intc_irq_check_default_state();
	intc_irq_static_priority_verify(true);
	arc_connect_interrupts();
	intc_irq_check_default_state();
	intc_irq_static_priority_verify(false);

	intc_irq_check_masking_dynamic();
}
