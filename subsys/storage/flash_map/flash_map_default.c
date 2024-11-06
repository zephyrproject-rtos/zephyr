/*
 * Copyright (c) 2017-2024 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2023 Sensorfy B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fixed_partitions

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>

#if CONFIG_FLASH_MAP_LABELS
#define FLASH_AREA_FOO(part)							\
	{.fa_id = DT_FIXED_PARTITION_ID(part),					\
	 .fa_off = DT_REG_ADDR(part),						\
	 .fa_dev = DEVICE_DT_GET_OR_NULL(DT_MTD_FROM_FIXED_PARTITION(part)),	\
	 .fa_size = DT_REG_SIZE(part),						\
	 .fa_label = DT_PROP_OR(part, label, NULL),	},
#else
#define FLASH_AREA_FOO(part)							\
	{.fa_id = DT_FIXED_PARTITION_ID(part),					\
	 .fa_off = DT_REG_ADDR(part),						\
	 .fa_dev = DEVICE_DT_GET_OR_NULL(DT_MTD_FROM_FIXED_PARTITION(part)),	\
	 .fa_size = DT_REG_SIZE(part), },
#endif

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

/* Generate objects representing each partition in system. In the end only
 * objects referenced by code will be included into build.
 */
#define DEFINE_PARTITION(part) DEFINE_PARTITION_1(part, DT_DEP_ORD(part))
#define DEFINE_PARTITION_1(part, ord)								\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_MTD_FROM_FIXED_PARTITION(part)),			\
		(DEFINE_PARTITION_0(part, ord)), ())
#define DEFINE_PARTITION_0(part, ord)								\
	const struct flash_area DT_CAT(global_fixed_partition_ORD_, ord) = {			\
		.fa_id = DT_FIXED_PARTITION_ID(part),						\
		.fa_off = DT_REG_ADDR(part),							\
		.fa_dev = DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(part)),			\
		.fa_size = DT_REG_SIZE(part),							\
	};

#define FOR_EACH_PARTITION_TABLE(table) DT_FOREACH_CHILD(table, DEFINE_PARTITION)
DT_FOREACH_STATUS_OKAY(fixed_partitions, FOR_EACH_PARTITION_TABLE)
