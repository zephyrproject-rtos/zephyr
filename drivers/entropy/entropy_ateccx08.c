/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_ateccx08_entropy

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/mfd/ateccx08.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(entropy_ateccx08, CONFIG_ENTROPY_LOG_LEVEL);

#define ATECCX08_RANDOM_SIZE 32

struct entropy_ateccx08_config {
	const struct device *parent;
};

struct entropy_ateccx08_data {
	uint8_t random_buffer[ATECCX08_RANDOM_SIZE];
	size_t buffer_remaining;
	struct k_sem sem_lock;
};

static int entropy_ateccx08_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	const struct entropy_ateccx08_config *config = dev->config;
	struct entropy_ateccx08_data *dev_data = dev->data;

	int ret = 0;

	if (atecc_is_locked_config(config->parent) == false) {
		LOG_ERR("Config not locked, no random data available.");
		return -EPERM;
	}

	k_sem_take(&dev_data->sem_lock, K_FOREVER);

	if (dev_data->buffer_remaining > 0) {
		size_t to_copy = MIN(length, dev_data->buffer_remaining);
		uint8_t *pos = dev_data->random_buffer + sizeof(dev_data->random_buffer) -
			       dev_data->buffer_remaining;

		memcpy(buffer, pos, to_copy);
		buffer += to_copy;
		length -= to_copy;
		dev_data->buffer_remaining -= to_copy;
	}

	while (length > 0) {
		size_t to_copy;

		ret = atecc_random(config->parent, dev_data->random_buffer, true);
		if (ret < 0) {
			goto exit;
		}

		to_copy = MIN(length, sizeof(dev_data->random_buffer));

		memcpy(buffer, dev_data->random_buffer, to_copy);
		buffer += to_copy;
		length -= to_copy;
		dev_data->buffer_remaining = sizeof(dev_data->random_buffer) - to_copy;
	}

exit:
	k_sem_give(&dev_data->sem_lock);
	return ret;
}

static int entropy_ateccx08_init(const struct device *dev)
{
	const struct entropy_ateccx08_config *config = dev->config;
	struct entropy_ateccx08_data *dev_data = dev->data;

	if (!device_is_ready(config->parent)) {
		return -ENODEV;
	}

	k_sem_init(&dev_data->sem_lock, 1, 1);

	return 0;
}

static const struct entropy_driver_api entropy_ateccx08_api = {
	.get_entropy = entropy_ateccx08_get_entropy};

BUILD_ASSERT(CONFIG_ENTROPY_ATECCX08_INIT_PRIORITY >= CONFIG_MFD_ATECCX08_INIT_PRIORITY,
	     "ATECCX08 entropy driver must be initialized after the mfd driver");

#define DEFINE_ATECCX08_ENTROPY(_num)                                                              \
	static const struct entropy_ateccx08_config entropy_ateccx08_config##_num = {              \
		.parent = DEVICE_DT_GET(DT_INST_BUS(_num))};                                       \
	static struct entropy_ateccx08_data entropy_ateccx08_data##_num;                           \
	DEVICE_DT_INST_DEFINE(_num, entropy_ateccx08_init, NULL, &entropy_ateccx08_data##_num,     \
			      &entropy_ateccx08_config##_num, POST_KERNEL,                         \
			      CONFIG_ENTROPY_ATECCX08_INIT_PRIORITY, &entropy_ateccx08_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_ATECCX08_ENTROPY);
