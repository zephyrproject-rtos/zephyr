/*
 * Copyright 2023 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chip_chipregs.h>
#include <zephyr/arch/posix/arch.h>

int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
		    void (*routine)(const void *parameter),
		    const void *parameter, uint32_t flags);
int arch_irq_disconnect_dynamic(unsigned int irq, unsigned int priority,
		       void (*routine)(const void *parameter),
		       const void *parameter, uint32_t flags);
typedef struct z_thread_stack_element k_thread_stack_t;
