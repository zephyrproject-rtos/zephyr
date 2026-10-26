/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xenlib Operating System Abstraction Layer (OSAL) atomic variables runtime
 * interface implementation
 *
 */
#include <xenlib_osal_platform.h>

void xenlib_osal_atomic_set_bit(xenlib_osal_atomic_t *target, unsigned int bit)
{
	atomic_set_bit(target, bit);
}

void xenlib_osal_atomic_clear_bit(xenlib_osal_atomic_t *target,
					unsigned int bit)
{
	atomic_clear_bit(target, bit);
}

xenlib_osal_atomic_val_t xenlib_osal_atomic_set(xenlib_osal_atomic_t *target,
					xenlib_osal_atomic_val_t value)
{
	return atomic_set(target, value);
}

xenlib_osal_atomic_val_t xenlib_osal_atomic_clear(xenlib_osal_atomic_t *target)
{
	return atomic_clear(target);
}

bool xenlib_osal_atomic_test_and_clear_bit(xenlib_osal_atomic_t *target,
					unsigned int bit)
{
	return atomic_test_and_clear_bit(target, bit);
}

xenlib_osal_atomic_val_t xenlib_osal_atomic_get(
					const xenlib_osal_atomic_t *target)
{
	return atomic_get(target);
}
