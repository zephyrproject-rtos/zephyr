/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for the Quark SE BSP
 *
 * This module provides routines to initialize and support board-level
 * hardware for the Quark SE BSP.
 */

#include <errno.h>

#include <kernel.h>
#include <misc/printk.h>
#include <misc/__assert.h>
#include "soc.h"
#include <uart.h>
#include <init.h>
#include "shared_mem.h"
#include <mmustructs.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(soc);

#ifdef CONFIG_X86_MMU
/* loapic */
MMU_BOOT_REGION(CONFIG_LOAPIC_BASE_ADDRESS, 4*1024, MMU_ENTRY_WRITE);

/*ioapic */
MMU_BOOT_REGION(DT_IOAPIC_BASE_ADDRESS, 1024*1024, MMU_ENTRY_WRITE);

/* peripherals */
MMU_BOOT_REGION(0xB0000000, 128*1024, MMU_ENTRY_WRITE);

/* SCSS system control subsystem */
MMU_BOOT_REGION(0xB0800000, 16*1024, MMU_ENTRY_WRITE);

/* DMA */
MMU_BOOT_REGION(0xB0700000, 4*1024, MMU_ENTRY_WRITE);

/* USB */
MMU_BOOT_REGION(0xB0500000, 256*1024, MMU_ENTRY_WRITE);

#endif /* CONFIG_X86_MMU */


#ifdef CONFIG_ARC_INIT
#define SCSS_REG_VAL(offset) \
	(*((volatile u32_t *)(SCSS_REGISTER_BASE+offset)))

/**
 *
 * @brief ARC Init
 *
 * This routine initialize the ARC reset vector and
 * starts the ARC processor.
 * @return N/A
 */
/* This function is also called at deep sleep resume. */
int z_arc_init(struct device *arg)
{
	u32_t *reset_vector;

	ARG_UNUSED(arg);

	if (SCSS_REG_VAL(SCSS_SS_STS) == 0U) {
		/* ARC shouldn't already be running! */
		printk("ARC core already running!");
		return -EIO;
	}

	/* Address of ARC side __reset stored in the first 4 bytes of arc.bin,
	 * we read the value and stick it in shared_mem->arc_start which is
	 * the beginning of the address space at 0xA8000000 */
	reset_vector = (u32_t *)RESET_VECTOR;
	LOG_DBG("Reset vector address: %x", *reset_vector);
	shared_data->arc_start = *reset_vector;
	shared_data->flags = 0U;
	if (shared_data->arc_start == 0U) {
		/* Reset vector points to NULL => skip ARC init. */
		LOG_DBG("Reset vector is NULL, skipping ARC init.");
		goto skip_arc_init;
	}

#ifndef CONFIG_ARC_GDB_ENABLE
	/* Start the CPU */
	SCSS_REG_VAL(SCSS_SS_CFG) |= ARC_RUN_REQ_A;
#endif

	LOG_DBG("Waiting for arc to start...");
	/* Block until the ARC core actually starts up */
	while ((SCSS_REG_VAL(SCSS_SS_STS) & 0x4000) != 0U) {
	}

	/* Block until ARC's quark_se_init() sets a flag indicating it is ready,
	 * if we get stuck here ARC has run but has exploded very early */
	LOG_DBG("Waiting for arc to init...");
	while ((shared_data->flags & ARC_READY) == 0U) {
	}

skip_arc_init:

	return 0;
}

SYS_INIT(z_arc_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /*CONFIG_ARC_INIT*/

