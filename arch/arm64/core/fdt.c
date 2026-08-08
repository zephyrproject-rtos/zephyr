/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/arch/arm64/fdt.h>

#include "boot.h"

LOG_MODULE_REGISTER(arm64_boot_fdt);

#define FDT_MAGIC 0xd00dfeed
#define FDT_TOTALSIZE_OFFSET 4

/* Raw early-boot handoff from reset.S. Survives data/BSS initialization. */
__noinit uintptr_t z_arm64_boot_fdt_addr;

/* Validated Zephyr-owned FDT copy and CPU-endian size for runtime consumers. */
static uint8_t boot_fdt[CONFIG_ARM64_BOOT_FDT_MAX_SIZE] __aligned(8);
static uint32_t boot_fdt_size;

uintptr_t arm64_boot_fdt_get(uint32_t *fdt_size)
{
	if (fdt_size != NULL) {
		*fdt_size = boot_fdt_size;
	}

	return (uintptr_t)boot_fdt;
}

void z_arm64_copy_boot_fdt(void)
{
	const uint8_t *src = (const uint8_t *)z_arm64_boot_fdt_addr;
	uint32_t magic;
	uint32_t size;

	if (src == NULL) {
		LOG_ERR("No boot device tree address");
		k_panic();
	}

	magic = sys_get_be32(src);
	if (magic != FDT_MAGIC) {
		LOG_ERR("Invalid boot device tree magic: 0x%x", magic);
		k_panic();
	}

	size = sys_get_be32(src + FDT_TOTALSIZE_OFFSET);
	if (size > CONFIG_ARM64_BOOT_FDT_MAX_SIZE) {
		LOG_ERR("Boot device tree size 0x%x exceeds max 0x%x",
			size, CONFIG_ARM64_BOOT_FDT_MAX_SIZE);
		k_panic();
	}

	arch_early_memcpy(boot_fdt, src, size);
	boot_fdt_size = size;
}
