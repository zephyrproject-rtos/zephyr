/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa/corebits.h>
#include <xtensa_asm2_context.h>
#include <zephyr/ztest.h>

#define FAULT_A0 0x12345678U

static bool expect_fault;
static uint32_t captured_a0;
static uint32_t captured_reason;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	const _xtensa_irq_stack_frame_raw_t *frame = (const void *)esf;

	if (!expect_fault) {
		k_fatal_halt(reason);
	}

	captured_a0 = frame->ptr_to_bsa->a0;
	captured_reason = reason;
	expect_fault = false;
}

static void fault(void *kernel_mode, void *unused1, void *unused2)
{
	uint32_t ps;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	__asm__ volatile("rsr.ps %0" : "=a"(ps));
	if (kernel_mode != NULL) {
		ps &= ~PS_UM_MASK;
	} else {
		ps |= PS_UM_MASK;
	}

	__asm__ volatile("wsr.ps %0; rsync; movi a0, 0x12345678; ill" : : "a"(ps) : "a0", "memory");
}

static void check_exception(bool kernel_mode)
{
	static K_THREAD_STACK_DEFINE(stack, 2048);
	static struct k_thread thread;

	captured_a0 = 0U;
	captured_reason = UINT32_MAX;
	expect_fault = true;
	k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack), fault,
			kernel_mode ? &thread : NULL, NULL, NULL, 0, 0, K_NO_WAIT);
	zassert_ok(k_thread_join(&thread, K_SECONDS(1)));
	zassert_false(expect_fault);
	zassert_equal(captured_reason, K_ERR_CPU_EXCEPTION);
	zassert_equal(captured_a0, FAULT_A0);
}

ZTEST(vector_long_jumps, test_user_exception)
{
	check_exception(false);
}

ZTEST(vector_long_jumps, test_kernel_exception)
{
	check_exception(true);
}

ZTEST_SUITE(vector_long_jumps, NULL, NULL, NULL, NULL, NULL);
