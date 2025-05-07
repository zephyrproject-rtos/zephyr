/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sw_isr_table.h>

#include "em_device.h"

void soc_early_init_hook(void)
{
	SystemInit();
}

/* Reserve the address 0x0 at the start of the RAM. LINKER_KEEP is used to prevent the compiler from
 * optimizing this function away. __ramfunc is used to place this function in RAM. It also has the
 * advantage of being moved with the ramfunc section if CONFIG_SRAM_VECTOR_TABLE is set.
 * TODO: In case of multiple __ramfunc functions, we need to ensure that null_pointer_function
 * is the first to be placed in RAM. This can be achieved by modifying the ramfunc linker script.
 */
__ramfunc void null_pointer_function(void)
{
}
LINKER_KEEP(null_pointer_function);

/* SiWx917's bootloader requires IRQn 32 to hold payload's entry point address. */
extern void z_arm_reset(void);
Z_ISR_DECLARE_DIRECT(32, 0, z_arm_reset);
