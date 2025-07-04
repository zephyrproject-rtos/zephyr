/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

int main(void)
{
	while (1) {
		k_yield();
	}

	return 0;
}
