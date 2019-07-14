/*
 * Copyright (c) 2019 NXP Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/camera.h>
#include <drivers/display.h>
#include <syscall_handler.h>

Z_SYSCALL_HANDLER(camera_power, cam_dev, power)
{
	return z_impl_camera_power((struct device *)cam_dev,
		(bool)power);
}

Z_SYSCALL_HANDLER(camera_get_cap, cam_dev, cap)
{
	return z_impl_camera_get_cap((struct device *)cam_dev,
		(struct camera_capability *)cap);
}

Z_SYSCALL_HANDLER(camera_reset, cam_dev)
{
	return z_impl_camera_reset((struct device *)cam_dev);
}

Z_SYSCALL_HANDLER(camera_configure, cam_dev,
	width, height, pixformat)
{
	return z_impl_camera_configure((struct device *)cam_dev,
		(u16_t)width, (u16_t)height,
		(enum display_pixel_format)pixformat);
}

Z_SYSCALL_HANDLER(camera_start, cam_dev,
	mode, bufs, buf_num, cb)
{
	return z_impl_camera_start((struct device *)cam_dev,
		(enum camera_mode)mode, (void **)bufs, (u8_t)buf_num,
		(camera_capture_cb)cb);
}

Z_SYSCALL_HANDLER(camera_acquire_fb, cam_dev,
	fb, timeout)
{
	return z_impl_camera_acquire_fb((struct device *)cam_dev,
		(void **)fb, (s32_t)timeout);
}

Z_SYSCALL_HANDLER(camera_release_fb, cam_dev,
	fb)
{
	return z_impl_camera_release_fb((struct device *)cam_dev,
		(void *)fb);
}
