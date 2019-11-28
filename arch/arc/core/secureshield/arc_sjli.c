/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <kernel.h>
#include <errno.h>
#include <zephyr/types.h>
#include <init.h>
#include <toolchain.h>

#include <arch/arc/v2/secureshield/arc_secure.h>

static void _default_sjli_entry(void);
/*
 * sjli vector table must be in instruction space
 * \todo: how to let user to install customized sjli entry easily, e.g.
 *  through macros or with the help of compiler?
 */
const static u32_t _sjli_vector_table[CONFIG_SJLI_TABLE_SIZE] = {
	[0] = (u32_t)_arc_do_secure_call,
	[1 ... (CONFIG_SJLI_TABLE_SIZE - 1)] = (u32_t)_default_sjli_entry,
};

/*
 * @brief default entry of sjli call
 *
 */
static void _default_sjli_entry(void)
{
	printk("default sjli entry\n");
}

/*
 * @brief initializaiton of sjli related functions
 *
 */
static void sjli_table_init(void)
{
	/* install SJLI table */
	z_arc_v2_aux_reg_write(_ARC_V2_NSC_TABLE_BASE, _sjli_vector_table);
	z_arc_v2_aux_reg_write(_ARC_V2_NSC_TABLE_TOP,
		(_sjli_vector_table + CONFIG_SJLI_TABLE_SIZE));
}

/*
 * @brief initializaiton of secureshield related functions.
 */
static int arc_secureshield_init(struct device *arg)
{
	sjli_table_init();

	/* set nic bit to enable seti/clri and
	 * sleep/wevt in normal mode.
	 * If not set, direct call of seti/clri etc. will raise exception.
	 * Then, these seti/clri instructions should be replaced with secure
	 * secure services (sjli call)
	 *
	 */
	__asm__ volatile("sflag  0x20");

	return 0;
}

SYS_INIT(arc_secureshield_init, PRE_KERNEL_1,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
