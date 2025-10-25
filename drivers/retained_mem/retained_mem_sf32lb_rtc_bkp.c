/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_rtc_backup

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/retained_mem.h>

#include <string.h>

LOG_MODULE_REGISTER(retained_mem_sf32lb_rtc_bkp, CONFIG_RETAINED_MEM_LOG_LEVEL);

struct retained_mem_sf32lb_config {
	uintptr_t base;
	uintptr_t sub_base;
	size_t size;
};

static ssize_t retained_mem_sf32lb_size(const struct device *dev)
{
	const struct retained_mem_sf32lb_config *cfg = dev->config;

	return (ssize_t)cfg->size;
}

static int retained_mem_sf32lb_read(const struct device *dev, off_t offset, uint8_t *buf,
					size_t len)
{
	const struct retained_mem_sf32lb_config *cfg = dev->config;

	if (offset < 0 || (size_t)offset >= cfg->size) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	if ((size_t)offset + len > cfg->size) {
		/* clamp the length to available bytes */
		len = cfg->size - (size_t)offset;
	}

	memcpy(buf, (cfg->base + cfg->sub_base + offset), len);

	return len;
}

static int retained_mem_sf32lb_write(const struct device *dev, off_t offset, const uint8_t *buf,
					 size_t len)
{
	const struct retained_mem_sf32lb_config *cfg = dev->config;

	if (offset < 0 || (size_t)offset >= cfg->size) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	if ((size_t)offset + len > cfg->size) {
		/* clamp the length to available bytes */
		len = cfg->size - (size_t)offset;
	}

	memcpy(cfg->base + cfg->sub_base + offset, buf, len);

	return len;
}

static DEVICE_API(retained_mem, retained_mem_sf32lb_api) = {
	.size = retained_mem_sf32lb_size,
	.read = retained_mem_sf32lb_read,
	.write = retained_mem_sf32lb_write,
};

static int retained_mem_sf32lb_init(const struct device *dev)
{
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	return 0;
}

static const struct retained_mem_sf32lb_config retained_mem_sf32lb_cfg_0 = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0)),
	.sub_base = DT_INST_REG_ADDR(0),
	.size = DT_INST_REG_SIZE(0),
};

DEVICE_DT_INST_DEFINE(0, retained_mem_sf32lb_init, NULL, NULL, &retained_mem_sf32lb_cfg_0,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &retained_mem_sf32lb_api);
