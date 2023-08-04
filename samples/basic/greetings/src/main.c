/*
 * Copyright (c) 2023 Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>

int main(void)
{
	while (1) {
		printf("Zephyr says 'Hi!'\n");
		k_msleep(1000);
	}
	return 0;
}
