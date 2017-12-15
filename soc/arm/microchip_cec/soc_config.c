/*
 * Copyright (c) 2017 Crypta Labs Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>

#include "soc.h"

static int soc_init(struct device *dev)
{
	__IO uint32_t *girc_enable;

	/* Enable IRQs as ROM loader does PRIMASK=1 */
	__enable_irq();

	/* Enable clocks for Interrupts and CPU */
	PCR_INST->CLK_REQ_1_b.INT_CLK_REQ = 1;
	PCR_INST->CLK_REQ_1_b.PROCESSOR_CLK_REQ = 1;

	/* Route all interrupts from EC to NVIC */
	EC_REG_BANK_INST->INTERRUPT_CONTROL = 0x1;
	for (girc_enable = &INTS_INST->GIRQ08_EN_SET;
	     girc_enable <= &INTS_INST->GIRQ15_EN_SET;
	     girc_enable += 5) {
		*girc_enable = 0xFFFFFFFF;
	}

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
