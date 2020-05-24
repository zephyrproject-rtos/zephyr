/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel_structs.h>
#include "interrupt_util.h"
#include <kernel_internal.h>

#define ISR0_ARG        0xca55e77e
#define ISR1_ARG	0xb01dface
#define DURATION	5
#define TEST_IRQ_PRI	1

/*
 * We try to choose the unused irq that fitting identical category of platform
 * as possible.
 */
#if defined(CONFIG_CPU_CORTEX_M)

/* For nrf5340xdk_nrf5340_cpunet board, irq 19 has been used */
#if defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUNET) || \
	defined(CONFIG_BOARD_NRF5340PDK_NRF5340_CPUNET)

#define TEST_IRQ_0_LINE 18
#define TEST_IRQ_1_LINE 20

#elif CONFIG_NUM_IRQS >= 96
#define TEST_IRQ_0_LINE (CONFIG_NUM_IRQS - 3)
#define TEST_IRQ_1_LINE (CONFIG_NUM_IRQS - 4)

#elif CONFIG_NUM_IRQS <= 8
#define TEST_IRQ_0_LINE 5
#define TEST_IRQ_1_LINE 6

#elif CONFIG_NUM_IRQS > 8 && CONFIG_NUM_IRQS < 32
#define TEST_IRQ_0_LINE 18
#define TEST_IRQ_1_LINE 19

#elif CONFIG_NUM_IRQS >= 32 && CONFIG_NUM_IRQS < 96
#define TEST_IRQ_0_LINE 22
#define TEST_IRQ_1_LINE 29
#endif

#elif defined(CONFIG_GIC)

#define TEST_IRQ_0_LINE 14
#define TEST_IRQ_1_LINE 15

#elif defined(CONFIG_CPU_ARCV2)
/*
 * Use 2th and 3th from last is because they won't be multiple registrations in
 * all ARC platform.
 */
#define TEST_IRQ_0_LINE (CONFIG_NUM_IRQS - 2)
#define TEST_IRQ_1_LINE (CONFIG_NUM_IRQS - 3)

#elif defined(CONFIG_X86)

#define TEST_IRQ_0_LINE 16
#define TEST_IRQ_1_LINE 17

#elif defined(CONFIG_ARCH_POSIX)

#define TEST_IRQ_0_LINE 6
#define TEST_IRQ_1_LINE 7

#elif defined(CONFIG_RISCV)

/* For Litex vexriscv board, irq 1 has been used */
#if defined(CONFIG_SOC_RISCV32_LITEX_VEXRISCV)
#define TEST_IRQ_0_LINE 2
#else
#define TEST_IRQ_0_LINE 1
#endif

#define TEST_IRQ_1_LINE 5

#elif defined(CONFIG_XTENSA)

#define TEST_IRQ_0_LINE 11

#endif

#define ORDER_TOP_DOWN	1
#define ORDER_BOTTOM_UP	0

#define DEBUG_STACK_TEST 0

K_KERNEL_STACK_EXTERN(z_main_stack);
extern K_KERNEL_STACK_ARRAY_DEFINE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);


#if !defined(CONFIG_BOARD_QEMU_CORTEX_A53)      && \
	!defined(CONFIG_BOARD_NATIVE_POSIX)     && \
	!defined(CONFIG_BOARD_NRF52_BSIM)

/* Helper function that check if a memory addr is in a region */
static bool check_addr_inside_region(uintptr_t addr, uintptr_t start, size_t sz)
{
	bool ret = true;

	if (addr < start || (addr > start + sz))
		ret = false;

	return ret;
}
#endif

/* Helper function that check if two memory region overlap */
static bool check_region_overlap(uintptr_t mr1_top, uintptr_t mr1_bottom,
		uintptr_t mr2_top, uintptr_t mr2_bottom)
{
	bool ret = true;

	if (mr1_top <= mr2_top && mr1_top >= mr2_bottom)
		ret = false;

	if (mr1_bottom <= mr2_top && mr1_bottom >= mr2_bottom)
		ret = false;

	if (mr1_top >= mr2_top && mr1_bottom <= mr2_bottom)
		ret = false;

	return ret;
}

static void find_stack_top_bottom(uintptr_t *sp_top, uintptr_t *sp_bottom,
		uintptr_t sp, size_t sz, bool direction)
{
	if (direction == ORDER_TOP_DOWN) {
		*sp_top = sp;
		*sp_bottom = sp - sz;
	} else {
		*sp_top = sp + sz;
		*sp_bottom = sp;
	}
}

/* Helper function for checking if two stacks overlap or not, by giving their
 * current statck pointer.
 */
static bool check_stack_overlap(uintptr_t sp1, size_t sz_sp1, bool dir_1,
		uintptr_t sp2, size_t sz_sp2, bool dir_2)
{
	uintptr_t sp1_top, sp1_bottom, sp2_top, sp2_bottom;

	find_stack_top_bottom(&sp1_top, &sp1_bottom, sp1, sz_sp1, dir_1);
	find_stack_top_bottom(&sp2_top, &sp2_bottom, sp2, sz_sp2, dir_2);

	TC_PRINT("sp1(0x%lx)(0x%lx) sp2(0x%lx)(0x%lx)\n",
			(unsigned long)sp1_top, (unsigned long)sp1_bottom,
			(unsigned long)sp2_top, (unsigned long)sp2_bottom);

	return check_region_overlap(sp1_top, sp1_bottom, sp2_top, sp2_bottom);
}

/* Getting a address that close to current stack pointer */
static inline void get_stack_addr(uintptr_t *reg)
{
	int dummy;
	*reg = (uintptr_t)&dummy;
}

static uintptr_t handler_test_result;
static uintptr_t int_0_stack_pointer;
static uintptr_t int_1_stack_pointer;

/* self-defined handler for testing */
void test_handler(const void *param)
{
	handler_test_result = (uintptr_t)param;

	zassert_true(k_is_in_isr(), "not in a interrupt context");

	/* for different ISR, get esp of current thread */
	if (handler_test_result == ISR0_ARG) {
		get_stack_addr(&int_0_stack_pointer);
	}

	if (handler_test_result == ISR1_ARG) {
		get_stack_addr(&int_1_stack_pointer);
	}
}

/**
 * @brief Test interrupt features
 *
 * @details This test case validates four features of interrupt
 * - The kernel support configuration of interrupts statically at build time.
 * - The user shall be able to supply a word-sized parameter when configuring
 *   interrupts, which are passed to the interrupt service routine when the
 *   interrupt happens.
 * - The kernel support multiple ISRs utilizing the same function to process
 *   interrupts, allowing a single function to service a device that generates
 *   multiple types of interrupts or to service multiple devices.
 * - The kernel provide a dedicated interrupt stack for processing hardware
 *   interrupts.
 *
 * @ingroup kernel_interrupt_tests
 */
void test_isr_static(void)
{
	uintptr_t curr_stack_ptr;
	uintptr_t stack_start = k_current_get()->stack_info.start;
	size_t stack_size = k_current_get()->stack_info.size;

	/**TESTPOINT: configuration of interrupts statically at build time */
	IRQ_CONNECT(TEST_IRQ_0_LINE, TEST_IRQ_PRI, test_handler,
		       (void *)ISR0_ARG, 0);

	zassert_not_equal(handler_test_result, ISR0_ARG,
		"shall not get parameter before interrupt");

	irq_enable(TEST_IRQ_0_LINE);
	trigger_irq(TEST_IRQ_0_LINE);

	k_busy_wait(MS_TO_US(DURATION));

	/**TESTPOINT: pass word-sized parameter to interrupt */
	zassert_equal(handler_test_result, ISR0_ARG,
		"parameter(%lx) in interrupt is not correct",
		handler_test_result);


/* For qemu_xtensa, there are only two software ISR, one is use for irq_offload
 * , so we only have one left to testing. That way we skip it here.
 */
#if !defined(CONFIG_XTENSA)

	IRQ_CONNECT(TEST_IRQ_1_LINE, TEST_IRQ_PRI, test_handler,
		       (void *)ISR1_ARG, 0);

	zassert_not_equal(handler_test_result, ISR1_ARG,
		"shall not get parameter before interrupt");

	irq_enable(TEST_IRQ_1_LINE);
	trigger_irq(TEST_IRQ_1_LINE);

	k_busy_wait(MS_TO_US(DURATION));

	/**
	 * TESTPOINT: multiple ISRs utilizing the same function to process
	 * interrupts.
	 */
	zassert_equal(handler_test_result, ISR1_ARG,
		"parameter(%lx) in interrupt is not correct",
		handler_test_result);

	/**TESTPOINT: Different interrupt using indentical stack */
	zassert_false(check_stack_overlap(int_0_stack_pointer,
			CONFIG_ISR_STACK_SIZE, ORDER_TOP_DOWN,
			int_1_stack_pointer, CONFIG_ISR_STACK_SIZE,
			ORDER_TOP_DOWN),
			"different interrupt should use the same stack");
#endif

	get_stack_addr(&curr_stack_ptr);

#ifdef DEBUG_STACK_TEST
	TC_PRINT("interrupt stack:\n--->top(0x%lx) size(0x%x)\n \
			int0(0x%lx) int1(0x%lx)\n--->bottom(0x%lx)\n\n",
				(unsigned long)_kernel.cpus[0].irq_stack,
				CONFIG_ISR_STACK_SIZE,
				(unsigned long)int_0_stack_pointer,
				(unsigned long)int_1_stack_pointer,
				(unsigned long)z_interrupt_stacks[0]);

	TC_PRINT("thread stack:\nmain(0x%lx)\n--->top(0x%lx) size(0x%lx)\n \
			curr(0x%lx)\n--->bottom(0x%lx)\n\n",
				(unsigned long)z_main_stack,
				(unsigned long)stack_start + stack_size,
				(unsigned long)stack_size,
				(unsigned long)curr_stack_ptr,
				(unsigned long)stack_start);
#endif

/* TODO: For these three boards, their interrupt stack pointer was not
 * located in interrupt stack correctly, so skip testing here at this
 * moment, need to be fixed as possisble. */
#if !defined(CONFIG_BOARD_QEMU_CORTEX_A53)	&& \
	!defined(CONFIG_BOARD_NATIVE_POSIX)	&& \
	!defined(CONFIG_BOARD_NRF52_BSIM)

	/**TESTPOINT: current stack pointer should be in thread stack */
	zassert_true(check_addr_inside_region(curr_stack_ptr, stack_start,
			stack_size),
			"current stack pointer shall be in region");

	/**TESTPOINT: dedicated interrupt stack for processing interrupt */
	zassert_true(check_stack_overlap(stack_start, stack_size,
			ORDER_BOTTOM_UP, int_0_stack_pointer,
			CONFIG_ISR_STACK_SIZE, ORDER_TOP_DOWN),
			"interrupt and thread shall be different stack");
#endif
}
