/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KERNEL_MOCKS_H
#define KERNEL_MOCKS_H

#include <stdint.h>

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

DECLARE_FAKE_VALUE_FUNC(unsigned int, k_sem_count_get, struct k_sem*);
DECLARE_FAKE_VALUE_FUNC(int, k_sem_take, struct k_sem*, k_timeout_t);

DECLARE_FAKE_VOID_FUNC(k_busy_wait, uint32_t);
DECLARE_FAKE_VOID_FUNC(k_sem_give, struct k_sem*);

#endif /* KERNEL_MOCKS_H */
