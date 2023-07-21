/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mocks/kernel_mocks.h>

DEFINE_FAKE_VALUE_FUNC(unsigned int, k_sem_count_get, struct k_sem*);
DEFINE_FAKE_VALUE_FUNC(int, k_sem_take, struct k_sem*, k_timeout_t);

DEFINE_FAKE_VOID_FUNC(k_busy_wait, uint32_t);
DEFINE_FAKE_VOID_FUNC(k_sem_give, struct k_sem*);

