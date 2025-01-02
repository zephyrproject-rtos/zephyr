/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>

#include "factory_data_parser.h"

static struct FactoryData factoryData;

int main(void)
{
	static const uint8_t *flashMemoryRegion = (const uint8_t *)FIXED_PARTITION_OFFSET(flash) +
						  FIXED_PARTITION_OFFSET(factory_partition);

	printk("Partition image offset: 0x%x, size: 0x%x\n",
	       FIXED_PARTITION_OFFSET(factory_partition), FIXED_PARTITION_SIZE(factory_partition));

	if (!parse_factory_data(flashMemoryRegion, FIXED_PARTITION_SIZE(factory_partition),
				&factoryData)) {
		printk("[E] Failed to parse factory data\n");
		return -EINVAL;
	}

	printk("Parsed Factory Data OK!\n");

	print_factory_data(&factoryData);

	return 0;
}
