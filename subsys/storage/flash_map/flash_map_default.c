/*
 * Copyright (c) 2017-2024 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2023 Sensorfy B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>

#if CONFIG_FLASH_MAP_LABELS
#define FLASH_AREA_FOO(part, mtd_from_partition)				\
	{.fa_id = DT_FIXED_PARTITION_ID(part),					\
	 .fa_off = DT_REG_ADDR(part),						\
	 .fa_dev = DEVICE_DT_GET(mtd_from_partition(part)),		        \
	 .fa_size = DT_REG_SIZE(part),						\
	 .fa_label = DT_PROP_OR(part, label, NULL),	},
#else
#define FLASH_AREA_FOO(part, mtd_from_partition)				\
	{.fa_id = DT_FIXED_PARTITION_ID(part),					\
	 .fa_off = DT_REG_ADDR(part),						\
	 .fa_dev = DEVICE_DT_GET(mtd_from_partition(part)),		        \
	 .fa_size = DT_REG_SIZE(part), },
#endif

#define FLASH_AREA_FOOO(part)	\
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_PARENT(part), fixed_partitions), (\
		COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_MTD_FROM_FIXED_PARTITION(part)), \
			(FLASH_AREA_FOO(part, DT_MTD_FROM_FIXED_PARTITION)), ())), ( \
		COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_MTD_FROM_FIXED_SUBPARTITION(part)), \
			(FLASH_AREA_FOO(part, DT_MTD_FROM_FIXED_SUBPARTITION)), ())))

#define FOREACH_PARTITION(n) DT_FOREACH_CHILD(n, FLASH_AREA_FOOO)

/* We iterate over all compatible 'fixed-partitions' nodes and
 * use DT_FOREACH_CHILD to iterate over all the partitions for that
 * 'fixed-partitions' node.  This way we build a global partition map
 */
const struct flash_area default_flash_map[] = {
	DT_FOREACH_STATUS_OKAY(fixed_partitions, FOREACH_PARTITION)
	DT_FOREACH_STATUS_OKAY(fixed_subpartitions, FOREACH_PARTITION)
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

#define DEFINE_SUBPARTITION(part) DEFINE_SUBPARTITION_1(part, DT_DEP_ORD(part))
#define DEFINE_SUBPARTITION_1(part, ord)							\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_MTD_FROM_FIXED_SUBPARTITION(part)),		\
		(DEFINE_SUBPARTITION_0(part, ord)), ())
#define DEFINE_SUBPARTITION_0(part, ord)							\
	const struct flash_area DT_CAT(global_fixed_subpartition_ORD_, ord) = {			\
		.fa_id = DT_FIXED_PARTITION_ID(part),						\
		.fa_off = DT_REG_ADDR(part),							\
		.fa_dev = DEVICE_DT_GET(DT_MTD_FROM_FIXED_SUBPARTITION(part)),			\
		.fa_size = DT_REG_SIZE(part),							\
	};

#define FOR_EACH_SUBPARTITION_TABLE(table) DT_FOREACH_CHILD(table, DEFINE_SUBPARTITION)
DT_FOREACH_STATUS_OKAY(fixed_subpartitions, FOR_EACH_SUBPARTITION_TABLE)
