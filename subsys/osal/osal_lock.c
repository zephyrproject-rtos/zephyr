/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/osal/osal.h>
#include <zephyr/logging/log.h>

#include "osal_priv.h"

LOG_MODULE_DECLARE(osal, CONFIG_OSAL_LOG_LEVEL);

/*
 * k_spin_lock returns a k_spinlock_key_t (a struct holding one int), which the
 * caller-facing API carries as a uint32_t token. The key is returned to the
 * caller and passed back at unlock, never stashed in the shared control block:
 * stashing would race when two CPUs contend the same lock.
 */
struct osal_spinlock_cb {
	struct k_spinlock lock;
};

osal_spinlock_t osal_spinlock_create(void)
{
	struct osal_spinlock_cb *s = osal_cb_alloc(sizeof(*s));

	if (s == NULL) {
		LOG_ERR("spinlock alloc failed");
		return NULL;
	}

	s->lock = (struct k_spinlock){0};

	return (osal_spinlock_t)s;
}

uint32_t osal_spin_lock(osal_spinlock_t lock)
{
	struct osal_spinlock_cb *s = lock;
	k_spinlock_key_t key;

	if (s == NULL) {
		return 0;
	}

	key = k_spin_lock(&s->lock);

	return (uint32_t)key.key;
}

void osal_spin_unlock(osal_spinlock_t lock, uint32_t key)
{
	struct osal_spinlock_cb *s = lock;
	k_spinlock_key_t k = { .key = (int)key };

	if (s == NULL) {
		return;
	}

	k_spin_unlock(&s->lock, k);
}

void osal_spinlock_delete(osal_spinlock_t lock)
{
	if (lock == NULL) {
		return;
	}

	osal_cb_free(lock);
}

uint32_t osal_irq_lock(void)
{
	return irq_lock();
}

void osal_irq_unlock(uint32_t key)
{
	irq_unlock(key);
}

void osal_sched_lock(void)
{
	k_sched_lock();
}

void osal_sched_unlock(void)
{
	k_sched_unlock();
}

/*
 * Recursive critical section.
 * On SMP the lock state lives in an atomic owner-CPU field: the outermost enter
 * disables local IRQs then CAS-spins to win ownership; a recursive enter on the
 * owning CPU just bumps the counter under the local IRQ lock (no CAS). The
 * outermost exit restores ownership to UNOWNED with a SEQ_CST store before
 * re-enabling IRQs. On non-SMP it reduces to an IRQ lock plus a recursion
 * counter, with the saved key held for the outermost exit.
 */
#define OSAL_CRIT_UNOWNED ((atomic_val_t)-1)

struct osal_crit_cb {
	atomic_t owner_cpu;
	uint32_t count; /* touched only by the owner with IRQs disabled */
	unsigned int saved_key;
};

osal_spinlock_t osal_crit_create(void)
{
	struct osal_crit_cb *c = osal_cb_alloc(sizeof(*c));

	if (c == NULL) {
		LOG_ERR("crit alloc failed");
		return NULL;
	}

	atomic_set(&c->owner_cpu, OSAL_CRIT_UNOWNED);
	c->count = 0;
	c->saved_key = 0;

	return (osal_spinlock_t)c;
}

void osal_crit_enter(osal_spinlock_t lock)
{
	struct osal_crit_cb *c = lock;
	unsigned int key;

	if (c == NULL) {
		return;
	}

	key = arch_irq_lock();

#ifdef CONFIG_SMP
	atomic_val_t me = (atomic_val_t)arch_curr_cpu()->id;

	if (atomic_get(&c->owner_cpu) == me) {
		c->count++;
		arch_irq_unlock(key);
		return;
	}

	/*
	 * Spin with local IRQs held disabled for the whole acquire. Dropping
	 * IRQs between CAS attempts would let an ISR on this CPU re-enter the
	 * same critical section and corrupt count/saved_key.
	 */
	while (!atomic_cas(&c->owner_cpu, OSAL_CRIT_UNOWNED, me)) {
		arch_spin_relax();
	}

	c->saved_key = key;
	c->count = 1;
	/* IRQs stay disabled while the section is held. */
#else
	if (c->count++ == 0) {
		c->saved_key = key;
	} else {
		arch_irq_unlock(key);
	}
#endif
}

void osal_crit_exit(osal_spinlock_t lock)
{
	struct osal_crit_cb *c = lock;

	if (c == NULL || c->count == 0) {
		return;
	}

#ifdef CONFIG_SMP
	if (--c->count == 0) {
		unsigned int key = c->saved_key;

		atomic_set(&c->owner_cpu, OSAL_CRIT_UNOWNED);
		arch_irq_unlock(key);
	}
#else
	if (--c->count == 0) {
		arch_irq_unlock(c->saved_key);
	}
#endif
}

void osal_crit_delete(osal_spinlock_t lock)
{
	if (lock == NULL) {
		return;
	}

	osal_cb_free(lock);
}
