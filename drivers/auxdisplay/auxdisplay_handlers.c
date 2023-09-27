/*
 * Copyright (c) 2022-2023 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_auxdisplay_display_on(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_display_on(dev);
}
#include <syscalls/auxdisplay_display_on_mrsh.c>

static inline int z_vrfy_auxdisplay_display_off(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_display_off(dev);
}
#include <syscalls/auxdisplay_display_off_mrsh.c>

static inline int z_vrfy_auxdisplay_cursor_set_enabled(const struct device *dev, bool enabled)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_cursor_set_enabled(dev, enabled);
}
#include <syscalls/auxdisplay_cursor_set_enabled_mrsh.c>

static inline int z_vrfy_auxdisplay_position_blinking_set_enabled(const struct device *dev,
								  bool enabled)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_position_blinking_set_enabled(dev, enabled);
}
#include <syscalls/auxdisplay_position_blinking_set_enabled_mrsh.c>

static inline int z_vrfy_auxdisplay_cursor_shift_set(const struct device *dev, uint8_t direction,
						     bool display_shift)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_cursor_shift_set(dev, direction, display_shift);
}
#include <syscalls/auxdisplay_cursor_shift_set_mrsh.c>

static inline int z_vrfy_auxdisplay_cursor_position_set(const struct device *dev,
							enum auxdisplay_position type,
							int16_t x, int16_t y)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_cursor_position_set(dev, type, x, y);
}
#include <syscalls/auxdisplay_cursor_position_set_mrsh.c>

static inline int z_vrfy_auxdisplay_cursor_position_get(const struct device *dev, int16_t *x,
							int16_t *y)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_cursor_position_get(dev, x,  y);
}
#include <syscalls/auxdisplay_cursor_position_get_mrsh.c>

static inline int z_vrfy_auxdisplay_display_position_set(const struct device *dev,
							 enum auxdisplay_position type,
							 int16_t x, int16_t y)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_display_position_set(dev, type, x, y);
}
#include <syscalls/auxdisplay_display_position_set_mrsh.c>

static inline int z_vrfy_auxdisplay_display_position_get(const struct device *dev, int16_t *x,
							 int16_t *y)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_display_position_get(dev, x, y);
}
#include <syscalls/auxdisplay_display_position_get_mrsh.c>

static inline int z_vrfy_auxdisplay_capabilities_get(const struct device *dev,
						struct auxdisplay_capabilities *capabilities)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_capabilities_get(dev, capabilities);
}
#include <syscalls/auxdisplay_capabilities_get_mrsh.c>

static inline int z_vrfy_auxdisplay_clear(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_clear(dev);
}
#include <syscalls/auxdisplay_clear_mrsh.c>

static inline int z_vrfy_auxdisplay_brightness_get(const struct device *dev,
						   uint8_t *brightness)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_brightness_get(dev, brightness);
}
#include <syscalls/auxdisplay_brightness_get_mrsh.c>

static inline int z_vrfy_auxdisplay_brightness_set(const struct device *dev,
						   uint8_t brightness)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_brightness_set(dev, brightness);
}
#include <syscalls/auxdisplay_brightness_set_mrsh.c>

static inline int z_vrfy_auxdisplay_backlight_get(const struct device *dev,
						  uint8_t *backlight)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_backlight_get(dev, backlight);
}
#include <syscalls/auxdisplay_backlight_get_mrsh.c>

static inline int z_vrfy_auxdisplay_backlight_set(const struct device *dev,
						  uint8_t backlight)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_backlight_set(dev, backlight);
}
#include <syscalls/auxdisplay_backlight_set_mrsh.c>

static inline int z_vrfy_auxdisplay_is_busy(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_is_busy(dev);
}
#include <syscalls/auxdisplay_is_busy_mrsh.c>

static inline int z_vrfy_auxdisplay_custom_character_set(const struct device *dev,
							 struct auxdisplay_character *character)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_custom_character_set(dev, character);
}
#include <syscalls/auxdisplay_custom_character_set_mrsh.c>

static inline int z_vrfy_auxdisplay_write(const struct device *dev, const uint8_t *data,
					  uint16_t len)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_write(dev, data, len);
}
#include <syscalls/auxdisplay_write_mrsh.c>

static inline int z_vrfy_auxdisplay_custom_command(const struct device *dev,
						   struct auxdisplay_custom_data *data)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_AUXDISPLAY));
	return z_impl_auxdisplay_custom_command(dev, data);
}
#include <syscalls/auxdisplay_custom_command_mrsh.c>
