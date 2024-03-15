/*
 * Copyright (c) 2018 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/led.h>

static inline int z_vrfy_led_blink(const struct device *dev, uint32_t led,
				   uint32_t delay_on, uint32_t delay_off)
{
	K_OOPS(K_SYSCALL_DRIVER_LED(dev, blink));
	return z_impl_led_blink((const struct device *)dev, led, delay_on,
					delay_off);
}
#include <syscalls/led_blink_mrsh.c>

static inline int z_vrfy_led_get_info(const struct device *dev, uint32_t led,
				      const struct led_info **info)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_LED));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(info, sizeof(*info)));
	return z_impl_led_get_info(dev, led, info);
}
#include <syscalls/led_get_info_mrsh.c>

static inline int z_vrfy_led_set_brightness(const struct device *dev,
					    uint32_t led,
					    uint8_t value)
{
	K_OOPS(K_SYSCALL_DRIVER_LED(dev, set_brightness));
	return z_impl_led_set_brightness((const struct device *)dev, led,
					 value);
}
#include <syscalls/led_set_brightness_mrsh.c>

static inline int
z_vrfy_led_write_channels(const struct device *dev, uint32_t start_channel,
			  uint32_t num_channels, const uint8_t *buf)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_LED));
	K_OOPS(K_SYSCALL_MEMORY_READ(buf, num_channels));
	return z_impl_led_write_channels(dev, start_channel, num_channels, buf);
}
#include <syscalls/led_write_channels_mrsh.c>

static inline int z_vrfy_led_set_channel(const struct device *dev,
					 uint32_t channel, uint8_t value)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_LED));
	return z_impl_led_set_channel(dev, channel, value);
}
#include <syscalls/led_set_channel_mrsh.c>

static inline int z_vrfy_led_set_color(const struct device *dev, uint32_t led,
				       uint8_t num_colors, const uint8_t *color)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_LED));
	K_OOPS(K_SYSCALL_MEMORY_READ(color, num_colors));
	return z_impl_led_set_color(dev, led, num_colors, color);
}
#include <syscalls/led_set_color_mrsh.c>

static inline int z_vrfy_led_on(const struct device *dev, uint32_t led)
{
	K_OOPS(K_SYSCALL_DRIVER_LED(dev, on));
	return z_impl_led_on((const struct device *)dev, led);
}
#include <syscalls/led_on_mrsh.c>

static inline int z_vrfy_led_off(const struct device *dev, uint32_t led)
{
	K_OOPS(K_SYSCALL_DRIVER_LED(dev, off));
	return z_impl_led_off((const struct device *)dev, led);
}
#include <syscalls/led_off_mrsh.c>
