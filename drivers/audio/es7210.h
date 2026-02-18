/*
 * Copyright (c) 2026 Zhang Xingtao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_ES7210_H_
#define ZEPHYR_DRIVERS_AUDIO_ES7210_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ES7210_CHANNEL_1 BIT(0)
#define ES7210_CHANNEL_2 BIT(1)
#define ES7210_CHANNEL_3 BIT(2)
#define ES7210_CHANNEL_4 BIT(3)
#define ES7210_CHANNEL_ALL                                                                      \
	(ES7210_CHANNEL_1 | ES7210_CHANNEL_2 | ES7210_CHANNEL_3 | ES7210_CHANNEL_4)

__subsystem struct es7210_driver_api {
	int (*read_reg)(const struct device *dev, uint8_t reg, uint8_t *val);
	int (*write_reg)(const struct device *dev, uint8_t reg, uint8_t val);
	int (*update_reg)(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val);
	int (*apply_init)(const struct device *dev);
	int (*set_i2s_format)(const struct device *dev, uint8_t sdp_interface1, uint8_t sdp_interface2);
	int (*set_mic_gain)(const struct device *dev, uint8_t channel_mask, uint8_t gain_regval);
};

static inline int es7210_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct es7210_driver_api *api = (const struct es7210_driver_api *)dev->api;

	return (api->read_reg != NULL) ? api->read_reg(dev, reg, val) : -ENOSYS;
}

static inline int es7210_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct es7210_driver_api *api = (const struct es7210_driver_api *)dev->api;

	return (api->write_reg != NULL) ? api->write_reg(dev, reg, val) : -ENOSYS;
}

static inline int es7210_update_reg(const struct device *dev, uint8_t reg, uint8_t mask,
				    uint8_t val)
{
	const struct es7210_driver_api *api = (const struct es7210_driver_api *)dev->api;

	return (api->update_reg != NULL) ? api->update_reg(dev, reg, mask, val) : -ENOSYS;
}

static inline int es7210_apply_init(const struct device *dev)
{
	const struct es7210_driver_api *api = (const struct es7210_driver_api *)dev->api;

	return (api->apply_init != NULL) ? api->apply_init(dev) : -ENOSYS;
}

static inline int es7210_set_i2s_format(const struct device *dev, uint8_t sdp_interface1,
					uint8_t sdp_interface2)
{
	const struct es7210_driver_api *api = (const struct es7210_driver_api *)dev->api;

	return (api->set_i2s_format != NULL) ? api->set_i2s_format(dev, sdp_interface1, sdp_interface2)
					      : -ENOSYS;
}

static inline int es7210_set_mic_gain(const struct device *dev, uint8_t channel_mask,
				      uint8_t gain_regval)
{
	const struct es7210_driver_api *api = (const struct es7210_driver_api *)dev->api;

	return (api->set_mic_gain != NULL) ? api->set_mic_gain(dev, channel_mask, gain_regval) : -ENOSYS;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_ES7210_H_ */
