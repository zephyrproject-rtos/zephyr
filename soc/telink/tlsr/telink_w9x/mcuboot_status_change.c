/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bootutil/mcuboot_status.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/fixed-partitions.h>
#include <zephyr/irq.h>

#define BOOTLOADER_MCUBOOT_ROM_START_OFFSET             0x200

void mcuboot_status_change(mcuboot_status_type_t status)
{
	if (status == MCUBOOT_STATUS_BOOTABLE_IMAGE_FOUND) {
		uintptr_t app_start_addr = DT_FIXED_PARTITION_ADDR(DT_NODELABEL(slot0_partition)) +
			BOOTLOADER_MCUBOOT_ROM_START_OFFSET;
		void *boot_app = (void *)app_start_addr;

		irq_lock();
		((void (*)(void))boot_app)();
	}
}
