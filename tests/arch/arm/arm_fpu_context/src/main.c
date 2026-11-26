/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * With FPU sharing, a thread must start with zeroed FP registers: neither a
 * previous thread that used the same thread object nor kernel code run by
 * the thread before it enters user mode may pass its FP values on.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define PATTERN_LO 0x5a5a1234U
#define PATTERN_HI 0xa5a5cafeU
/* Round towards zero, flush to zero */
#define PATTERN_FPSCR 0x01c00000U

struct fp_state {
	uint32_t d0[2];
	uint32_t d15[2];
	uint32_t fpscr;
};

static void fp_write(void)
{
	__asm__ volatile("vmov d0, %0, %1\n"
			 "vmov d15, %0, %1\n"
			 "vmsr fpscr, %2\n"
			 :
			 : "r"(PATTERN_LO), "r"(PATTERN_HI), "r"(PATTERN_FPSCR)
			 : "d0", "d15", "memory");
}

static void fp_read(struct fp_state *s)
{
	uint32_t lo0, hi0, lo15, hi15, fpscr;

	__asm__ volatile("vmov %0, %1, d0\n"
			 "vmov %2, %3, d15\n"
			 "vmrs %4, fpscr\n"
			 : "=r"(lo0), "=r"(hi0), "=r"(lo15), "=r"(hi15), "=r"(fpscr));
	s->d0[0] = lo0;
	s->d0[1] = hi0;
	s->d15[0] = lo15;
	s->d15[1] = hi15;
	s->fpscr = fpscr;
}

static void check_zero(const struct fp_state *s)
{
	zassert_equal(s->d0[0] | s->d0[1], 0U, "d0: 0x%08x%08x", s->d0[1], s->d0[0]);
	zassert_equal(s->d15[0] | s->d15[1], 0U, "d15: 0x%08x%08x", s->d15[1], s->d15[0]);
	zassert_equal(s->fpscr, 0U, "fpscr: 0x%08x", s->fpscr);
}

#define STACK_SIZE 1024
K_THREAD_STACK_DEFINE(fp_stack, STACK_SIZE);
static struct k_thread fp_thread;
ZTEST_BMEM static struct fp_state seen;

static void writer_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fp_write();
}

static void reader_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fp_read(&seen);
}

static void run(k_thread_entry_t entry, uint32_t options)
{
	k_tid_t tid = k_thread_create(&fp_thread, fp_stack, STACK_SIZE, entry, NULL, NULL,
				      NULL, K_PRIO_PREEMPT(1), options, K_NO_WAIT);

	zassert_ok(k_thread_join(tid, K_FOREVER));
}

/* A thread reusing the thread object of one that left FP values behind */
ZTEST(arm_fpu_context, test_reused_thread_object)
{
	run(writer_entry, 0);
	memset(&seen, 0xff, sizeof(seen));
	run(reader_entry, 0);
	check_zero(&seen);
}

#if defined(CONFIG_USERSPACE)
static void kernel_then_user_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	fp_write();
	k_thread_user_mode_enter(reader_entry, NULL, NULL, NULL);
}

/* A thread entering user mode after using the FPU in kernel mode */
ZTEST(arm_fpu_context, test_user_mode_enter)
{
	memset(&seen, 0xff, sizeof(seen));
	run(kernel_then_user_entry, K_INHERIT_PERMS);
	check_zero(&seen);
}
#endif

ZTEST_SUITE(arm_fpu_context, NULL, NULL, NULL, NULL, NULL);
