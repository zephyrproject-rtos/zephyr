/*
 * Copyright (c) 2018 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <spinlock.h>

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
