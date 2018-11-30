/*
 * Copyright (c) 2018, Cypress Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cy_sysint.h"
#include <misc/printk.h>
#include <zephyr.h>

#include "cyprotection.h"

#define CM0_MEMORY_ADDR (0x08000000 + 0x500)
#define CM4_MEMORY_ADDR (CONFIG_SRAM_BASE_ADDRESS + 0x500)

void main(void)
{
	u32_t *mem_addr;

	printk("PSoC6 Unsecure core has started\n\n");

	printk("Read from CM4 memory\n");
	mem_addr = (u32_t *)CM4_MEMORY_ADDR;
	printk("Content of CM4 memory by address 0x%x is 0x%x\n\n",
		(u32_t)mem_addr, *mem_addr);
	printk("Read from CM0 memory\n");
	mem_addr = (u32_t *)CM0_MEMORY_ADDR;
	printk("Content of CM0 memory by address 0x%x is 0x%x\n",
		(u32_t)mem_addr, *mem_addr);

	while (1) {
	}
}

