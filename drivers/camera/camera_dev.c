/*
 * Copyright (c) 2019 NXP Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/camera_drv.h>
#define LOG_LEVEL LOG_LEVEL_ERR
#include <logging/log.h>
#define CAMERA_DEV_DBG

#undef LOG_ERR
#define LOG_ERR printk

#ifdef CAMERA_DEV_DBG
#undef LOG_INF
#define LOG_INF printk
#endif

static int z_camera_num;
static struct device *z_camera_dev[CAMERA_MAX_NUMBER];

#define CAMERA_DRV_DATA_MAX_SIZE 1024
static u8_t z_camera_drv_data[CAMERA_MAX_NUMBER *
	CAMERA_DRV_DATA_MAX_SIZE] __aligned(64);

static K_MUTEX_DEFINE(z_camera_lock);

int camera_dev_get_cap(struct device *cam_dev,
		struct camera_capability *cap)
{
	struct camera_driver_data *data = cam_dev->driver_data;

	memcpy((char *)cap, (char *)&data->cap,
		sizeof(struct camera_capability));

	return 0;
}

int  camera_dev_configure(struct device *cam_dev,
		struct camera_fb_cfg *fb_cfg)
{
	struct camera_driver_data *data = cam_dev->driver_data;

	if (fb_cfg->cfg_mode == CAMERA_DEFAULT_CFG) {
		memcpy((char *)&fb_cfg->fb_attr,
			(char *)&data->fb_attr,
			sizeof(struct camera_fb_attr));
	} else {
		memcpy((char *)&data->fb_attr,
			(char *)&fb_cfg->fb_attr,
			sizeof(struct camera_fb_attr));
	}

	return 0;
}

int camera_dev_register(struct device *dev)
{
	(void)k_mutex_lock(&z_camera_lock, K_FOREVER);

	if (z_camera_num < CAMERA_MAX_NUMBER) {
		z_camera_dev[z_camera_num] = dev;
		z_camera_num++;

		k_mutex_unlock(&z_camera_lock);

		return 0;
	}

	k_mutex_unlock(&z_camera_lock);

	return -ENOSPC;
}

struct camera_driver_data *camera_drv_data_alloc(
	u32_t priv_size, enum camera_id id, bool clear)
{
	struct camera_driver_data *data;

	if ((priv_size + sizeof(struct camera_driver_data)) >
		CAMERA_DRV_DATA_MAX_SIZE) {

		LOG_ERR("Camera data alloc size %d exceeds max size\r\n",
			(int)(CAMERA_DRV_DATA_MAX_SIZE -
			sizeof(struct camera_driver_data)));
		return 0;
	}

	if (id != CAMERA_PRIMARY_ID &&
		id != CAMERA_SECONDARY_ID) {

		LOG_ERR("Camera data alloc id %d is illegal\r\n", id);
		return 0;
	}

	data = (struct camera_driver_data *)
		&z_camera_drv_data[(id - CAMERA_PRIMARY_ID) *
		CAMERA_DRV_DATA_MAX_SIZE];
	if (clear) {
		memset((char *)data, 0, sizeof(struct camera_driver_data));
	}
	data->id = id;

	return data;
}

static struct device *camera_get_by_id(enum camera_id id)
{
	int i;
	struct device *dev;
	struct camera_driver_data *camera_data;

	(void)k_mutex_lock(&z_camera_lock, K_FOREVER);

	for (i = 0; i < z_camera_num; i++) {
		dev = z_camera_dev[i];
		camera_data = dev->driver_data;
		if (camera_data->id == id) {

			k_mutex_unlock(&z_camera_lock);

			return dev;
		}
	}

	k_mutex_unlock(&z_camera_lock);

	return 0;
}

struct device *camera_get_primary(void)
{
	return camera_get_by_id(CAMERA_PRIMARY_ID);
}

struct device *camera_get_secondary(void)
{
	return camera_get_by_id(CAMERA_SECONDARY_ID);
}
