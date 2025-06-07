/*
 * Copyright (c) 2024 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Raspberry Pi RP235xx MCUs
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Raspberry Pi RP235xx (RP2350A, RP2350B, RP2354A, RP2354B).
 */

#include <hardware/address_mapped.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_DECLARE(soc);

#if CONFIG_SOC_RESET_HOOK
#include <pico/runtime_init.h>

void soc_reset_hook(void)
{
	runtime_init_per_core_enable_coprocessors();
}

#endif /* CONFIG_SOC_RESET_HOOK */

#if CONFIG_SOC_RP2350A_BOOT_CPU1

#include <hardware/structs/psm.h>
#include "common/mailbox.h"

static inline bool address_in_range(uint32_t addr, uint32_t base, uint32_t size)
{
	return addr >= base && addr < base + size;
}

static int reset_cpu1(void)
{
	uint32_t val;

	/* Power off, and wait for it to take effect. */
	hw_set_bits(&psm_hw->frce_off, PSM_FRCE_OFF_PROC1_BITS);
	while (!(psm_hw->frce_off & PSM_FRCE_OFF_PROC1_BITS)) {
		;
	}

	/* Power back on, and we can wait for a '0' in the FIFO to know
	 * that it has come back.
	 */
	hw_clear_bits(&psm_hw->frce_off, PSM_FRCE_OFF_PROC1_BITS);
	val = mailbox_pop_blocking();

	return val == 0 ? 0 : -EIO;
}

static void boot_cpu1(uint32_t vector_table_addr, uint32_t stack_ptr, uint32_t reset_ptr)
{
	/* We synchronise with CPU1 and then we can hand over the memory addresses. */
	uint32_t cmds[] = {0, 0, 1, vector_table_addr, stack_ptr, reset_ptr};
	uint32_t seq = 0;

	do {
		uint32_t cmd = cmds[seq], rsp;

		if (cmd == 0) {
			mailbox_flush();
			__SEV();
		}

		mailbox_put_blocking(cmd);
		rsp = mailbox_pop_blocking();

		seq = cmd == rsp ? seq + 1 : 0;
	} while (seq < ARRAY_SIZE(cmds));
}

static int enable_cpu1(void)
{
	/* Flash addresses are defined in our partition table from the device tree. */
	uint32_t cpu1_flash_base =
		DT_REG_ADDR(DT_NODELABEL(flash0)) + DT_REG_ADDR(DT_NODELABEL(code_partition_cpu1));
	uint32_t cpu1_flash_size = DT_REG_SIZE(DT_NODELABEL(code_partition_cpu1));

	uint32_t *cpu1_vector_table = (uint32_t *)cpu1_flash_base;
	uint32_t cpu1_stack_ptr = cpu1_vector_table[0];
	uint32_t cpu1_reset_ptr = cpu1_vector_table[1];

	/* RAM address used to check arguments. */
	uint32_t sram0_start = DT_REG_ADDR(DT_NODELABEL(sram0));
	uint32_t sram0_size = DT_REG_SIZE(DT_NODELABEL(sram0));

	/* Stack pointer shall point within RAM. */
	if (!address_in_range(cpu1_stack_ptr, sram0_start, sram0_size)) {
		LOG_ERR("CPU1 stack pointer invalid.");
		return -EINVAL;
	}

	/* Reset pointer shall point to code within the CPU1 partition. */
	if (!address_in_range(cpu1_reset_ptr, cpu1_flash_base, cpu1_flash_size)) {
		LOG_ERR("CPU1 reset pointer invalid.");
		return -EINVAL;
	}

	LOG_DBG("CPU1 vector table at 0x%p", cpu1_vector_table);
	LOG_DBG("CPU1 stack pointer at 0x%08x", cpu1_stack_ptr);
	LOG_DBG("CPU1 reset pointer at 0x%08x", cpu1_reset_ptr);

	if (reset_cpu1() != 0) {
		LOG_ERR("CPU1 reset failed.");
		return -EIO;
	}

	boot_cpu1((uint32_t)cpu1_vector_table, cpu1_stack_ptr, cpu1_reset_ptr);

	return 0;
}

SYS_INIT(enable_cpu1, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_SOC_RP2350A_BOOT_CPU1 */
