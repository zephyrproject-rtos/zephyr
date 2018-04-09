/**
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrfx.h>

void nrfx_isr(void *irq_handler)
{
	((nrfx_irq_handler_t)irq_handler)();
}
