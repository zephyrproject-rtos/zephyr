/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "spinlock.h"

DEFINE_FAKE_VALUE_FUNC(bool, z_spin_lock_valid, struct k_spinlock *);
DEFINE_FAKE_VALUE_FUNC(bool, z_spin_unlock_valid, struct k_spinlock *);
DEFINE_FAKE_VOID_FUNC(z_spin_lock_set_owner, struct k_spinlock *);
