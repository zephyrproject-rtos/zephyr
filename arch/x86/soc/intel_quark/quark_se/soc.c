/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief System/hardware module for the Quark SE BSP
 *
 * This module provides routines to initialize and support board-level
 * hardware for the Quark SE BSP.
 */

#include <errno.h>

#include <nanokernel.h>
#include <misc/printk.h>
#include <misc/__assert.h>
#include "soc.h"
#include <uart.h>
#include <init.h>
#include "shared_mem.h"

#ifdef CONFIG_ARC_INIT
#define SCSS_REG_VAL(offset) \
	(*((volatile uint32_t *)(SCSS_REGISTER_BASE+offset)))

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ARC_INIT_LEVEL
#include <misc/sys_log.h>

/**
 *
 * @brief ARC Init
 *
 * This routine initialize the ARC reset vector and
 * starts the ARC processor.
 * @return N/A
 */
static int arc_init(struct device *arg)
{
	uint32_t *reset_vector;

	ARG_UNUSED(arg);

	if (!SCSS_REG_VAL(SCSS_SS_STS)) {
		/* ARC shouldn't already be running! */
		printk("ARC core already running!");
		return -EIO;
	}

	/* Address of ARC side __reset stored in the first 4 bytes of arc.bin,
	 * we read the value and stick it in shared_mem->arc_start which is
	 * the beginning of the address space at 0xA8000000 */
	reset_vector = (uint32_t *)RESET_VECTOR;
	SYS_LOG_DBG("Reset vector address: %x", *reset_vector);
	shared_data->arc_start = *reset_vector;
	shared_data->flags = 0;
	if (!shared_data->arc_start) {
		/* Reset vector points to NULL => skip ARC init. */
		SYS_LOG_DBG("Reset vector is NULL, skipping ARC init.");
		goto skip_arc_init;
	}

#ifndef CONFIG_ARC_GDB_ENABLE
	/* Start the CPU */
	SCSS_REG_VAL(SCSS_SS_CFG) |= ARC_RUN_REQ_A;
#endif

	SYS_LOG_DBG("Waiting for arc to start...");
	/* Block until the ARC core actually starts up */
	while (SCSS_REG_VAL(SCSS_SS_STS) & 0x4000) {
	}

	/* Block until ARC's quark_se_init() sets a flag indicating it is ready,
	 * if we get stuck here ARC has run but has exploded very early */
	SYS_LOG_DBG("Waiting for arc to init...");
	while (!(shared_data->flags & ARC_READY)) {
	}

skip_arc_init:

	return 0;
}

SYS_INIT(arc_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /*CONFIG_ARC_INIT*/

