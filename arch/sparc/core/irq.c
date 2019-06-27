/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

void z_irq_spurious(void *unused)
{
	ARG_UNUSED(unused);

	z_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}
