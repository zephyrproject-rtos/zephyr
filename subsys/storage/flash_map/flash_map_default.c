/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fixed_partitions

#include <zephyr/device.h>
#include <zephyr/zephyr.h>
#include <zephyr/storage/flash_map.h>

#define FLASH_AREA_FOO(part)							\
	{.fa_id = DT_FIXED_PARTITION_ID(part),					\
	 .fa_off = DT_REG_ADDR(part),						\
	 .fa_dev = DEVICE_DT_GET_OR_NULL(DT_MTD_FROM_FIXED_PARTITION(part)),	\
	 .fa_size = DT_REG_SIZE(part),},

#define FOREACH_PARTITION(n) DT_FOREACH_CHILD(DT_DRV_INST(n), FLASH_AREA_FOO)

/* We iterate over all compatible 'fixed-partitions' nodes and
 * use DT_FOREACH_CHILD to iterate over all the partitions for that
 * 'fixed-partitions' node.  This way we build a global partition map
 */
const struct flash_area default_flash_map[] = {
	DT_INST_FOREACH_STATUS_OKAY(FOREACH_PARTITION)
};

const int flash_map_entries = ARRAY_SIZE(default_flash_map);
const struct flash_area *flash_map = default_flash_map;
