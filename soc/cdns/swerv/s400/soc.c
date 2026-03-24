/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>

void soc_interrupt_init(void)
{
	(void)arch_irq_lock();

	csr_write(mie, 0);
	csr_write(mip, 0);
}
