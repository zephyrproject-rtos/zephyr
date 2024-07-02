/*
 * Copyright (c) 2024 ANITRA system s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv3028_retmem

#include <zephyr/drivers/retained_mem.h>
#include "rtc_rv3028.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(rv3028, CONFIG_RETAINED_MEM_LOG_LEVEL);

#define RV3028_USER_RAM_SIZE (2 * sizeof(uint8_t))

BUILD_ASSERT(CONFIG_RETAINED_MEM_RV3028_INIT_PRIORITY > CONFIG_RTC_INIT_PRIORITY,
	     "RV3028 Retained Memory driver must be initialized after RV3028 RTC driver");

struct rv3028_retained_mem_config {
	const struct device *parent;
};

static ssize_t rv3028_retained_mem_size(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (ssize_t)RV3028_USER_RAM_SIZE;
}

static int rv3028_retained_mem_read(const struct device *dev, off_t offset, uint8_t *buffer,
				    size_t size)
{
	const struct rv3028_retained_mem_config *config = dev->config;
	const struct device *rtc_dev = config->parent;

	return rv3028_read_regs(rtc_dev, (RV3028_REG_USER_RAM1 + offset), buffer, size);
}

static int rv3028_retained_mem_write(const struct device *dev, off_t offset, const uint8_t *buffer,
				     size_t size)
{
	const struct rv3028_retained_mem_config *config = dev->config;
	const struct device *rtc_dev = config->parent;

	return rv3028_write_regs(rtc_dev, (RV3028_REG_USER_RAM1 + offset), buffer, size);
}

static int rv3028_retained_mem_clear(const struct device *dev)
{
	const struct rv3028_retained_mem_config *config = dev->config;
	const struct device *rtc_dev = config->parent;
	uint8_t block[RV3028_USER_RAM_SIZE] = {0};

	return rv3028_write_regs(rtc_dev, RV3028_REG_USER_RAM1, block, RV3028_USER_RAM_SIZE);
}

static int rv3028_retmem_init(const struct device *dev)
{
	const struct rv3028_retained_mem_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("parent device %s is not ready", config->parent->name);
		return -ENODEV;
	}

	return 0;
}

static const struct retained_mem_driver_api rv3028_retmem_api = {
	.size = rv3028_retained_mem_size,
	.read = rv3028_retained_mem_read,
	.write = rv3028_retained_mem_write,
	.clear = rv3028_retained_mem_clear,
};

#define RV3028_RETMEM_INIT(inst)                                                                   \
	static const struct rv3028_retained_mem_config rv3028_retained_mem_config_##inst = {       \
		.parent = DEVICE_DT_GET(DT_INST_BUS(inst)),                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, rv3028_retmem_init, NULL, NULL,                                \
			      &rv3028_retained_mem_config_##inst, POST_KERNEL,                     \
			      CONFIG_RETAINED_MEM_RV3028_INIT_PRIORITY, &rv3028_retmem_api);

DT_INST_FOREACH_STATUS_OKAY(RV3028_RETMEM_INIT);
