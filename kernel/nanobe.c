/*
 * Copyright (c) 2016-2026 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Nanobe context initialization, scheduler, sleep, semaphores and ISR
 *        injection
 *
 * The architecture specific context switch (nanobe_switch()), the nanobe
 * entry trampoline and the injection syringe are implemented in assembly,
 * see arch/arm/core/cortex_m/nanobe.S.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel/nanobe.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include <cmsis_core.h>

/* Context frame saved on a nanobe stack by nanobe_switch(), lowest address
 * first. Must match the push/pop sequence in nanobe.S.
 */
struct nanobe_frame {
	uint32_t igrd;
#if defined(CONFIG_NANOBE_FPU)
	uint32_t s16_s31[16];
#endif
	uint32_t r8, r9, r10, r11;
	uint32_t r4, r5, r6, r7;
	uint32_t pc;
};

BUILD_ASSERT((sizeof(struct nanobe_frame) % NANOBE_STACK_ALIGN) == 0);

/* Nanobe entry trampoline, calls the entry function held in r4 with the
 * argument held in r5.
 */
extern void z_nanobe_entry(void);

/* Scheduler lock (guard) and pending yield (trigger), shared with nanobe.S */
volatile uint8_t z_nanobe_sgrd;
volatile uint8_t z_nanobe_strg;

/* Injection in progress (guard), injection re-run (trigger), the return
 * address and the callee of the injected call. Shared with nanobe.S.
 */
volatile uint8_t z_nanobe_igrd;
volatile uint8_t z_nanobe_itrg;
volatile uint32_t z_nanobe_iret;
volatile nanobe_t z_nanobe_ical;

/* Injection syringe, executed in thread mode on the interrupted stack */
extern void z_nanobe_syringe(void);

static struct k_thread *volatile nanobe_owner;

static inline bool owner_set(void)
{
	if (nanobe_owner == NULL) {
		nanobe_owner = k_current_get();
	}

	return nanobe_owner == k_current_get();
}

static inline bool owner_is_current(void)
{
	return (nanobe_owner != NULL) && (nanobe_owner == k_current_get());
}

/* Ready queue, linked using the word just below each saved stack pointer,
 * which is unused stack space of the suspended context.
 */
static struct {
	void *head;
	void *tail;
	void *prev;
} volatile sched;

static inline void **sched_next(void *nanobe_sp)
{
	return (void **)nanobe_sp - 1;
}

void *z_nanobe_frame_init(nanobe_arg_t entry, void *arg, void *stack_top)
{
	struct nanobe_frame *frame;
	uintptr_t top;

	__ASSERT_NO_MSG(entry != NULL);
	__ASSERT_NO_MSG(stack_top != NULL);

	top = ROUND_DOWN((uintptr_t)stack_top, NANOBE_STACK_ALIGN);
	frame = (struct nanobe_frame *)top - 1;

	(void)memset(frame, 0, sizeof(*frame));
	frame->r4 = (uint32_t)(uintptr_t)entry;
	frame->r5 = (uint32_t)(uintptr_t)arg;
	frame->pc = (uint32_t)(uintptr_t)z_nanobe_entry;

	return frame;
}

void *nanobe_init_arg(nanobe_arg_t entry, void *arg, void *stack_top)
{
	bool owner;

	owner = owner_set();
	__ASSERT(owner, "nanobe used from a thread other than the owner thread");
	ARG_UNUSED(owner);

	return z_nanobe_frame_init(entry, arg, stack_top);
}

void *nanobe_init(nanobe_t entry, void *stack_top)
{
	/* Calling a void (void) function with an argument is harmless under
	 * the AAPCS, the argument register is caller-saved.
	 */
	return nanobe_init_arg((nanobe_arg_t)(uintptr_t)entry, NULL, stack_top);
}

FUNC_NORETURN void z_nanobe_exit(void)
{
	__ASSERT(false, "nanobe entry function returned");

	k_panic();
	CODE_UNREACHABLE;
}

void nanobe_sched_enqueue(void *nanobe_sp)
{
	__ASSERT_NO_MSG(nanobe_sp != NULL);

	*sched_next(nanobe_sp) = NULL;
	if (sched.tail != NULL) {
		*sched_next(sched.tail) = nanobe_sp;
	} else {
		sched.head = nanobe_sp;
	}
	sched.tail = nanobe_sp;
}

static void *sched_dequeue(void)
{
	void *nanobe_sp;

	nanobe_sp = sched.head;
	if (nanobe_sp != NULL) {
		sched.head = *sched_next(nanobe_sp);
		if (sched.head == NULL) {
			sched.tail = NULL;
		}
	}

	return nanobe_sp;
}

void nanobe_sched_yield(void)
{
	void *ready;

	if (z_nanobe_sgrd != 0U) {
		return;
	}
	z_nanobe_sgrd = 1U;
	compiler_barrier();

	__ASSERT(owner_is_current(),
		 "nanobe used from a thread other than the owner thread");

	if (sched.prev != NULL) {
		nanobe_sched_enqueue(sched.prev);
		sched.prev = NULL;
	}

	ready = sched_dequeue();
	if (ready == NULL) {
		compiler_barrier();
		z_nanobe_sgrd = 0U;

		return;
	}

	/* Scheduler lock is released by nanobe_switch() in the resumed
	 * context.
	 */
	nanobe_switch(ready, (void **)&sched.prev);
}

uint8_t nanobe_sched_lock(void)
{
	uint8_t lock;

	lock = z_nanobe_sgrd;
	z_nanobe_sgrd = 1U;
	compiler_barrier();

	return lock;
}

void nanobe_sched_unlock(uint8_t lock)
{
	if (lock != 0U) {
		return;
	}

	compiler_barrier();
	z_nanobe_sgrd = 0U;
	compiler_barrier();

	if (z_nanobe_strg != 0U) {
		z_nanobe_strg = 0U;
		compiler_barrier();

		nanobe_sched_yield();
	}
}

void nanobe_sleep(k_timeout_t timeout)
{
	k_timepoint_t end;

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		nanobe_sched_yield();

		return;
	}

	end = sys_timepoint_calc(timeout);
	do {
		nanobe_sched_yield();
	} while (!sys_timepoint_expired(end));
}

int nanobe_sem_init(struct nanobe_sem *sem, uint32_t initial_count,
		    uint32_t limit)
{
	if ((limit == 0U) || (initial_count > limit)) {
		return -EINVAL;
	}

	sem->count = initial_count;
	sem->limit = limit;

	return 0;
}

static bool sem_try_take(struct nanobe_sem *sem)
{
	unsigned int key;
	bool taken;

	key = irq_lock();
	taken = (sem->count != 0U);
	if (taken) {
		sem->count--;
	}
	irq_unlock(key);

	return taken;
}

int nanobe_sem_take(struct nanobe_sem *sem, k_timeout_t timeout)
{
	k_timepoint_t end;

	if (sem_try_take(sem)) {
		return 0;
	}

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return -EBUSY;
	}

	end = sys_timepoint_calc(timeout);
	do {
		nanobe_sched_yield();

		if (sem_try_take(sem)) {
			return 0;
		}
	} while (!sys_timepoint_expired(end));

	return -EAGAIN;
}

void nanobe_sem_give(struct nanobe_sem *sem)
{
	unsigned int key;

	key = irq_lock();
	if (sem->count < sem->limit) {
		sem->count++;
	}
	irq_unlock(key);
}

/* Stacked xPSR ICI/IT bits, EPSR[26:25] and EPSR[15:10] */
#define NANOBE_XPSR_ICI_IT_MASK 0x0600FC00UL

/* Stacked xPSR Thumb bit, EPSR[24] */
#define NANOBE_XPSR_T BIT(24)

/* Offsets, in words, of the return address and xPSR in the basic exception
 * stack frame; same in the extended (FPU) frame.
 */
#define NANOBE_ESF_PC   6
#define NANOBE_ESF_XPSR 7

static int isr_inject(nanobe_t callee, bool owner)
{
	uint32_t *esf;

	__ASSERT_NO_MSG(callee != NULL);

	/* Only when returning to thread mode, i.e. not a nested exception */
	if ((SCB->ICSR & SCB_ICSR_RETTOBASE_Msk) == 0U) {
		return -EPERM;
	}

	/* Interrupted thread mode code uses the PSP, the exception stack frame
	 * is at the top of the process stack.
	 */
	esf = (uint32_t *)__get_PSP();

	/* An interruptible-continuable instruction or IT block state cannot be
	 * restored other than by exception return.
	 */
	if ((esf[NANOBE_ESF_XPSR] & NANOBE_XPSR_ICI_IT_MASK) != 0U) {
		return -EBUSY;
	}

	if (owner && (z_nanobe_sgrd != 0U)) {
		/* Scheduler locked, yield on unlock */
		z_nanobe_strg = 1U;

		return 0;
	}

	if (z_nanobe_igrd != 0U) {
		/* Injection in progress, re-run on completion. A call not
		 * injected by the nanobe owner, i.e. a thread reschedule, is
		 * re-run instead of the in-progress call so that it is not
		 * lost.
		 */
		if (!owner) {
			z_nanobe_ical = callee;
		}
		z_nanobe_itrg = 1U;

		return 0;
	}

	z_nanobe_igrd = 1U;

	/* Syringe returns using a Thumb interworking pop {pc} */
	z_nanobe_iret = esf[NANOBE_ESF_PC] | 1U;
	z_nanobe_ical = callee;

	/* Exception return address must have bit[0] cleared */
	esf[NANOBE_ESF_PC] = (uint32_t)(uintptr_t)z_nanobe_syringe & ~1UL;

	/* Syringe is Thumb code, also when the interrupted context faulted on
	 * an invalid EPSR.T, e.g. a thread being aborted.
	 */
	esf[NANOBE_ESF_XPSR] |= NANOBE_XPSR_T;

	return 0;
}

#if defined(CONFIG_NANOBE_INJECTION)
int nanobe_isr_inject(nanobe_t callee)
{
	/* Only into the nanobe owner thread */
	if (!owner_is_current()) {
		return -EPERM;
	}

	return isr_inject(callee, true);
}
#endif /* CONFIG_NANOBE_INJECTION */

void z_nanobe_isr_iciit_discard(void)
{
	uint32_t *esf = (uint32_t *)__get_PSP();

	esf[NANOBE_ESF_XPSR] &= ~NANOBE_XPSR_ICI_IT_MASK;
}

int z_nanobe_isr_inject(nanobe_t callee)
{
	return isr_inject(callee, false);
}
