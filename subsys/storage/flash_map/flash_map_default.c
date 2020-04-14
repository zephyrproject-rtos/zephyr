/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <devicetree.h>
#include <storage/flash_map.h>

/* Generate map entry only for non-empty identifiers */
#define FLASH_AREA_GEN(id)				\
	COND_CODE_1(IS_EMPTY(id), (), (FLASH_AREA_GEN_(id)))

#define FLASH_AREA_GEN_(id)				\
	{.fa_id = FLASH_AREA_ID_GEN(id)			\
	 .fa_off = FLASH_AREA_OFFSET(id),		\
	 .fa_dev_name = FLASH_AREA_DEV_LABEL(id),	\
	 .fa_size = FLASH_AREA_SIZE(id), },

/*
 * Create flash map by iterating through list of flash area identifiers
 * collected on the FLASH_AREA_PARTITIONS_LIST.
 */
const struct flash_area default_flash_map[] = {
	FOR_EACH(FLASH_AREA_GEN, FLASH_AREA_PARTITIONS_LIST)
};

const int flash_map_entries = ARRAY_SIZE(default_flash_map);
const struct flash_area *flash_map = default_flash_map;
