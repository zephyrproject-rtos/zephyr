/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <cy_device.h>

#define IPC_ATOMIC_STRUCT CY_IPC_STRUCT_PTR(CPUSS_IPC_IPC_NR - 1U)

static ALWAYS_INLINE unsigned int ipc_lock(void)
{
	unsigned int key = arch_irq_lock();

	/* Reading ACQUIRE tries to take the lock */
	while ((REG_IPC_STRUCT_ACQUIRE(IPC_ATOMIC_STRUCT) & IPC_STRUCT_ACQUIRE_SUCCESS_Msk) == 0U) {
	}
	__DMB();

	return key;
}

static ALWAYS_INLINE void ipc_unlock(unsigned int key)
{
	__DMB();
	REG_IPC_STRUCT_RELEASE(IPC_ATOMIC_STRUCT) = 0U;
	arch_irq_unlock(key);
}

/* The lock may still be held from before a reset */
void cyt4dn_atomic_init(void)
{
	REG_IPC_STRUCT_RELEASE(IPC_ATOMIC_STRUCT) = 0U;
	__DMB();
}

#define ATOMIC_RMW(target, expr)                                                                   \
	({                                                                                         \
		unsigned int key = ipc_lock();                                                     \
		atomic_val_t old = *(target);                                                      \
                                                                                                   \
		*(target) = (expr);                                                                \
		ipc_unlock(key);                                                                   \
		old;                                                                               \
	})

bool atomic_cas(atomic_t *target, atomic_val_t old_value, atomic_val_t new_value)
{
	unsigned int key = ipc_lock();
	bool ret = (*target == old_value);

	if (ret) {
		*target = new_value;
	}
	ipc_unlock(key);

	return ret;
}

bool atomic_ptr_cas(atomic_ptr_t *target, void *old_value, void *new_value)
{
	unsigned int key = ipc_lock();
	bool ret = (*target == old_value);

	if (ret) {
		*target = new_value;
	}
	ipc_unlock(key);

	return ret;
}

atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	return ATOMIC_RMW(target, old + value);
}

atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value)
{
	return ATOMIC_RMW(target, old - value);
}

atomic_val_t atomic_inc(atomic_t *target)
{
	return atomic_add(target, 1);
}

atomic_val_t atomic_dec(atomic_t *target)
{
	return atomic_sub(target, 1);
}

atomic_val_t atomic_get(const atomic_t *target)
{
	return *(volatile const atomic_t *)target;
}

void *atomic_ptr_get(const atomic_ptr_t *target)
{
	return *(void *volatile const *)target;
}

atomic_val_t atomic_set(atomic_t *target, atomic_val_t value)
{
	return ATOMIC_RMW(target, value);
}

void *atomic_ptr_set(atomic_ptr_t *target, void *value)
{
	unsigned int key = ipc_lock();
	void *old = *target;

	*target = value;
	ipc_unlock(key);

	return old;
}

atomic_val_t atomic_clear(atomic_t *target)
{
	return atomic_set(target, 0);
}

void *atomic_ptr_clear(atomic_ptr_t *target)
{
	return atomic_ptr_set(target, NULL);
}

atomic_val_t atomic_or(atomic_t *target, atomic_val_t value)
{
	return ATOMIC_RMW(target, old | value);
}

atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value)
{
	return ATOMIC_RMW(target, old ^ value);
}

atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	return ATOMIC_RMW(target, old & value);
}

atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value)
{
	return ATOMIC_RMW(target, ~(old & value));
}
