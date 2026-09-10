/*
 * Copyright (c) 2026, Janez Ugovsek <janez@ugovsek.info>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv3028_ram

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <rv3028.h>

LOG_MODULE_REGISTER(retained_mem_rv3028, CONFIG_RETAINED_MEM_LOG_LEVEL);

struct config {
	const struct device *mfd;
};

static ssize_t retained_mem_ram_size(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (ssize_t)RV3028_RAM_SIZE;
}

static int retained_mem_ram_read(const struct device *dev, off_t offset, uint8_t *buffer,
				 size_t size)
{
	const struct config *config = dev->config;
	int err;

	if ((offset < 0) || ((offset + size) > RV3028_RAM_SIZE)) {
		return -EINVAL;
	}

	mfd_rv3028_lock_sem(config->mfd);
	err = mfd_rv3028_read_regs(config->mfd, RV3028_REG_USER_RAM1 + offset, buffer, size);
	if (err) {
		goto unlock;
	}

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

static int retained_mem_ram_write(const struct device *dev, off_t offset, const uint8_t *buffer,
				  size_t size)
{
	const struct config *config = dev->config;
	int err = 0;

	if ((offset < 0) || ((offset + size) > RV3028_RAM_SIZE)) {
		return -EINVAL;
	}

	mfd_rv3028_lock_sem(config->mfd);
	err = mfd_rv3028_write_regs(config->mfd, RV3028_REG_USER_RAM1 + offset, buffer, size);
	if (err) {
		goto unlock;
	}

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

static int retained_mem_ram_clear(const struct device *dev)
{
	const struct config *config = dev->config;
	int err = 0;
	uint8_t buff[RV3028_RAM_SIZE];

	memset(buff, 0, sizeof(buff));

	mfd_rv3028_lock_sem(config->mfd);
	err = mfd_rv3028_write_regs(config->mfd, RV3028_REG_USER_RAM1, buff, sizeof(buff));
	if (err) {
		goto unlock;
	}

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

static DEVICE_API(retained_mem, retained_mem_ram_api) = {
	.size = retained_mem_ram_size,
	.read = retained_mem_ram_read,
	.write = retained_mem_ram_write,
	.clear = retained_mem_ram_clear,
};

#define INIT(inst)                                                                                 \
	static const struct config config_##inst = {                                               \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &config_##inst, POST_KERNEL,                 \
			      CONFIG_RETAINED_MEM_RV3028_INIT_PRIORITY, &retained_mem_ram_api);

DT_INST_FOREACH_STATUS_OKAY(INIT)
