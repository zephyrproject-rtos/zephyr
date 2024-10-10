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

struct rv3028_retained_mem_config {
	const struct device *parent;
	uint8_t addr;
	uint8_t size;
};

static ssize_t rv3028_retained_mem_size(const struct device *dev)
{
	const struct rv3028_retained_mem_config *config = dev->config;

	return config->size;
}

static int rv3028_retained_mem_read(const struct device *dev, off_t offset, uint8_t *buffer,
				    size_t size)
{
	const struct rv3028_retained_mem_config *config = dev->config;
	const struct device *rtc_dev = config->parent;

	return rv3028_read_regs(rtc_dev, config->addr + offset, buffer, size);
}

static int rv3028_retained_mem_write(const struct device *dev, off_t offset, const uint8_t *buffer,
				     size_t size)
{
	const struct rv3028_retained_mem_config *config = dev->config;
	const struct device *rtc_dev = config->parent;

	return rv3028_write_regs(rtc_dev, config->addr + offset, buffer, size);
}

static int rv3028_retained_mem_clear(const struct device *dev)
{
	const struct rv3028_retained_mem_config *config = dev->config;
	const struct device *rtc_dev = config->parent;
	uint8_t block[RV3028_USER_RAM_SIZE] = {0};

	return rv3028_write_regs(rtc_dev, config->addr, block, config->size);
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

#define RV3028_RETMEM_ASSERT_AREA_SIZE(inst)                                                       \
	BUILD_ASSERT(DT_INST_REG_ADDR(inst) >= RV3028_REG_USER_RAM1 &&                             \
			     DT_INST_REG_ADDR(inst) + DT_INST_REG_SIZE(inst) <=                    \
				     RV3028_REG_USER_RAM1 + RV3028_USER_RAM_SIZE,                  \
		     "Invalid RV3028 RAM area size");

#define RV3028_RETMEM_DEFINE(inst)                                                                 \
	RV3028_RETMEM_ASSERT_AREA_SIZE(inst)                                                       \
                                                                                                   \
	static const struct rv3028_retained_mem_config rv3028_retained_mem_config_##inst = {       \
		.parent = DEVICE_DT_GET(DT_INST_BUS(inst)),                                        \
		.addr = DT_INST_REG_ADDR(inst),                                                    \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, rv3028_retmem_init, NULL, NULL,                                \
			      &rv3028_retained_mem_config_##inst, POST_KERNEL,                     \
			      CONFIG_RETAINED_MEM_RV3028_INIT_PRIORITY, &rv3028_retmem_api);

DT_INST_FOREACH_STATUS_OKAY(RV3028_RETMEM_DEFINE);
