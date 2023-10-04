/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/kernel.h>


static int soc_init(void)
{
	__IO uint32_t *girc_enable_set;

	__enable_irq();

	/* Enable clocks for Interrupts and CPU */
	PCR_INST->CLK_REQ_1_b.INT_CLK_REQ = 1;
	PCR_INST->CLK_REQ_1_b.PROCESSOR_CLK_REQ = 1;

	/* Route all interrupts from EC to NVIC */
	EC_REG_BANK_INST->INTERRUPT_CONTROL = 0x1;
	for (girc_enable_set = (uint32_t *)&INTS_INST->GIRQ08_EN_SET;
	     girc_enable_set <= &INTS_INST->GIRQ15_EN_SET;
	     girc_enable_set += 5) {
	/* This probably will require tuning, but drawing 8.2 also
	illustrates how to disable spurious interrupts */
		*girc_enable_set = 0xFFFFFFFF;
	}

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
