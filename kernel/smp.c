/*
 * Copyright (c) 2018 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <spinlock.h>
#include <kswap.h>
#include <kernel_internal.h>

#ifdef CONFIG_SMP
static atomic_t global_lock;

unsigned int _smp_global_lock(void)
{
	unsigned int key = _arch_irq_lock();

	if (!_current->base.global_lock_count) {
		while (!atomic_cas(&global_lock, 0, 1)) {
		}
	}

	_current->base.global_lock_count++;

	return key;
}

void _smp_global_unlock(unsigned int key)
{
	if (_current->base.global_lock_count) {
		_current->base.global_lock_count--;

		if (!_current->base.global_lock_count) {
			atomic_clear(&global_lock);
		}
	}

	_arch_irq_unlock(key);
}

void _smp_reacquire_global_lock(struct k_thread *thread)
{
	if (thread->base.global_lock_count) {
		_arch_irq_lock();

		while (!atomic_cas(&global_lock, 0, 1)) {
		}
	}
}


/* Called from within _Swap(), so assumes lock already held */
void _smp_release_global_lock(struct k_thread *thread)
{
	if (!thread->base.global_lock_count) {
		atomic_clear(&global_lock);
	}
}

#endif

extern k_thread_stack_t _interrupt_stack1[];
extern k_thread_stack_t _interrupt_stack2[];
extern k_thread_stack_t _interrupt_stack3[];

#ifdef CONFIG_SMP
static void smp_init_top(int key, void *arg)
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
	unsigned int k = irq_lock();
	smp_timer_init();
	(void)_Swap(k);

	CODE_UNREACHABLE;
}
#endif

void smp_init(void)
{
	atomic_t start_flag;

	(void)atomic_clear(&start_flag);

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
	_arch_start_cpu(1, _interrupt_stack1, CONFIG_ISR_STACK_SIZE,
			smp_init_top, &start_flag);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 2
	_arch_start_cpu(2, _interrupt_stack2, CONFIG_ISR_STACK_SIZE,
			smp_init_top, &start_flag);
#endif

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 3
	_arch_start_cpu(3, _interrupt_stack3, CONFIG_ISR_STACK_SIZE,
			smp_init_top, &start_flag);
#endif

	(void)atomic_set(&start_flag, 1);
}
