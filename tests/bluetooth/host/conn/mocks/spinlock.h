/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>

/* List of fakes used by this unit tester */
#define SPINLOCK_MOCKS_FFF_FAKES_LIST(FAKE)                                                        \
	FAKE(z_spin_lock_valid)                                                                    \
	FAKE(z_spin_unlock_valid)                                                                  \
	FAKE(z_spin_lock_set_owner)

DECLARE_FAKE_VALUE_FUNC(bool, z_spin_lock_valid, struct k_spinlock *);
DECLARE_FAKE_VALUE_FUNC(bool, z_spin_unlock_valid, struct k_spinlock *);
DECLARE_FAKE_VOID_FUNC(z_spin_lock_set_owner, struct k_spinlock *);
