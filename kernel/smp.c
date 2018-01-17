/*
 * Copyright (c) 2018 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <spinlock.h>
#include <kswap.h>
#include <nano_internal.h>

static struct k_spinlock global_spinlock;

static volatile int recursive_count;

/* FIXME: this value of key works on all known architectures as an
 * "invalid state" that will never be legitimately returned from
 * _arch_irq_lock().  But we should force the architecture code to
 * define something for us.
 */
#define KEY_RECURSIVE 0xffffffff

unsigned int _smp_global_lock(void)
{
	/* OK to test this outside the lock.  If it's non-zero, then
	 * we hold the lock by definition
	 */
	if (recursive_count) {
		recursive_count++;

		return KEY_RECURSIVE;
	}

	unsigned int k = k_spin_lock(&global_spinlock).key;

	recursive_count = 1;
	return k;
}

void _smp_global_unlock(unsigned int key)
{
	if (key == KEY_RECURSIVE) {
		recursive_count--;
		return;
	}

	k_spinlock_key_t sk = { .key = key };

	recursive_count = 0;
	k_spin_unlock(&global_spinlock, sk);
}

extern k_thread_stack_t _interrupt_stack1[];
extern k_thread_stack_t _interrupt_stack2[];
extern k_thread_stack_t _interrupt_stack3[];

#ifdef CONFIG_SMP
static void _smp_init_top(int key, void *arg)
{
	atomic_t *start_flag = arg;

	/* Wait for the signal to begin scheduling */
	do {
		k_busy_wait(100);
	} while (!atomic_get(start_flag));

	/* Switch out of a dummy thread.  Trick cribbed from the main
	 * thread init.  Should probably unify implementations.
	 */
	struct k_thread dummy_thread = {
		.base.user_options = K_ESSENTIAL,
		.base.thread_state = _THREAD_DUMMY,
	};

	_arch_curr_cpu()->current = &dummy_thread;
	_Swap(irq_lock());

	CODE_UNREACHABLE;
}
#endif

void smp_init(void)
{
	atomic_t start_flag;

	atomic_clear(&start_flag);

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
	_arch_start_cpu(1, _interrupt_stack1, CONFIG_ISR_STACK_SIZE,
			_smp_init_top, &start_flag);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 2
	_arch_start_cpu(2, _interrupt_stack2, CONFIG_ISR_STACK_SIZE,
			_smp_init_top, &start_flag);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 3
	_arch_start_cpu(3, _interrupt_stack3, CONFIG_ISR_STACK_SIZE,
			_smp_init_top, &start_flag);
#endif

	atomic_set(&start_flag, 1);
}
