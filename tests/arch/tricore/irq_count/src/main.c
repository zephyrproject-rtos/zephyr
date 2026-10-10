/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>

static void irq_handler(const void *arg)
{
	ARG_UNUSED(arg);
}

int main(void)
{
	IRQ_CONNECT(172, 1, irq_handler, NULL, 0);

	return 0;
}
