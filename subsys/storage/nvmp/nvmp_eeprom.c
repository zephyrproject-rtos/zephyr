/*
 * Copyright (c) 2024, Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/storage/nvmp.h>
#include <zephyr/drivers/eeprom.h>

LOG_MODULE_REGISTER(nvmp_eeprom, CONFIG_NVMP_LOG_LEVEL);

struct eeprom_store {
	const struct device *dev;
	size_t offset;
};

static int nvmp_eeprom_open(const struct nvmp_info *info)
{
	const struct eeprom_store *st = (const struct eeprom_store *)info->store;
	int rc = 0;

	if (!device_is_ready(st->dev)) {
		rc = -ENODEV;
	}

	return rc;
}

static int nvmp_eeprom_read(const struct nvmp_info *info, size_t start,
			    void *data, size_t len)
{
	const struct eeprom_store *st = (const struct eeprom_store *)info->store;

	LOG_DBG("read %d byte at 0x%x", len, start);
	start += st->offset;
	return eeprom_read(st->dev, (off_t)start, data, len);
}

static int nvmp_eeprom_write(const struct nvmp_info *info, size_t start,
			     const void *data, size_t len)
{
	const struct eeprom_store *st = (const struct eeprom_store *)info->store;

	LOG_DBG("write %d byte at 0x%x", len, start);
	start += st->offset;
	return eeprom_write(st->dev, (off_t)start, data, len);
}

static int nvmp_eeprom_close(const struct nvmp_info *info)
{
	return 0;
}

#include "nvmp_define.h"

#define NVMP_EEPROM_DEV(inst)    DEVICE_DT_GET(DT_GPARENT(inst))
#define NVMP_EEPROM_BSIZE(inst)  DT_PROP_OR(inst, block_size, (NVMP_SIZE(inst)))
#define NVMP_EEPROM_WBSIZE(inst) DT_PROP_OR(inst, write_block_size, 1U)

#define NVMP_EEPROM_ITEM_DEFINE(inst)                                           \
	const struct eeprom_store eeprom_store_##inst = {                       \
		.dev = NVMP_EEPROM_DEV(inst),                                   \
		.offset = NVMP_OFF(inst),                                       \
	};                                                                      \
	NVMP_INFO_DEFINE(inst, (void *)&eeprom_store_##inst, NVMP_SIZE(inst),   \
			 NVMP_EEPROM_BSIZE(inst), NVMP_EEPROM_WBSIZE(inst),     \
			 nvmp_eeprom_open, nvmp_eeprom_read,                    \
			 NVMP_PRO(inst) ? NULL : nvmp_eeprom_write, NULL, NULL, \
			 nvmp_eeprom_close);

#define NVMP_EEPROM_DEFINE(inst) DT_FOREACH_CHILD(inst, NVMP_EEPROM_ITEM_DEFINE)

DT_FOREACH_STATUS_OKAY(zephyr_eeprom_nvmp_fixed_partitions, NVMP_EEPROM_DEFINE)
