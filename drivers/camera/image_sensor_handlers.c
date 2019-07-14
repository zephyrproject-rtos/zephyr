/*
 * Copyright (c) 2019 NXP Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/image_sensor.h>
#include <syscall_handler.h>

Z_SYSCALL_HANDLER(img_sensor_reset, dev)
{
	return z_impl_img_sensor_reset((struct device *)dev);
}

Z_SYSCALL_HANDLER(img_sensor_get_cap, dev, cap)
{
	return z_impl_img_sensor_get_cap((struct device *)dev,
		(struct img_sensor_capability *)cap);
}

Z_SYSCALL_HANDLER(img_sensor_read_reg, dev,
	addr, addr_width, data, data_width)
{
	return z_impl_img_sensor_read_reg((struct device *)dev,
		(u32_t)addr, (u8_t)addr_width, (u8_t *)data, (u8_t)data_width);
}

Z_SYSCALL_HANDLER(img_sensor_write_reg, dev,
	addr, addr_width, data, data_width)
{
	return z_impl_img_sensor_write_reg((struct device *)dev,
		(u32_t)addr, (u8_t)addr_width, (u32_t)data, (u8_t)data_width);
}

Z_SYSCALL_HANDLER(img_sensor_set_pixformat, dev,
	pixformat)
{
	return z_impl_img_sensor_set_pixformat((struct device *)dev,
		(enum img_sensor_pixel_format)pixformat);
}

Z_SYSCALL_HANDLER(img_sensor_set_framesize, dev,
	framesize)
{
	return z_impl_img_sensor_set_framesize((struct device *)dev,
		(enum img_sensor_frame_size)framesize);
}

Z_SYSCALL_HANDLER(img_sensor_set_contrast, dev, level)
{
	return z_impl_img_sensor_set_contrast((struct device *)dev,
		(int)level);
}

Z_SYSCALL_HANDLER(img_sensor_set_brightness, dev, level)
{
	return z_impl_img_sensor_set_brightness((struct device *)dev,
		(int)level);
}

Z_SYSCALL_HANDLER(img_sensor_set_gain, dev,
	r_gain_db, g_gain_db, b_gain_db)
{
	return z_impl_img_sensor_set_gain((struct device *)dev,
		(float)r_gain_db, (float)g_gain_db, (float)b_gain_db);
}

Z_SYSCALL_HANDLER(img_sensor_set_effect, dev, effect)
{
	return z_impl_img_sensor_set_effect((struct device *)dev,
		(enum img_sensor_effect)effect);
}

Z_SYSCALL_HANDLER(img_sensor_configure, dev)
{
	return z_impl_img_sensor_configure((struct device *)dev);
}
