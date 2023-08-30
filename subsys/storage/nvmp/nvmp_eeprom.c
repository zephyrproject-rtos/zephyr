/*
 * Copyright (c) 2023, Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/nvmp.h>

LOG_MODULE_REGISTER(nvmp_eeprom, CONFIG_NVMP_LOG_LEVEL);

static int nvmp_eeprom_read(const struct nvmp_info *info, size_t start,
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
	return eeprom_read(dev, (off_t)start, data, len);
}

static int nvmp_eeprom_write(const struct nvmp_info *info, size_t start,
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
	return eeprom_write(dev, (off_t)start, data, len);
}

#ifdef CONFIG_NVMP_EEPROM_ERASE
static int nvmp_eeprom_erase(const struct nvmp_info *info, size_t start,
			     size_t len)
{
	if ((info == NULL) || (info->size < len) ||
	    ((info->size - len) < start)) {
		return -EINVAL;
	}

	if (info->read_only) {
		return -EACCES;
	}

	int rc;
	uint8_t buf[CONFIG_NVMP_ERASE_BUFSIZE];

	memset(buf, (char)CONFIG_NVMP_ERASE_VALUE, sizeof(buf));
	while (len != 0) {
		size_t wrlen = MIN(len, sizeof(buf));

		rc = info->write(info, start, buf, wrlen);
		if (rc) {
			break;
		}

		len -= wrlen;
		start += wrlen;
	}

	return rc;
}
#else /* CONFIG_NVMP_EEPROM_ERASE */
static int nvmp_eeprom_erase(const struct nvmp_info *info, size_t start,
			     size_t len)
{
	return -ENOTSUP;
}
#endif /* CONFIG_NVMP_EEPROM_ERASE */

#include "nvmp_define.h"

#define NVMP_EEPROM_DEV(inst) DEVICE_DT_GET(inst)
#define NVMP_EEPROM_PDEV(inst) NVMP_EEPROM_DEV(DT_GPARENT(inst))

#define NVMP_EEPROM_PARTITION_DEFINE(inst)					\
	NVMP_INFO_DEFINE(inst, EEPROM, NVMP_PSIZE(inst), NVMP_PRO(inst),	\
			 (void *)NVMP_EEPROM_PDEV(inst), NVMP_POFF(inst),	\
			 nvmp_eeprom_read, nvmp_eeprom_write, nvmp_eeprom_erase);

#define NVMP_EEPROM_PARTITIONS_DEFINE(inst)					\
	DT_FOREACH_CHILD_STATUS_OKAY(inst, NVMP_EEPROM_PARTITION_DEFINE)

#define NVMP_EEPROM_SIZE(inst) DT_PROP(inst, size)
#define NVMP_EEPROM_OFF(inst) 0U
#define NVMP_EEPROM_RO(inst) NVMP_RO(inst)

#define NVMP_EEPROM_DEFINE(inst)						\
	NVMP_INFO_DEFINE(inst, EEPROM, NVMP_EEPROM_SIZE(inst),			\
			 NVMP_EEPROM_RO(inst), (void *)NVMP_EEPROM_DEV(inst),	\
			 NVMP_EEPROM_OFF(inst), nvmp_eeprom_read,		\
			 nvmp_eeprom_write, nvmp_eeprom_erase);			\
	DT_FOREACH_CHILD(inst, NVMP_EEPROM_PARTITIONS_DEFINE)

DT_FOREACH_STATUS_OKAY(zephyr_nvmp_eeprom, NVMP_EEPROM_DEFINE)
