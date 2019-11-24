/*
 * Copyright (c) 2019 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module
 *
 * This module initializes SoC
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include "soc_clock.h"
#include "soc_memctl.h"
#include "soc_pmu.h"

int acts_init(struct device *dev)
{
	ARG_UNUSED(dev);
	uint32_t key;

	key = irq_lock();

	/* Init VD12 */
	acts_request_vd12_pd(false);
	acts_request_vd12_largebias(false);

	/* Disable RC_3M */
	acts_request_rc_3M(false);

	/* Remap Vector base address to SPI NOR memory */
	sys_write32(0x01000000, VECTOR_BASE);
	sys_write32(sys_read32(MEM_CTL) | MEM_CTL_VECTOR_TABLE_SEL, MEM_CTL);

	/* Set VD12_OK for RC3M */
	sys_write32(sys_read32(COREPLL_DBGCTL) | (0x1 << 12), COREPLL_DBGCTL);

	irq_unlock(key);

	return 0;
}

SYS_INIT(acts_init, PRE_KERNEL_1, 0);
