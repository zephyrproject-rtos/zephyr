/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A thread sets its SP right above its MPU stack guard, with known values
 * in its registers, then makes an instruction or an exception entry push
 * into the guard. The stack overflow report must give the PC, CPSR, SP and
 * registers of the thread.
 */

#include <zephyr/kernel.h>
#include <zephyr/fatal.h>
#include <zephyr/sys/printk.h>

/*
 * SP above the guard, and the size of the push the thread or the exception
 * entry makes:
 * - push {r0-r3}: 16 bytes, all in the guard.
 * - push {r0-r7}: 32 bytes. The data abort entry stores 8 bytes with SRS
 *   then 24 with its push, which hits the guard. The data abort entry
 *   taken on that push does the same, and the next one hits the guard with
 *   its SRS.
 * - An exception entry: 8 bytes with SRS, all in the guard.
 * - push {r0-r8}: 36 bytes, with 32 bytes left. The frame of the data abort
 *   entry fits, the store of its address below the frame hits the guard,
 *   then the SRS of the next data abort entry does.
 * - The IRQ entry with 12 bytes left: its SRS fits, its push hits the guard,
 *   then the SRS of the data abort entry does.
 * - The IRQ entry with 36 bytes left: its frame fits, the store of its
 *   address hits the guard, then the SRS of the data abort entry does.
 */
#if defined(CONFIG_TEST_OVERFLOW_PUSH)
#define SP_OFFSET 4
#define FAULT_INSN "push {r0-r3}\n"
#define FAULT_BEFORE ""
#elif defined(CONFIG_TEST_OVERFLOW_PUSH_PARTIAL)
#define SP_OFFSET 16
#define FAULT_INSN "push {r0-r7}\n"
#define FAULT_BEFORE ""
#elif defined(CONFIG_TEST_OVERFLOW_PUSH_FRAME)
#define SP_OFFSET 32
#define FAULT_INSN "push {r0-r8}\n"
#define FAULT_BEFORE ""
#elif defined(CONFIG_TEST_OVERFLOW_SVC)
/* The SVC entry records the address of the instruction after the SVC */
#define SP_OFFSET 4
#define FAULT_INSN "b .\n"
#define FAULT_BEFORE "svc #0xff\n"
#elif defined(CONFIG_TEST_OVERFLOW_IRQ)
#define SP_OFFSET 4
#define FAULT_INSN "b .\n"
#define FAULT_BEFORE "cpsie i\n"
#elif defined(CONFIG_TEST_OVERFLOW_IRQ_PARTIAL)
#define SP_OFFSET 12
#define FAULT_INSN "b .\n"
#define FAULT_BEFORE "cpsie i\n"
#elif defined(CONFIG_TEST_OVERFLOW_IRQ_FRAME)
#define SP_OFFSET 36
#define FAULT_INSN "b .\n"
#define FAULT_BEFORE "cpsie i\n"
#elif defined(CONFIG_TEST_OVERFLOW_UNDEF)
#define SP_OFFSET 4
#define FAULT_INSN "udf #0\n"
#define FAULT_BEFORE ""
#elif defined(CONFIG_TEST_OVERFLOW_PABT)
#define SP_OFFSET 4
#define FAULT_INSN "bkpt #0\n"
#define FAULT_BEFORE ""
#endif

#define REG_VAL(n) (0x5a5a0000U + (n))

extern const char fault_insn[];

static uint32_t expected_sp;

K_THREAD_STACK_DEFINE(overflow_stack, 1024);
static struct k_thread overflow_thread;

static FUNC_NORETURN __attribute__((naked)) void overflow(uint32_t sp)
{
	__asm__ volatile("cpsid i\n"
			 "mov sp, r0\n"
			 "ldr r0, =%c[r0]\n"
			 "ldr r1, =%c[r1]\n"
			 "ldr r2, =%c[r2]\n"
			 "ldr r3, =%c[r3]\n"
			 "ldr r4, =%c[r4]\n"
			 "ldr r5, =%c[r5]\n"
			 "ldr r6, =%c[r6]\n"
			 "ldr r7, =%c[r7]\n"
			 "ldr r8, =%c[r8]\n"
			 "ldr r9, =%c[r9]\n"
			 "ldr r10, =%c[r10]\n"
			 "ldr r11, =%c[r11]\n"
			 "ldr r12, =%c[r12]\n"
			 "ldr lr, =%c[lr]\n"
			 FAULT_BEFORE
			 ".global fault_insn\n"
			 "fault_insn:\n"
			 FAULT_INSN
			 "b .\n"
			 ".ltorg\n"
			 :
			 : [r0] "i"(REG_VAL(0)), [r1] "i"(REG_VAL(1)), [r2] "i"(REG_VAL(2)),
			   [r3] "i"(REG_VAL(3)), [r4] "i"(REG_VAL(4)), [r5] "i"(REG_VAL(5)),
			   [r6] "i"(REG_VAL(6)), [r7] "i"(REG_VAL(7)), [r8] "i"(REG_VAL(8)),
			   [r9] "i"(REG_VAL(9)), [r10] "i"(REG_VAL(10)),
			   [r11] "i"(REG_VAL(11)), [r12] "i"(REG_VAL(12)),
			   [lr] "i"(REG_VAL(14)));
}

static void overflow_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	expected_sp = k_current_get()->stack_info.start + SP_OFFSET;
	overflow(expected_sp);
}

static bool check(const char *name, uint32_t val, uint32_t expected)
{
	if (val != expected) {
		printk("%s: 0x%08x, expected 0x%08x\n", name, val, expected);
		return false;
	}
	return true;
}

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	bool ok = true;

	ok &= check("reason", reason, K_ERR_STACK_CHK_FAIL);
	ok &= check("pc", esf->basic.pc, POINTER_TO_UINT(fault_insn));
	ok &= check("mode", esf->basic.xpsr & MODE_MASK, MODE_SYS);
	ok &= check("r0", esf->basic.r0, REG_VAL(0));
	ok &= check("r1", esf->basic.r1, REG_VAL(1));
	ok &= check("r2", esf->basic.r2, REG_VAL(2));
	ok &= check("r3", esf->basic.r3, REG_VAL(3));
	ok &= check("r12", esf->basic.r12, REG_VAL(12));
	ok &= check("lr", esf->basic.lr, REG_VAL(14));
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	const struct _callee_saved *callee = esf->extra_info.callee;

	ok &= check("sp", callee->psp, expected_sp);
	ok &= check("r4", callee->v1, REG_VAL(4));
	ok &= check("r5", callee->v2, REG_VAL(5));
	ok &= check("r6", callee->v3, REG_VAL(6));
	ok &= check("r7", callee->v4, REG_VAL(7));
	ok &= check("r8", callee->v5, REG_VAL(8));
	ok &= check("r9", callee->v6, REG_VAL(9));
	ok &= check("r10", callee->v7, REG_VAL(10));
	ok &= check("r11", callee->v8, REG_VAL(11));
#endif

	printk("Stack overflow report %s\n", ok ? "OK" : "FAILED");

	/* Stop here, with the fault context on the abort mode stack for a debugger */
	k_fatal_halt(reason);
}

int main(void)
{
	k_thread_create(&overflow_thread, overflow_stack, K_THREAD_STACK_SIZEOF(overflow_stack),
			overflow_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	/* The IRQ cases need an interrupt: wake up from this sleep */
	k_sleep(K_MSEC(100));
	printk("No stack overflow\n");

	return 0;
}
