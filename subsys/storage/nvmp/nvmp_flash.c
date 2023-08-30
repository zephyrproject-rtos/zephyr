/*
 * Copyright (c) 2023, Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/storage/nvmp.h>

LOG_MODULE_REGISTER(nvmp_flash, CONFIG_NVMP_LOG_LEVEL);

static int nvmp_flash_read(const struct nvmp_info *info, size_t start,
			   void *data, size_t len)
{
	if ((info == NULL) || (info->size < len) ||
	    ((info->size - len) < start)) {
		return -EINVAL;
	}

	const struct device *dev = (const struct device *)info->store;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	start += info->store_start;
	LOG_DBG("read %d byte at 0x%x", len, start);
	return flash_read(dev, (off_t)start, data, len);
}

static int nvmp_flash_write(const struct nvmp_info *info, size_t start,
			    const void *data, size_t len)
{
	if ((info == NULL) || (info->size < len) ||
	    ((info->size - len) < start)) {
		return -EINVAL;
	}

	if (info->read_only) {
		return -EACCES;
	}

	const struct device *dev = (const struct device *)info->store;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	start += info->store_start;
	LOG_DBG("write %d byte at 0x%x", len, start);
	return flash_write(dev, (off_t)start, data, len);
}

static int nvmp_flash_erase(const struct nvmp_info *info, size_t start,
			    size_t len)
{
	if ((info == NULL) || (info->size < len) ||
	    ((info->size - len) < start)) {
		return -EINVAL;
	}

	if (info->read_only) {
		return -EACCES;
	}

	const struct device *dev = (const struct device *)info->store;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	start += info->store_start;
	LOG_DBG("erase %d byte at 0x%x", len, start);
	return flash_erase(dev, (off_t)start, len);
}

#include "nvmp_define.h"

#define NVMP_FLASH_DEV(inst)							\
	COND_CODE_1(DT_NODE_HAS_COMPAT(inst, soc_nv_flash),			\
		    (DEVICE_DT_GET(DT_PARENT(inst))), (DEVICE_DT_GET(inst)))
#define NVMP_FLASH_PDEV(inst) NVMP_FLASH_DEV(DT_GPARENT(inst))

#define NVMP_FLASH_PARTITION_DEFINE(inst)					\
	NVMP_INFO_DEFINE(inst, FLASH, NVMP_PSIZE(inst), NVMP_PRO(inst),		\
			 (void *)NVMP_FLASH_PDEV(inst), NVMP_POFF(inst),	\
			 nvmp_flash_read, nvmp_flash_write, nvmp_flash_erase);

#define NVMP_FLASH_PARTITIONS_DEFINE(inst)					\
	DT_FOREACH_CHILD_STATUS_OKAY(inst, NVMP_FLASH_PARTITION_DEFINE)

#define NVMP_FLASH_SIZE(inst)							\
	COND_CODE_1(DT_NODE_HAS_COMPAT(inst, soc_nv_flash), (DT_REG_SIZE(inst)),\
		    (DT_PROP_OR(inst, size, 0) / 8))
#define NVMP_FLASH_OFF(inst) 0U
#define NVMP_FLASH_RO(inst) NVMP_RO(inst)

#define NVMP_FLASH_DEFINE(inst)							\
	NVMP_INFO_DEFINE(inst, FLASH, NVMP_FLASH_SIZE(inst),			\
			 NVMP_FLASH_RO(inst), (void *)NVMP_FLASH_DEV(inst),	\
			 NVMP_FLASH_OFF(inst), nvmp_flash_read,			\
			 nvmp_flash_write, nvmp_flash_erase);			\
	DT_FOREACH_CHILD(inst, NVMP_FLASH_PARTITIONS_DEFINE)

DT_FOREACH_STATUS_OKAY(zephyr_nvmp_flash, NVMP_FLASH_DEFINE)
