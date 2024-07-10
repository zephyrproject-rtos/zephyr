/*
 * Copyright (c) 2022-2023 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for auxiliary display driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_AUXDISPLAY_H_
#error "Should only be included by zephyr/drivers/auxdisplay.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_AUXDISPLAY_INTERNAL_AUXDISPLAY_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_AUXDISPLAY_INTERNAL_AUXDISPLAY_IMPL_H_

#include <stdint.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_auxdisplay_display_on(const struct device *dev)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->display_on) {
		return -ENOSYS;
	}

	return api->display_on(dev);
}

static inline int z_impl_auxdisplay_display_off(const struct device *dev)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->display_off) {
		return -ENOSYS;
	}

	return api->display_off(dev);
}

static inline int z_impl_auxdisplay_cursor_set_enabled(const struct device *dev,
						       bool enabled)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->cursor_set_enabled) {
		return -ENOSYS;
	}

	return api->cursor_set_enabled(dev, enabled);
}

static inline int z_impl_auxdisplay_position_blinking_set_enabled(const struct device *dev,
								  bool enabled)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->position_blinking_set_enabled) {
		return -ENOSYS;
	}

	return api->position_blinking_set_enabled(dev, enabled);
}

static inline int z_impl_auxdisplay_cursor_shift_set(const struct device *dev,
						     uint8_t direction,
						     bool display_shift)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->cursor_shift_set) {
		return -ENOSYS;
	}

	if (direction >= AUXDISPLAY_DIRECTION_COUNT) {
		return -EINVAL;
	}

	return api->cursor_shift_set(dev, direction, display_shift);
}

static inline int z_impl_auxdisplay_cursor_position_set(const struct device *dev,
							enum auxdisplay_position type,
							int16_t x, int16_t y)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->cursor_position_set) {
		return -ENOSYS;
	} else if (type >= AUXDISPLAY_POSITION_COUNT) {
		return -EINVAL;
	} else if (type == AUXDISPLAY_POSITION_ABSOLUTE && (x < 0 || y < 0)) {
		return -EINVAL;
	}

	return api->cursor_position_set(dev, type, x, y);
}

static inline int z_impl_auxdisplay_cursor_position_get(const struct device *dev,
							int16_t *x, int16_t *y)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->cursor_position_get) {
		return -ENOSYS;
	}

	return api->cursor_position_get(dev, x, y);
}

static inline int z_impl_auxdisplay_display_position_set(const struct device *dev,
							 enum auxdisplay_position type,
							 int16_t x, int16_t y)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->display_position_set) {
		return -ENOSYS;
	} else if (type >= AUXDISPLAY_POSITION_COUNT) {
		return -EINVAL;
	} else if (type == AUXDISPLAY_POSITION_ABSOLUTE && (x < 0 || y < 0)) {
		return -EINVAL;
	}

	return api->display_position_set(dev, type, x, y);
}

static inline int z_impl_auxdisplay_display_position_get(const struct device *dev,
							 int16_t *x, int16_t *y)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->display_position_get) {
		return -ENOSYS;
	}

	return api->display_position_get(dev, x, y);
}

static inline int z_impl_auxdisplay_capabilities_get(const struct device *dev,
						     struct auxdisplay_capabilities *capabilities)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	return api->capabilities_get(dev, capabilities);
}

static inline int z_impl_auxdisplay_clear(const struct device *dev)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	return api->clear(dev);
}

static inline int z_impl_auxdisplay_brightness_get(const struct device *dev,
						   uint8_t *brightness)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->brightness_get) {
		return -ENOSYS;
	}

	return api->brightness_get(dev, brightness);
}

static inline int z_impl_auxdisplay_brightness_set(const struct device *dev,
						   uint8_t brightness)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->brightness_set) {
		return -ENOSYS;
	}

	return api->brightness_set(dev, brightness);
}

static inline int z_impl_auxdisplay_backlight_get(const struct device *dev,
						  uint8_t *backlight)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->backlight_get) {
		return -ENOSYS;
	}

	return api->backlight_get(dev, backlight);
}

static inline int z_impl_auxdisplay_backlight_set(const struct device *dev,
						  uint8_t backlight)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->backlight_set) {
		return -ENOSYS;
	}

	return api->backlight_set(dev, backlight);
}

static inline int z_impl_auxdisplay_is_busy(const struct device *dev)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->is_busy) {
		return -ENOSYS;
	}

	return api->is_busy(dev);
}

static inline int z_impl_auxdisplay_custom_character_set(const struct device *dev,
							 struct auxdisplay_character *character)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->custom_character_set) {
		return -ENOSYS;
	}

	return api->custom_character_set(dev, character);
}

static inline int z_impl_auxdisplay_write(const struct device *dev,
					  const uint8_t *data, uint16_t len)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	return api->write(dev, data, len);
}

static inline int z_impl_auxdisplay_custom_command(const struct device *dev,
						   struct auxdisplay_custom_data *data)
{
	struct auxdisplay_driver_api *api = (struct auxdisplay_driver_api *)dev->api;

	if (!api->custom_command) {
		return -ENOSYS;
	}

	return api->custom_command(dev, data);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_AUXDISPLAY_INTERNAL_AUXDISPLAY_IMPL_H_ */
