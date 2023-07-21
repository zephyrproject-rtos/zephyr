/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mocks/arch_interface_mocks.h>

DEFINE_FAKE_VALUE_FUNC(int,
                       arch_irq_connect_dynamic,
                       unsigned int,
                       unsigned int,
                       arch_irq_connect_dynamic_funcptr_t,
                       const void*,
                       uint32_t);

