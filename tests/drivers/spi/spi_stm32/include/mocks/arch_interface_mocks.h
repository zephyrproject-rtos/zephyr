/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARCH_INTERFACE_MOCKS_H
#define ARCH_INTERFACE_MOCKS_H

#include <zephyr/fff.h>
#include <zephyr/sys/arch_interface.h>

typedef void (*arch_irq_connect_dynamic_funcptr_t)(const void *parameter);

DECLARE_FAKE_VALUE_FUNC(int,
                        arch_irq_connect_dynamic,
                        unsigned int,
                        unsigned int,
                        arch_irq_connect_dynamic_funcptr_t,
                        const void*,
                        uint32_t);

#endif /* ARCH_INTERFACE_MOCKS_H */
