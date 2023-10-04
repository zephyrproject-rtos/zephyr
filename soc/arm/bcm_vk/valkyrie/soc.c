/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright 2018 Broadcom.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int valkyrie_init(void)
{
	uint32_t key;


	key = irq_lock();

	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(valkyrie_init, PRE_KERNEL_1, 0);
