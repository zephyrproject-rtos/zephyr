/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mocks/k_mutex_mocks.h>


DEFINE_FAKE_VALUE_FUNC(int, k_mutex_lock, struct k_mutex*, k_timeout_t);
DEFINE_FAKE_VALUE_FUNC(int, k_mutex_unlock, struct k_mutex*);
