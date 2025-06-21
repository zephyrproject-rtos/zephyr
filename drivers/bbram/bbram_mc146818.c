/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Read and Write access to offset ranges 0x2A(42)-0x31(49) and 0xAA(170)-0xB1(177)
 * are lockable through BIOS setting. To access the memory in those offsets,
 * disable the Lock in BIOS through following steps.
 * Intel Advanced Menu -> PCH-IO Configuration -> Security Configuration ->
 * RTC Memory Lock -> Disable
 */

#define DT_DRV_COMPAT motorola_mc146818_bbram

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/rtc/mc146818.h>

#define MIN_SIZE	1 /* Minimum size to write */
#define MIN_OFFSET	0x0E /* Starting offset of memory */
#define MAX_STD		0x7F /* Last offset of Standard memory bank */
#define RTC_CENT	0x32 /* Offset for RTC Century Byte */

struct bbram_mc146818_config {
	const struct device *parent;
	io_port_t std_index;
	io_port_t std_target;
	io_port_t ext_index;
	io_port_t ext_target;
	size_t mem_size;
};

struct bbram_mc146818_data {
	struct k_spinlock *lock;
};

static uint8_t std_reg_read(const struct bbram_mc146818_config *cfg, int reg)
{

	sys_out8(reg, cfg->std_index);
	return sys_in8(cfg->std_target);
}

static uint8_t ext_reg_read(const struct bbram_mc146818_config *cfg, int reg)
{

	sys_out8(reg, cfg->ext_index);
	return sys_in8(cfg->ext_target);
}

static void std_reg_write(const struct bbram_mc146818_config *cfg, uint8_t value, int reg)
{
	sys_out8(reg, cfg->std_index);
	sys_out8(value, cfg->std_target);
}

static void ext_reg_write(const struct bbram_mc146818_config *cfg, uint8_t value, int reg)
{
	sys_out8(reg, cfg->ext_index);
	sys_out8(value, cfg->ext_target);
}

static int bbram_mc146818_read(const struct device *dev, size_t offset,
			    size_t size, uint8_t *data)
{
	const struct bbram_mc146818_config *config = dev->config;
	struct bbram_mc146818_data *dev_data = dev->data;

	if (size < MIN_SIZE || offset + size > config->mem_size
	    || data == NULL) {
		return -EFAULT;
	}

	offset += MIN_OFFSET;

	k_spinlock_key_t key = k_spin_lock(dev_data->lock);

	for (size_t i = 0; i < size; i++) {
		if (offset < MAX_STD) {
			if (offset >= RTC_CENT) {

			/* RTC_CENT byte is used to store Century data for the
			 * RTC time and date, so skipping read/write operation
			 * to this byte.
			 */

				*(data + i) = std_reg_read(config, offset+1);
			} else {
				*(data + i) = std_reg_read(config, offset);
			}
		} else {
			*(data + i) = ext_reg_read(config, offset+1);
		}
		offset++;
	}

	k_spin_unlock(dev_data->lock, key);
	return 0;
}

static int bbram_mc146818_write(const struct device *dev, size_t offset,
			     size_t size, const uint8_t *data)
{
	const struct bbram_mc146818_config *config = dev->config;
	struct bbram_mc146818_data *dev_data = dev->data;

	if (size < MIN_SIZE || offset + size > config->mem_size
	    || data == NULL) {
		return -EFAULT;
	}

	offset += MIN_OFFSET;

	k_spinlock_key_t key = k_spin_lock(dev_data->lock);

	for (size_t i = 0; i < size; i++) {
		if (offset < MAX_STD) {
			if (offset >= RTC_CENT) {

			/* RTC_CENT byte is used to store Century data for the
			 * RTC time and date, so skipping read/write operation
			 * to this byte.
			 */

				std_reg_write(config, *(data + i), offset+1);
			} else {
				std_reg_write(config, *(data + i), offset);
			}
		} else {
			ext_reg_write(config, *(data + i), offset+1);
		}
		offset++;
	}

	k_spin_unlock(dev_data->lock, key);
	return 0;
}

static int bbram_mc146818_get_size(const struct device *dev, size_t *size)
{
	const struct bbram_mc146818_config *config = dev->config;

	*size = config->mem_size;

	return 0;
}

static const struct bbram_driver_api bbram_mc146818_api = {
	.read = bbram_mc146818_read,
	.write = bbram_mc146818_write,
	.get_size = bbram_mc146818_get_size,
};

static int bbram_mc146818_init(const struct device *dev)
{
	const struct bbram_mc146818_config *config = dev->config;
	struct bbram_mc146818_data *dev_data = dev->data;
	struct rtc_mc146818_data *parent_data = config->parent->data;

	if (!device_is_ready(config->parent)) {
		return -ENODEV;
	}

	dev_data->lock = &parent_data->lock;
	return 0;
}

#define BBRAM_MC146818_DEV_CFG(n)					\
	static const struct bbram_mc146818_config bbram_config_##n = {	\
		.parent = DEVICE_DT_GET(DT_INST_PARENT(n)),		\
		.std_index = DT_REG_ADDR_BY_IDX(DT_INST_PARENT(n), 0),	\
		.std_target = DT_REG_ADDR_BY_IDX(DT_INST_PARENT(n), 1),	\
		.ext_index = DT_INST_REG_ADDR_BY_IDX(n, 0),		\
		.ext_target = DT_INST_REG_ADDR_BY_IDX(n, 1),		\
		.mem_size = DT_INST_PROP(n, size),			\
	};								\
	static struct bbram_mc146818_data bbram_data_##n;		\
	DEVICE_DT_INST_DEFINE(n, &bbram_mc146818_init, NULL,		\
				&bbram_data_##n, &bbram_config_##n,	\
				POST_KERNEL,				\
				CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
				&bbram_mc146818_api);			\

DT_INST_FOREACH_STATUS_OKAY(BBRAM_MC146818_DEV_CFG)
