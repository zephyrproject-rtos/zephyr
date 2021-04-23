/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fixed_partitions

#include <zephyr.h>
#include <storage/flash_map.h>
#include <device.h>

#define DEFINE_FLASH_AREA_FOO(part)								\
	DEFINE_FLASH_AREA(DT_FIXED_PARTITION_ID(part)) = {					\
		.fa_off = DT_REG_ADDR(part),							\
		.fa_dev = DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(part)),			\
		.fa_size = DT_REG_SIZE(part),};

#define FOREACH_AREA(n) DT_FOREACH_CHILD(DT_DRV_INST(n), DEFINE_FLASH_AREA_FOO)
DT_FLASH_AREA_FOREACH_OKAY_FIXED_PARTITION(FOREACH_AREA);
