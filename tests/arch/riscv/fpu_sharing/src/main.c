/*
 * Copyright (c) 2023 BayLibre SAS
 * Written by: Nicolas Pitre
 * SPDX-License-Identifier: Apache-2.0
 *
 * The purpose of this test is to exercize and validate the on-demand and
 * preemptive FPU access algorithms implemented in arch/riscv/core/fpu.c.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>

static inline unsigned long fpu_state(void)
{
	return csr_read(mstatus) & MSTATUS_FS;
}

static inline bool fpu_is_off(void)
{
	return fpu_state() == MSTATUS_FS_OFF;
}

static inline bool fpu_is_clean(void)
{
	unsigned long state = fpu_state();

	return state == MSTATUS_FS_INIT || state == MSTATUS_FS_CLEAN;
}

static inline bool fpu_is_dirty(void)
{
	return fpu_state() == MSTATUS_FS_DIRTY;
}

/*
 * Test for basic FPU access states.
 */

ZTEST(riscv_fpu_sharing, test_basics)
{
	int32_t val;

	/* write to a FP reg */
	__asm__ volatile ("fcvt.s.w fa0, %0" : : "r" (42) : "fa0");

	/* the FPU should be dirty now */
	zassert_true(fpu_is_dirty());

	/* flush the FPU and disable it */
	zassert_true(k_float_disable(k_current_get()) == 0);
	zassert_true(fpu_is_off());

	/* read the FP reg back which should re-enable the FPU */
	__asm__ volatile ("fcvt.w.s %0, fa0, rtz" : "=r" (val));

	/* the FPU should be enabled now but not dirty */
	zassert_true(fpu_is_clean());

	/* we should have retrieved the same value */
	zassert_true(val == 42, "got %d instead", val);
}

/*
 * Test for FPU contention between threads.
 */

static void new_thread_check(const char *name)
{
	int32_t val;

	/* threads should start with the FPU disabled */
	zassert_true(fpu_is_off(), "FPU not off when starting thread %s", name);

	/* read one FP reg */
#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
	/*
	 * Registers are initialized with zeroes but single precision values
	 * are expected to be "NaN-boxed" to be valid. So don't use the s
	 * format here as it won't convert to zero. It's not a problem
	 * otherwise as proper code is not supposed to rely on unitialized
	 * registers anyway.
	 */
	__asm__ volatile ("fcvt.w.d %0, fa0, rtz" : "=r" (val));
#else
	__asm__ volatile ("fcvt.w.s %0, fa0, rtz" : "=r" (val));
#endif

	/* the FPU should be enabled now and not dirty */
	zassert_true(fpu_is_clean(), "FPU not clean after read");

	/* the FP regs are supposed to be zero initialized */
	zassert_true(val == 0, "got %d instead", val);
}

static K_SEM_DEFINE(thread1_sem, 0, 1);
static K_SEM_DEFINE(thread2_sem, 0, 1);

#define STACK_SIZE 2048
static K_THREAD_STACK_DEFINE(thread1_stack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(thread2_stack, STACK_SIZE);

static struct k_thread thread1;
static struct k_thread thread2;

static void thread1_entry(void *p1, void *p2, void *p3)
{
	int32_t val;

	/*
	 * Test 1: Wait for thread2 to let us run and make sure we still own the
	 * FPU afterwards.
	 */
	new_thread_check("thread1");
	zassert_true(fpu_is_clean());
	k_sem_take(&thread1_sem, K_FOREVER);
	zassert_true(fpu_is_clean());

	/*
	 * Test 2: Let thread2 do its initial thread checks. When we're
	 * scheduled again, thread2 should be the FPU owner at that point
	 * meaning the FPU should then be off for us.
	 */
	k_sem_give(&thread2_sem);
	k_sem_take(&thread1_sem, K_FOREVER);
	zassert_true(fpu_is_off());

	/*
	 * Test 3: Let thread2 verify that it still owns the FPU.
	 */
	k_sem_give(&thread2_sem);
	k_sem_take(&thread1_sem, K_FOREVER);
	zassert_true(fpu_is_off());

	/*
	 * Test 4: Dirty the FPU for ourself. Schedule to thread2 which won't
	 * touch the FPU. Make sure we still own the FPU in dirty state when
	 * we are scheduled back.
	 */
	__asm__ volatile ("fcvt.s.w fa1, %0" : : "r" (42) : "fa1");
	zassert_true(fpu_is_dirty());
	k_sem_give(&thread2_sem);
	k_sem_take(&thread1_sem, K_FOREVER);
	zassert_true(fpu_is_dirty());

	/*
	 * Test 5: Because we currently own a dirty FPU, we are considered
	 * an active user. This means we should still own it after letting
	 * thread1 use it as it would be preemptively be restored, but in a
	 * clean state then.
	 */
	k_sem_give(&thread2_sem);
	k_sem_take(&thread1_sem, K_FOREVER);
	zassert_true(fpu_is_clean());

	/*
	 * Test 6: Avoid dirtying the FPU (we'll just make sure it holds our
	 * previously written value). Because thread2 had dirtied it in
	 * test 5, it is considered an active user. Scheduling thread2 will
	 * make it own the FPU right away. However we won't preemptively own
	 * it anymore afterwards as we didn't actively used it this time.
	 */
	__asm__ volatile ("fcvt.w.s %0, fa1, rtz" : "=r" (val));
	zassert_true(val == 42, "got %d instead", val);
	zassert_true(fpu_is_clean());
	k_sem_give(&thread2_sem);
	k_sem_take(&thread1_sem, K_FOREVER);
	zassert_true(fpu_is_off());

	/*
	 * Test 7: Just let thread2 run again. Even if it is no longer an
	 * active user, it should still own the FPU as it is not contended.
	 */
	k_sem_give(&thread2_sem);
}

static void thread2_entry(void *p1, void *p2, void *p3)
{
	int32_t val;

	/*
	 * Test 1: thread1 waits until we're scheduled.
	 * Let it run again without doing anything else for now.
	 */
	k_sem_give(&thread1_sem);

	/*
	 * Test 2: Perform the initial thread check and return to thread1.
	 */
	k_sem_take(&thread2_sem, K_FOREVER);
	new_thread_check("thread2");
	k_sem_give(&thread1_sem);

	/*
	 * Test 3: Make sure we still own the FPU when scheduled back.
	 */
	k_sem_take(&thread2_sem, K_FOREVER);
	zassert_true(fpu_is_clean());
	k_sem_give(&thread1_sem);

	/*
	 * Test 4: Confirm that thread1 took the FPU from us.
	 */
	k_sem_take(&thread2_sem, K_FOREVER);
	zassert_true(fpu_is_off());
	k_sem_give(&thread1_sem);

	/*
	 * Test 5: Take ownership of the FPU by using it.
	 */
	k_sem_take(&thread2_sem, K_FOREVER);
	zassert_true(fpu_is_off());
	__asm__ volatile ("fcvt.s.w fa1, %0" : : "r" (37) : "fa1");
	zassert_true(fpu_is_dirty());
	k_sem_give(&thread1_sem);

	/*
	 * Test 6: We dirtied the FPU last time therefore we are an active
	 * user. We should own it right away but clean this time.
	 */
	k_sem_take(&thread2_sem, K_FOREVER);
	zassert_true(fpu_is_clean());
	__asm__ volatile ("fcvt.w.s %0, fa1" : "=r" (val));
	zassert_true(val == 37, "got %d instead", val);
	zassert_true(fpu_is_clean());
	k_sem_give(&thread1_sem);

	/*
	 * Test 7: thread1 didn't claim the FPU and it wasn't preemptively
	 * assigned to it. This means we should still own it despite not
	 * having been an active user lately as the FPU is not contended.
	 */
	k_sem_take(&thread2_sem, K_FOREVER);
	zassert_true(fpu_is_clean());
	__asm__ volatile ("fcvt.w.s %0, fa1" : "=r" (val));
	zassert_true(val == 37, "got %d instead", val);
}

ZTEST(riscv_fpu_sharing, test_multi_thread_interaction)
{
	k_thread_create(&thread1, thread1_stack, STACK_SIZE,
			thread1_entry, NULL, NULL, NULL,
			-1, 0, K_NO_WAIT);
	k_thread_create(&thread2, thread2_stack, STACK_SIZE,
			thread2_entry, NULL, NULL, NULL,
			-1, 0, K_NO_WAIT);
	zassert_true(k_thread_join(&thread1, K_FOREVER) == 0);
	zassert_true(k_thread_join(&thread2, K_FOREVER) == 0);
}

/*
 * Test for thread vs exception interactions.
 *
 * Context switching for userspace threads always happens through an
 * exception. Privileged preemptive threads also get preempted through
 * an exception. Same for ISRs and system calls. This test reproduces
 * the conditions for those cases.
 */

#define NO_FPU		NULL
#define WITH_FPU	(const void *)1

static void exception_context(const void *arg)
{
	/* All exxceptions should always have the FPU disabled initially */
	zassert_true(fpu_is_off());

	if (arg == NO_FPU) {
		return;
	}

	/* Simulate a user syscall environment by having IRQs enabled */
	csr_set(mstatus, MSTATUS_IEN);

	/* make sure the FPU is still off */
	zassert_true(fpu_is_off());

	/* write to an FPU register */
	__asm__ volatile ("fcvt.s.w fa1, %0" : : "r" (987) : "fa1");

	/* the FPU state should be dirty now */
	zassert_true(fpu_is_dirty());

	/* IRQs should have been disabled on us to prevent recursive FPU usage */
	zassert_true((csr_read(mstatus) & MSTATUS_IEN) == 0, "IRQs should be disabled");
}

ZTEST(riscv_fpu_sharing, test_thread_vs_exc_interaction)
{
	int32_t val;

	/* Ensure the FPU is ours and dirty. */
	__asm__ volatile ("fcvt.s.w fa1, %0" : : "r" (654) : "fa1");
	zassert_true(fpu_is_dirty());

	/* We're not in exception so IRQs should be enabled. */
	zassert_true((csr_read(mstatus) & MSTATUS_IEN) != 0, "IRQs should be enabled");

	/* Exceptions with no FPU usage shouldn't affect our state. */
	irq_offload(exception_context, NO_FPU);
	zassert_true((csr_read(mstatus) & MSTATUS_IEN) != 0, "IRQs should be enabled");
	zassert_true(fpu_is_dirty());
	__asm__ volatile ("fcvt.w.s %0, fa1" : "=r" (val));
	zassert_true(val == 654, "got %d instead", val);

	/*
	 * Exceptions with FPU usage should be trapped to save our context
	 * before letting its accesses go through. Because our FPU state
	 * is dirty at the moment of the trap, we are considered to be an
	 * active user and the FPU context should be preemptively restored
	 * upon leaving the exception, but with a clean state at that point.
	 */
	irq_offload(exception_context, WITH_FPU);
	zassert_true((csr_read(mstatus) & MSTATUS_IEN) != 0, "IRQs should be enabled");
	zassert_true(fpu_is_clean());
	__asm__ volatile ("fcvt.w.s %0, fa1" : "=r" (val));
	zassert_true(val == 654, "got %d instead", val);

	/*
	 * Do the exception with FPU usage again, but this time our current
	 * FPU state is clean, meaning we're no longer an active user.
	 * This means our FPU context should not be preemptively restored.
	 */
	irq_offload(exception_context, WITH_FPU);
	zassert_true((csr_read(mstatus) & MSTATUS_IEN) != 0, "IRQs should be enabled");
	zassert_true(fpu_is_off());

	/* Make sure we still have proper context when accessing the FPU. */
	__asm__ volatile ("fcvt.w.s %0, fa1" : "=r" (val));
	zassert_true(fpu_is_clean());
	zassert_true(val == 654, "got %d instead", val);
}

/*
 * Test for proper FPU instruction trap.
 *
 * There is no dedicated FPU trap flag bit on RISC-V. FPU specific opcodes
 * must be looked for when an illegal instruction exception is raised.
 * This is done in arch/riscv/core/isr.S and explicitly tested here.
 */

#define TEST_TRAP(insn) \
	/* disable the FPU access */ \
	zassert_true(k_float_disable(k_current_get()) == 0); \
	zassert_true(fpu_is_off()); \
	/* execute the instruction */ \
	{ \
		/* use a0 to be universal with all configs */ \
		register unsigned long __r __asm__ ("a0") = reg; \
		PRE_INSN \
		__asm__ volatile (insn : "+r" (__r) : : "fa0", "fa1", "memory"); \
		POST_INSN \
		reg = __r; \
	} \
	/* confirm that the FPU state has changed */ \
	zassert_true(!fpu_is_off())

ZTEST(riscv_fpu_sharing, test_fp_insn_trap)
{
	unsigned long reg;
	uint32_t buf;

	/* Force non RVC instructions */
	#define PRE_INSN  __asm__ volatile (".option push; .option norvc");
	#define POST_INSN __asm__ volatile (".option pop");

	/* OP-FP major opcode space */
	reg = 123456;
	TEST_TRAP("fcvt.s.w fa1, %0");
	TEST_TRAP("fadd.s fa0, fa1, fa1");
	TEST_TRAP("fcvt.w.s %0, fa0");
	zassert_true(reg == 246912, "got %ld instead", reg);

	/* LOAD-FP / STORE-FP space */
	buf = 0x40490ff9; /* 3.1416 */
	reg = (unsigned long)&buf;
	TEST_TRAP("flw fa1, 0(%0)");
	TEST_TRAP("fadd.s fa0, fa0, fa1, rtz");
	TEST_TRAP("fsw fa0, 0(%0)");
	zassert_true(buf == 0x487120c9 /* 246915.140625 */, "got %#x instead", buf);

	/* CSR with fcsr, frm and fflags */
	TEST_TRAP("frcsr %0");
	TEST_TRAP("fscsr %0");
	TEST_TRAP("frrm %0");
	TEST_TRAP("fsrm %0");
	TEST_TRAP("frflags %0");
	TEST_TRAP("fsflags %0");

	/* lift restriction on RVC instructions */
	#undef PRE_INSN
	#define PRE_INSN
	#undef POST_INSN
	#define POST_INSN

	/* RVC variants */
#if defined(CONFIG_RISCV_ISA_EXT_C)
#if !defined(CONFIG_64BIT)
	/* only available on RV32 */
	buf = 0x402df8a1; /* 2.7183 */
	reg = (unsigned long)&buf;
	TEST_TRAP("c.flw fa1, 0(%0)");
	TEST_TRAP("fadd.s fa0, fa0, fa1");
	TEST_TRAP("c.fsw fa0, 0(%0)");
	zassert_true(buf == 0x48712177 /* 246917.859375 */, "got %#x instead", buf);
#endif
#if defined(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
	uint64_t buf64;

	buf64 = 0x400921ff2e48e8a7LL;
	reg = (unsigned long)&buf64;
	TEST_TRAP("c.fld fa0, 0(%0)");
	TEST_TRAP("fadd.d fa1, fa0, fa0, rtz");
	TEST_TRAP("fadd.d fa1, fa1, fa0, rtz");
	TEST_TRAP("c.fsd fa1, 0(%0)");
	zassert_true(buf64 == 0x4022d97f62b6ae7dLL /* 9.4248 */,
		     "got %#llx instead", buf64);
#endif
#endif /* CONFIG_RISCV_ISA_EXT_C */

	/* MADD major opcode space */
	reg = 3579;
	TEST_TRAP("fcvt.s.w fa1, %0");
	TEST_TRAP("fmadd.s fa0, fa1, fa1, fa1");
	TEST_TRAP("fcvt.w.s %0, fa0");
	zassert_true(reg == 12812820, "got %ld instead", reg);

	/* MSUB major opcode space */
	reg = 1234;
	TEST_TRAP("fcvt.s.w fa1, %0");
	TEST_TRAP("fmsub.s fa0, fa1, fa1, fa0");
	TEST_TRAP("fcvt.w.s %0, fa0");
	zassert_true(reg == -11290064, "got %ld instead", reg);

	/* NMSUB major opcode space */
	reg = -23;
	TEST_TRAP("fcvt.s.w fa1, %0");
	TEST_TRAP("fnmsub.s fa0, fa1, fa1, fa0");
	TEST_TRAP("fcvt.w.s %0, fa0");
	zassert_true(reg == -11290593, "got %ld instead", reg);

	/* NMADD major opcode space */
	reg = 765;
	TEST_TRAP("fcvt.s.w fa1, %0");
	TEST_TRAP("fnmadd.s fa0, fa1, fa1, fa1");
	TEST_TRAP("fcvt.w.s %0, fa0");
	zassert_true(reg == -585990, "got %ld instead", reg);
}

ZTEST_SUITE(riscv_fpu_sharing, NULL, NULL, NULL, NULL, NULL);
