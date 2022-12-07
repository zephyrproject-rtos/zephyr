/*
 * Copyright 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/regulator.h>

void regulator_common_data_init(const struct device *dev)
{
	struct regulator_common_data *data =
		(struct regulator_common_data *)dev->data;

	(void)k_mutex_init(&data->lock);
	data->refcnt = 0;
}

int regulator_enable(const struct device *dev)
{
	struct regulator_common_data *data =
		(struct regulator_common_data *)dev->data;
	int ret = 0;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	data->refcnt++;

	if (data->refcnt == 1) {
		const struct regulator_driver_api *api =
			(const struct regulator_driver_api *)dev->api;

		ret = api->enable(dev);
		if (ret < 0) {
			data->refcnt--;
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

int regulator_disable(const struct device *dev)
{
	struct regulator_common_data *data =
		(struct regulator_common_data *)dev->data;
	int ret = 0;

	(void)k_mutex_lock(&data->lock, K_FOREVER);

	data->refcnt--;

	if (data->refcnt == 0) {
		const struct regulator_driver_api *api =
			(const struct regulator_driver_api *)dev->api;

		ret = api->disable(dev);
		if (ret < 0) {
			data->refcnt++;
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}
