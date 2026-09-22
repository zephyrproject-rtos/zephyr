/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

extern void bflb_ble_irq_setup(void);

int bflb_rf_init(void)
{
	bflb_ble_irq_setup();
	return 0;
}
