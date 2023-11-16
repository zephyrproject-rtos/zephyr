/*
 * Copyright (c) 2021 Demant A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ATOMIC_ARCH_H_
#define ZEPHYR_INCLUDE_SYS_ATOMIC_ARCH_H_

#include <stdbool.h>
#include <zephyr/sys/atomic_types.h>

/* Included from <atomic.h> */

/* Arch specific atomic primitives */

bool atomic_cas(atomic_t *target, atomic_val_t old_value,
			 atomic_val_t new_value);

bool atomic_ptr_cas(atomic_ptr_t *target, void *old_value,
			      void *new_value);

atomic_val_t atomic_add(atomic_t *target, atomic_val_t value);

atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value);

atomic_val_t atomic_inc(atomic_t *target);

atomic_val_t atomic_dec(atomic_t *target);

atomic_val_t atomic_get(const atomic_t *target);

void *atomic_ptr_get(const atomic_ptr_t *target);

atomic_val_t atomic_set(atomic_t *target, atomic_val_t value);

void *atomic_ptr_set(atomic_ptr_t *target, void *value);

atomic_val_t atomic_clear(atomic_t *target);

void *atomic_ptr_clear(atomic_ptr_t *target);

atomic_val_t atomic_or(atomic_t *target, atomic_val_t value);

atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value);

atomic_val_t atomic_and(atomic_t *target, atomic_val_t value);

atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value);


#endif /* ZEPHYR_INCLUDE_SYS_ATOMIC_ARCH_H_ */
