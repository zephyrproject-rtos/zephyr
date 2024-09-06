/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/linker/section_tags.h>
#include "memc_nxp_flexram.h"
#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <zephyr/cache.h>

K_SEM_DEFINE(dtcm_magic, 0, 1);

__dtcm_bss_section uint8_t var;
int cnt;

void flexram_magic_addr_isr_cb(enum memc_flexram_interrupt_cause cause,
						void *data)
{
	ARG_UNUSED(data);

	if (cause == flexram_dtcm_magic_addr) {
		printf("Magic DTCM address accessed %d times\n", ++cnt);
		k_sem_give(&dtcm_magic);
	}
}

int main(void)
{
	memc_flexram_register_callback(flexram_magic_addr_isr_cb, NULL);

	console_init();

	printf("%s is opening spellbook...\n", CONFIG_BOARD);
	printf("Cast some characters:\n");

	uint32_t dtcm_addr = (uint32_t)&var;

	memc_flexram_set_dtcm_magic_addr(dtcm_addr);

	uint8_t tmp;

	while (1) {
		printf("\n");
		tmp = console_getchar();
		printf("Writing %c to magic addr...\n", tmp);
		var = tmp;
		k_sem_take(&dtcm_magic, K_FOREVER);
		printf("Reading from magic addr...\n");
		printf("Magic variable got: %c\n", var);
		k_sem_take(&dtcm_magic, K_FOREVER);
	}

	return 0;
}
