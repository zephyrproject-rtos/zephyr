/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementations for Sensor driver APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_H_
#error "Should only be included by zephyr/drivers/sensor.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_INTERNAL_SENSOR_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_INTERNAL_SENSOR_IMPL_H_

#include <errno.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/dsp/types.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_sensor_attr_set(const struct device *dev,
					 enum sensor_channel chan,
					 enum sensor_attribute attr,
					 const struct sensor_value *val)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	if (api->attr_set == NULL) {
		return -ENOSYS;
	}

	return api->attr_set(dev, chan, attr, val);
}

static inline int z_impl_sensor_attr_get(const struct device *dev,
					 enum sensor_channel chan,
					 enum sensor_attribute attr,
					 struct sensor_value *val)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	if (api->attr_get == NULL) {
		return -ENOSYS;
	}

	return api->attr_get(dev, chan, attr, val);
}

static inline int z_impl_sensor_sample_fetch(const struct device *dev)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	return api->sample_fetch(dev, SENSOR_CHAN_ALL);
}

static inline int z_impl_sensor_sample_fetch_chan(const struct device *dev,
						  enum sensor_channel type)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	return api->sample_fetch(dev, type);
}

static inline int z_impl_sensor_channel_get(const struct device *dev,
					    enum sensor_channel chan,
					    struct sensor_value *val)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	return api->channel_get(dev, chan, val);
}

#if defined(CONFIG_SENSOR_ASYNC_API)

static inline int z_impl_sensor_get_decoder(const struct device *dev,
					    const struct sensor_decoder_api **decoder)
{
	const struct sensor_driver_api *api = (const struct sensor_driver_api *)dev->api;

	__ASSERT_NO_MSG(api != NULL);

	if (api->get_decoder == NULL) {
		*decoder = &__sensor_default_decoder;
		return 0;
	}

	return api->get_decoder(dev, decoder);
}

static inline int z_impl_sensor_reconfigure_read_iodev(struct rtio_iodev *iodev,
						       const struct device *sensor,
						       const struct sensor_chan_spec *channels,
						       size_t num_channels)
{
	struct sensor_read_config *cfg = (struct sensor_read_config *)iodev->data;

	if (cfg->max < num_channels || cfg->is_streaming) {
		return -ENOMEM;
	}

	cfg->sensor = sensor;
	memcpy(cfg->channels, channels, num_channels * sizeof(struct sensor_chan_spec));
	cfg->count = num_channels;
	return 0;
}

#endif /* defined(CONFIG_SENSOR_ASYNC_API) */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_INTERNAL_SENSOR_API_H_ */
