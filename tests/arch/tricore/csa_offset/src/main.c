/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/tricore/cr.h>

/* Every recursion level keeps a stack frame: allow 16 bytes per CSA */
#define RSTACK_SIZE (CONFIG_TRICORE_CSA_COUNT * 16)

static K_THREAD_STACK_DEFINE(rstack, RSTACK_SIZE);
static struct k_thread rthread;
static volatile uint32_t sink;
static volatile uint32_t oops_pcxi;
static volatile unsigned int hook_reason = UINT32_MAX;
static volatile uintptr_t hook_esf;

static uintptr_t pcxi_to_csa(uint32_t pcxi)
{
	return ((pcxi & 0xF0000) << 12) | ((pcxi & 0xFFFF) << 6);
}

void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *esf)
{
	hook_reason = reason;
	hook_esf = (uintptr_t)esf;
}

/*
 * Recurse until the CSA in use lies more than 64 KiB above its segment base
 * (PCXO >= 0x400), then raise a kernel oops. The oops system call converts
 * PCXI into the address of the saved context; that conversion must keep
 * PCXO bits 15:10 out of address bits 27:22.
 */
static uint32_t recurse(int depth)
{
	volatile uint32_t local = depth;
	uint32_t pcxi = cr_read(TRICORE_PCXI);

	if ((pcxi & 0xFFFF) >= 0x420) {
		oops_pcxi = pcxi;
		k_oops();
	}

	if (depth > 0) {
		local += recurse(depth - 1);
	}

	sink += local;

	return local;
}

static void oops_entry(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	/* Disable call depth counting (PSW.CDC = 0x7F) for the deep recursion */
	cr_write(TRICORE_PSW, cr_read(TRICORE_PSW) | 0x7F);

	ztest_set_fault_valid(true);
	recurse(CONFIG_TRICORE_CSA_COUNT);

	zassert_unreachable("no CSA above 64 KiB reached");
}

ZTEST(tricore_csa_offset, test_oops_with_high_csa_offset)
{
	k_tid_t t = k_thread_create(&rthread, rstack, RSTACK_SIZE, oops_entry, NULL, NULL, NULL,
				    K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	k_thread_join(t, K_FOREVER);

	zassert_not_equal(oops_pcxi, 0, "deep CSA not reached");
	zassert_equal(hook_reason, K_ERR_KERNEL_OOPS, "reason %u instead of a kernel oops",
		      hook_reason);
	zassert_equal(hook_esf & 0x0FC00000, 0, "ESF 0x%08lx has PCXO bits in 27:22",
		      (unsigned long)hook_esf);
	zassert_equal(hook_esf >> 28, pcxi_to_csa(oops_pcxi) >> 28, "ESF segment mismatch");
}

ZTEST_SUITE(tricore_csa_offset, NULL, NULL, NULL, NULL, NULL);
