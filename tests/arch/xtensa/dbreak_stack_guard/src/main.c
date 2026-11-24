/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <xtensa/corebits.h>

#define CHILD_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define GUARD_CTRL                                                                                 \
	(DBREAKC_LOADBREAK_MASK | DBREAKC_STOREBREAK_MASK |                                        \
	 (DBREAKC_MASK_MASK & ~(CONFIG_XTENSA_DBREAK_STACK_GUARD_SIZE - 1U)))

/* The child runs on child_stacks[1]; the top of child_stacks[0] lies just below it. */
static K_THREAD_STACK_ARRAY_DEFINE(child_stacks, 2, CHILD_STACK_SIZE);
static struct k_thread child;
static K_SEM_DEFINE(isr_sem, 0, 1);

static volatile unsigned int fatal_count;
static volatile unsigned int fatal_reason;
static volatile bool child_returned;
static volatile uint32_t child_dbreaka_at_start;
static volatile uint32_t child_dbreaka_after_isr;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	if (k_current_get() != &child) {
		printk("Unexpected fatal error %u\n", reason);
		k_fatal_halt(reason);
	}

	fatal_count++;
	fatal_reason = reason;
}

static uint32_t guard_of(const struct k_thread *thread)
{
	return thread->stack_info.start - CONFIG_XTENSA_DBREAK_STACK_GUARD_SIZE;
}

static uint32_t read_dbreaka0(void)
{
	uint32_t val;

	__asm__ volatile("rsr.dbreaka0 %0" : "=r"(val));
	return val;
}

static uint32_t read_dbreakc0(void)
{
	uint32_t val;

	__asm__ volatile("rsr.dbreakc0 %0" : "=r"(val));
	return val;
}

static void run_child(k_thread_entry_t entry, uintptr_t arg, void *arg2)
{
	fatal_count = 0U;
	child_returned = false;

	k_thread_create(&child, child_stacks[1], K_THREAD_STACK_SIZEOF(child_stacks[1]), entry,
			(void *)arg, arg2, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_thread_join(&child, K_SECONDS(5)), "child did not finish");
}

static void read_own_guard(void *offset, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)*(volatile uint8_t *)(guard_of(k_current_get()) + (uintptr_t)offset);
	child_returned = true;
}

static void assert_guard_hit(void)
{
	zassert_equal(fatal_count, 1U, "expected one fatal error, got %u", fatal_count);
	zassert_equal(fatal_reason, K_ERR_STACK_CHK_FAIL, "unexpected reason %u", fatal_reason);
	zassert_false(child_returned, "guard access did not fault");
}

/* A read of the first guard byte is reported as a stack overflow. */
ZTEST(xtensa_dbreak_stack_guard, test_guard_first_byte)
{
	run_child(read_own_guard, 0U, NULL);
	assert_guard_hit();
}

/* A read of the last guard byte is reported as a stack overflow. */
ZTEST(xtensa_dbreak_stack_guard, test_guard_last_byte)
{
	run_child(read_own_guard, CONFIG_XTENSA_DBREAK_STACK_GUARD_SIZE - 1U, NULL);
	assert_guard_hit();
}

/* A read of the lowest stack byte, just above the guard, does not fault. */
ZTEST(xtensa_dbreak_stack_guard, test_stack_bottom_not_guarded)
{
	run_child(read_own_guard, CONFIG_XTENSA_DBREAK_STACK_GUARD_SIZE, NULL);
	zassert_equal(fatal_count, 0U, "access to the stack bottom faulted");
	zassert_true(child_returned, "child did not return");
}

static void record_guard(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	child_dbreaka_at_start = read_dbreaka0();
	k_sem_take(&isr_sem, K_FOREVER);
	child_dbreaka_after_isr = read_dbreaka0();
}

static void give_isr_sem(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_sem_give(&isr_sem);
}

/* DBREAK0 watches the running thread's guard after a swap and after an interrupt-exit switch. */
ZTEST(xtensa_dbreak_stack_guard, test_guard_follows_current_thread)
{
	static struct k_timer timer;
	uint32_t parent_guard = guard_of(k_current_get());

	zassert_equal(read_dbreaka0(), parent_guard, "DBREAK0 0x%x, guard 0x%x", read_dbreaka0(),
		      parent_guard);
	zassert_equal(read_dbreakc0(), GUARD_CTRL, "DBREAKC0 0x%x", read_dbreakc0());

	k_timer_init(&timer, give_isr_sem, NULL);
	k_timer_start(&timer, K_MSEC(10), K_NO_WAIT);
	run_child(record_guard, 0U, NULL);

	zassert_equal(fatal_count, 0U);
	zassert_equal(child_dbreaka_at_start, guard_of(&child), "after swap: 0x%x, guard 0x%x",
		      child_dbreaka_at_start, guard_of(&child));
	zassert_equal(child_dbreaka_after_isr, guard_of(&child),
		      "after interrupt exit: 0x%x, guard 0x%x", child_dbreaka_after_isr,
		      guard_of(&child));
	zassert_equal(read_dbreaka0(), parent_guard, "DBREAK0 0x%x, guard 0x%x", read_dbreaka0(),
		      parent_guard);
}

/* After a thread dies on its guard, DBREAK0 watches the guard of the thread that resumes. */
ZTEST(xtensa_dbreak_stack_guard, test_fault_exit_rearms_parent_guard)
{
	uint32_t parent_guard = guard_of(k_current_get());
	uint32_t child_guard;

	run_child(read_own_guard, 0U, NULL);
	assert_guard_hit();

	child_guard = guard_of(&child);
	zassert_not_equal(read_dbreaka0(), child_guard,
			  "DBREAK0 still watches the dead thread's guard");
	zassert_equal(read_dbreaka0(), parent_guard, "DBREAK0 0x%x, guard 0x%x", read_dbreaka0(),
		      parent_guard);
	zassert_equal(read_dbreakc0(), GUARD_CTRL, "DBREAKC0 0x%x", read_dbreakc0());

	(void)*(volatile uint8_t *)child_guard;
	zassert_equal(fatal_count, 1U, "parent faulted reading the dead thread's guard");
}

static void hit_guard_with_sp_at(void *sp_offset, void *exception, void *p3)
{
	uint32_t sp = guard_of(k_current_get()) + (uintptr_t)sp_offset;

	ARG_UNUSED(p3);

	if (exception != NULL) {
		__asm__ volatile("mov a1, %0\n\t"
				 "ill"
				 :
				 : "r"(sp)
				 : "memory");
	} else {
		__asm__ volatile("mov a1, %0\n\t"
				 "l32i %0, a1, 0"
				 : "+r"(sp)
				 :
				 : "memory");
	}
	child_returned = true;
}

static void assert_nothing_written_below_guard(uintptr_t sp_offset, void *exception)
{
	uint8_t *below = (uint8_t *)child_stacks[1] - CONFIG_XTENSA_DBREAK_STACK_GUARD_SIZE;

	memset(below, 0xa5, CONFIG_XTENSA_DBREAK_STACK_GUARD_SIZE);
	run_child(hit_guard_with_sp_at, sp_offset, exception);
	assert_guard_hit();

	for (uint8_t *p = below; p < (uint8_t *)child_stacks[1]; p++) {
		zassert_equal(*p, 0xa5U, "byte %p below the guard was written", p);
	}
}

/* A guard hit with SP at the guard's lowest byte writes nothing below the guard. */
ZTEST(xtensa_dbreak_stack_guard, test_guard_hit_writes_nothing_below_guard)
{
	assert_nothing_written_below_guard(0U, NULL);
}

/* An exception whose frame save hits the guard writes nothing below the guard. */
ZTEST(xtensa_dbreak_stack_guard, test_nested_guard_hit_writes_nothing_below_guard)
{
	/* With a smaller guard the first store of the frame lands below the guard */
	if (CONFIG_XTENSA_DBREAK_STACK_GUARD_SIZE < 64) {
		ztest_test_skip();
	}

	assert_nothing_written_below_guard(CONFIG_XTENSA_DBREAK_STACK_GUARD_SIZE, (void *)1);
}

static void read_current_guard(const void *unused)
{
	static atomic_t pending;

	ARG_UNUSED(unused);

	/* The aborted ISR leaves the offload interrupt pending, so it runs again */
	if (atomic_cas(&pending, 0, 1)) {
		(void)*(volatile uint8_t *)guard_of(k_current_get());
	} else {
		atomic_clear(&pending);
	}
}

static void read_guard_in_isr(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	irq_offload(read_current_guard, NULL);
	child_returned = true;
}

/* A guard hit in interrupt context is reported without overflowing the debug stack. */
ZTEST(xtensa_dbreak_stack_guard, test_guard_hit_in_isr)
{
	uint8_t *dbg = arch_curr_cpu()->arch.dbreak_stack;
	size_t size = sizeof(arch_curr_cpu()->arch.dbreak_stack);
	size_t unused = 0U;

	memset(dbg, 0xa5, size);
	run_child(read_guard_in_isr, 0U, NULL);
	assert_guard_hit();

	while ((unused < size) && (dbg[unused] == 0xa5U)) {
		unused++;
	}
	TC_PRINT("debug stack: %zu of %zu bytes used\n", size - unused, size);
	zassert_true(unused > 0U, "debug stack overflowed");
}

ZTEST_SUITE(xtensa_dbreak_stack_guard, NULL, NULL, NULL, NULL, NULL);
