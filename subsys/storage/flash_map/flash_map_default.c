/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fixed_partitions

#include <zephyr.h>
#include <storage/flash_map.h>

/* Get the grand parent of a node */
#define GPARENT(node_id) DT_PARENT(DT_PARENT(node_id))

/* return great-grandparent label if 'soc-nv-flash' else grandparent label */
#define DT_FLASH_DEV_FROM_PART(part)					      \
	DT_LABEL(COND_CODE_1(DT_NODE_HAS_COMPAT(GPARENT(part), soc_nv_flash), \
			     (DT_PARENT(GPARENT(part))),		      \
			     (GPARENT(part))))

#define FLASH_AREA_FOO(part)				\
	{.fa_id = DT_FIXED_PARTITION_ID(part),		\
	 .fa_off = DT_REG_ADDR(part),			\
	 .fa_dev_name = DT_FLASH_DEV_FROM_PART(part),	\
	 .fa_size = DT_REG_SIZE(part),},

#define FOREACH_PARTION(n) DT_FOREACH_CHILD(DT_DRV_INST(n), FLASH_AREA_FOO)

/* We iterate over all compatible 'fixed-partions' nodes and
 * use DT_FOREACH_CHILD to iterate over all the partitions for that
 * 'fixed-partions' node.  This way we build a global partition map
 */
const struct flash_area default_flash_map[] = {
	DT_INST_FOREACH_STATUS_OKAY(FOREACH_PARTION)
};

const int flash_map_entries = ARRAY_SIZE(default_flash_map);
const struct flash_area *flash_map = default_flash_map;
