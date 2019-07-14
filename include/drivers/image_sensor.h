/*
 * Copyright (c) 2019 NXP Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_IMG_SENSOR_H_
#define ZEPHYR_INCLUDE_IMG_SENSOR_H_

#include <zephyr/types.h>
#include <device.h>
#include <errno.h>
#include <drivers/camera_generic.h>
#include <drivers/display.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMAGE_SENSOR0_NAME "zephyr-img-sensor0"
#define IMAGE_SENSOR1_NAME "zephyr-img-sensor1"

enum img_sensor_effect {
	IMG_EFFECT_NORMAL,
	IMG_EFFECT_NEGATIVE,
};

enum bytes_width {
	W1B = 1,
	W2B = 2,
	W4B = 4
};

struct img_sensor_reg {
	u32_t reg;
	enum bytes_width w_reg;
	u32_t value;
	enum bytes_width w_value;
};

/*Public data placed at the beginning of driver data of device*/
struct img_sensor_host {
	/*Camera module associated to*/
	enum camera_id id;
	/*I2C device used for configuration*/
	struct device *i2c;
	/*GPIO device used for Power control, optional*/
	struct device *pwr_gpio;
	/*GPIO pin used for Power control, optional*/
	u32_t pin;
	/*GPIO level used for Power control, optional*/
	int flag;
};

struct img_sensor_capability {
	u32_t pixformat_support;
	u16_t width_max;
	u16_t height_max;
};

struct img_sensor_client {
	u16_t i2c_addr;
	u32_t sensor_id;
	enum bytes_width w_sensor_id;
	u32_t id_reg;
	enum bytes_width w_id_reg;
	int contrast_level;
	int bright_level;
	float r_gain_db;
	float g_gain_db;
	float b_gain_db;
	u16_t width;
	u16_t height;
	enum img_sensor_effect effect;
	enum display_pixel_format pixformat;
	struct img_sensor_capability cap;
};

struct img_sensor_data {
	struct img_sensor_host host_info;
	struct img_sensor_client client_info;
};

/*Master clock could be enabled by camer driver
 * before accessing CMOS.
 * Or disabled by camera driver when system is
 * entering sleep mode.
 */
typedef void  (*img_sensor_mclk_enable)(struct device *cam_dev, bool enable);

struct img_sensor_driver_api {
	int  (*img_sensor_reset_cb)(struct device *dev);
	int  (*img_sensor_get_cap_cb)(struct device *dev,
		struct img_sensor_capability *cap);
	int  (*img_sensor_read_reg_cb)(struct device *dev,
		u32_t addr, u8_t addr_width, u8_t *data, u8_t data_width);
	int  (*img_sensor_write_reg_cb)(struct device *dev,
		u32_t addr, u8_t addr_width, u32_t data, u8_t data_width);
	int  (*img_sensor_set_pixformat_cb)(struct device *dev,
		enum display_pixel_format pixformat);
	int  (*img_sensor_set_framesize_cb)(struct device *dev,
		u16_t width, u16_t height);
	int  (*img_sensor_set_contrast_cb)(struct device *dev,
		int level);
	int  (*img_sensor_set_brightness_cb)(struct device *dev,
		int level);
	int  (*img_sensor_set_rgb_gain_cb)(struct device *dev,
		float r_gain_db, float g_gain_db, float b_gain_db);
	int  (*img_sensor_set_effect_cb)(struct device *dev,
		enum img_sensor_effect effect);
	int  (*img_sensor_config_cb)(struct device *dev);
};

struct img_sensor_info {
	sys_dnode_t node;
	struct img_sensor_client sensor_client;
	const struct img_sensor_driver_api *sensor_api;
};

void img_sensor_support_add(
	struct img_sensor_info *img_sensor);

__syscall int img_sensor_reset(struct device *dev);

static inline int z_impl_img_sensor_reset(struct device *dev)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_reset_cb(dev);
}

__syscall int img_sensor_get_cap(struct device *dev,
	struct img_sensor_capability *cap);

static inline int z_impl_img_sensor_get_cap(struct device *dev,
	struct img_sensor_capability *cap)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_get_cap_cb(dev, cap);
}


__syscall int img_sensor_read_reg(struct device *dev,
	u32_t addr, u8_t addr_width, u8_t *data, u8_t data_width);

static inline int z_impl_img_sensor_read_reg(struct device *dev,
	u32_t addr, u8_t addr_width, u8_t *data, u8_t data_width)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_read_reg_cb(dev,
			addr, addr_width, data, data_width);
}

__syscall int img_sensor_write_reg(struct device *dev,
	u32_t addr, u8_t addr_width, u32_t data, u8_t data_width);

static inline int z_impl_img_sensor_write_reg(struct device *dev,
	u32_t addr, u8_t addr_width, u32_t data, u8_t data_width)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_write_reg_cb(dev,
			addr, addr_width, data, data_width);
}

__syscall int img_sensor_set_pixformat(struct device *dev,
	enum display_pixel_format pixformat);

static inline int z_impl_img_sensor_set_pixformat(struct device *dev,
	enum display_pixel_format pixformat)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_set_pixformat_cb(dev, pixformat);
}
__syscall int img_sensor_set_framesize(struct device *dev,
	u16_t width, u16_t height);

static inline int z_impl_img_sensor_set_framesize(struct device *dev,
	u16_t width, u16_t height)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_set_framesize_cb(dev, width, height);
}
__syscall int img_sensor_set_contrast(struct device *dev,
	int level);

static inline int z_impl_img_sensor_set_contrast(struct device *dev,
	int level)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_set_contrast_cb(dev, level);
}

__syscall int img_sensor_set_brightness(struct device *dev,
	int level);

static inline int z_impl_img_sensor_set_brightness(struct device *dev,
	int level)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_set_brightness_cb(dev, level);
}

__syscall int img_sensor_set_gain(struct device *dev,
	float r_gain_db, float g_gain_db, float b_gain_db);

static inline int z_impl_img_sensor_set_gain(struct device *dev,
	float r_gain_db, float g_gain_db, float b_gain_db)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_set_rgb_gain_cb(dev,
			r_gain_db, g_gain_db, b_gain_db);
}

__syscall int img_sensor_set_effect(struct device *dev,
	enum img_sensor_effect effect);

static inline int z_impl_img_sensor_set_effect(struct device *dev,
	enum img_sensor_effect effect)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_set_effect_cb(dev, effect);
}

__syscall int img_sensor_configure(struct device *dev);

static inline int z_impl_img_sensor_configure(struct device *dev)
{
	const struct img_sensor_driver_api *api = dev->driver_api;

	return api->img_sensor_config_cb(dev);
}

struct device *img_sensor_get_prime(void);

struct device *img_sensor_get_by_id(enum camera_id id);

struct device *img_sensor_scan(enum camera_id id);

#include <syscalls/image_sensor.h>

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IMG_SENSOR_H_ */
