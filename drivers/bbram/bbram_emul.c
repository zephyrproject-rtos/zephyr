/*
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_bbram_emul

#include <zephyr/drivers/bbram.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bbram, CONFIG_BBRAM_LOG_LEVEL);

/** Device config */
struct bbram_emul_config {
	/** BBRAM size (Unit:bytes) */
	int size;
};

/** Device data */
struct bbram_emul_data {
	/** Memory */
	uint8_t *data;
	/** Status register */
	struct {
		/** True if BBRAM is in an invalid state */
		uint8_t is_invalid : 1;
		/** True if BBRAM incurred a standby power failure */
		uint8_t standby_failure : 1;
		/** True if BBRAM incurred a power failure */
		uint8_t power_failure : 1;
	} status;
};

int bbram_emul_set_invalid(const struct device *dev, bool is_invalid)
{
	struct bbram_emul_data *data = dev->data;

	data->status.is_invalid = is_invalid;
	return 0;
}

int bbram_emul_set_standby_power_state(const struct device *dev, bool failure)
{
	struct bbram_emul_data *data = dev->data;

	data->status.standby_failure = failure;
	return 0;
}

int bbram_emul_set_power_state(const struct device *dev, bool failure)
{
	struct bbram_emul_data *data = dev->data;

	data->status.power_failure = failure;
	return 0;
}

static int bbram_emul_check_invalid(const struct device *dev)
{
	struct bbram_emul_data *data = dev->data;
	bool is_invalid = data->status.is_invalid;

	data->status.is_invalid = false;
	return is_invalid;
}

static int bbram_emul_check_standby_power(const struct device *dev)
{
	struct bbram_emul_data *data = dev->data;
	bool failure = data->status.standby_failure;

	data->status.standby_failure = false;
	return failure;
}

static int bbram_emul_check_power(const struct device *dev)
{
	struct bbram_emul_data *data = dev->data;
	bool failure = data->status.power_failure;

	data->status.power_failure = false;
	return failure;
}

static int bbram_emul_get_size(const struct device *dev, size_t *size)
{
	const struct bbram_emul_config *config = dev->config;

	*size = config->size;
	return 0;
}

static int bbram_emul_read(const struct device *dev, size_t offset, size_t size,
			   uint8_t *data)
{
	const struct bbram_emul_config *config = dev->config;
	struct bbram_emul_data *dev_data = dev->data;

	if (size < 1 || offset + size > config->size || bbram_emul_check_invalid(dev)) {
		return -EFAULT;
	}

	memcpy(data, dev_data->data + offset, size);
	return 0;
}

static int bbram_emul_write(const struct device *dev, size_t offset, size_t size,
			    const uint8_t *data)
{
	const struct bbram_emul_config *config = dev->config;
	struct bbram_emul_data *dev_data = dev->data;

	if (size < 1 || offset + size > config->size || bbram_emul_check_invalid(dev)) {
		return -EFAULT;
	}

	memcpy(dev_data->data + offset, data, size);
	return 0;
}

static const struct bbram_driver_api bbram_emul_driver_api = {
	.check_invalid = bbram_emul_check_invalid,
	.check_standby_power = bbram_emul_check_standby_power,
	.check_power = bbram_emul_check_power,
	.get_size = bbram_emul_get_size,
	.read = bbram_emul_read,
	.write = bbram_emul_write,
};

static int bbram_emul_init(const struct device *dev)
{
	return 0;
}

#define BBRAM_INIT(inst)                                                                           \
	static uint8_t bbram_emul_mem_##inst[DT_INST_PROP(inst, size)];                            \
	static struct bbram_emul_data bbram_emul_data_##inst = {                                   \
		.data = bbram_emul_mem_##inst,                                                     \
	};                                                                                         \
	static struct bbram_emul_config bbram_emul_config_##inst = {                               \
		.size = DT_INST_PROP(inst, size),                                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &bbram_emul_init, NULL, &bbram_emul_data_##inst,               \
			      &bbram_emul_config_##inst, PRE_KERNEL_1, CONFIG_BBRAM_INIT_PRIORITY, \
			      &bbram_emul_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_INIT);
