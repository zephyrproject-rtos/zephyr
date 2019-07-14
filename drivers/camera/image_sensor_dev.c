/*
 * Copyright (c) 2019 NXP Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <init.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <drivers/image_sensor.h>

#define LOG_LEVEL LOG_LEVEL_ERR
#include <logging/log.h>
LOG_MODULE_REGISTER(IMAGE_SENSOR_DEV);

#undef LOG_ERR
#define LOG_ERR printk

#undef LOG_INF
#define LOG_INF printk

/*Contain all the CMOS sensors
 * supported in current Zephyr.
 */
static bool z_img_sensor_support_list_init;
static sys_dlist_t z_img_sensor_support_list;

static K_MUTEX_DEFINE(z_img_sensor_support_lock);

void img_sensor_support_add(struct img_sensor_info *img_sensor)
{
	(void)k_mutex_lock(&z_img_sensor_support_lock, K_FOREVER);

	if (!z_img_sensor_support_list_init) {
		z_img_sensor_support_list_init = true;
		sys_dlist_init(&z_img_sensor_support_list);
	}

	sys_dlist_append(&z_img_sensor_support_list, &img_sensor->node);

	LOG_INF("Add image sensor (id %d) into support list\r\n",
		img_sensor->sensor_client.sensor_id);

	k_mutex_unlock(&z_img_sensor_support_lock);
}

/*Image sensors present on the device.
 * Image sensors devices are populated
 * in board DTS.
 */
static int z_img_sensor_num;
static struct device *z_img_sensor_dev[CAMERA_MAX_NUMBER];

static K_MUTEX_DEFINE(z_img_sensor_lock);

static inline int img_sensor_register(struct device *dev)
{
	int i;

	(void)k_mutex_lock(&z_img_sensor_lock, K_FOREVER);

	for (i = 0; i < z_img_sensor_num; i++) {
		if (z_img_sensor_dev[i] == dev) {
			k_mutex_unlock(&z_img_sensor_lock);
			return -EINVAL;
		}
	}
	if (z_img_sensor_num < CAMERA_MAX_NUMBER) {
		z_img_sensor_dev[z_img_sensor_num] = dev;
		z_img_sensor_num++;

		k_mutex_unlock(&z_img_sensor_lock);

		return 0;
	}

	k_mutex_unlock(&z_img_sensor_lock);

	return -ENOSPC;
}

struct device *img_sensor_get_by_id(enum camera_id id)
{
	int i;

	(void)k_mutex_lock(&z_img_sensor_lock, K_FOREVER);
	for (i = 0; i < z_img_sensor_num; i++) {
		struct img_sensor_data *drv_data =
			z_img_sensor_dev[i]->driver_data;

		if (drv_data->host_info.id == id) {
			k_mutex_unlock(&z_img_sensor_lock);
			return z_img_sensor_dev[i];
		}
	}

	k_mutex_unlock(&z_img_sensor_lock);
	return 0;
}

struct device *img_sensor_get_prime(void)
{
	return img_sensor_get_by_id(CAMERA_PRIMARY_ID);
}

static const struct img_sensor_info *img_sensor_scan_one(
	struct device *i2c_dev)
{
	int ret;
	sys_dnode_t *node, *next_node;

	(void)k_mutex_lock(&z_img_sensor_support_lock, K_FOREVER);

	SYS_DLIST_FOR_EACH_NODE_SAFE(&z_img_sensor_support_list,
		node, next_node) {
		const struct img_sensor_info *sensor_info =
			CONTAINER_OF(node, const struct img_sensor_info,
				node);
		const struct img_sensor_client *sensor_client =
			&sensor_info->sensor_client;
		u32_t sensor_id = 0;

		ret = i2c_write_read(i2c_dev, sensor_client->i2c_addr,
			&sensor_client->id_reg,
			sensor_client->w_id_reg,
			&sensor_id,
			sensor_client->w_sensor_id);
		if (!ret && sensor_id == sensor_client->sensor_id) {
			k_mutex_unlock(&z_img_sensor_support_lock);

			return sensor_info;
		}
	}
	k_mutex_unlock(&z_img_sensor_support_lock);

	return 0;
}

#if defined(DT_ZEPHYR_IMAGE_SENSOR_0) || defined(DT_ZEPHYR_IMAGE_SENSOR_1)
static int img_sensor_dev_init(struct device *dev)
{
	struct img_sensor_data *drv_data = dev->driver_data;
	int ret = -EINVAL;

	memset(drv_data, 0, sizeof(struct img_sensor_data));
	drv_data->host_info.id = CAMERA_NULL_ID;

#ifdef DT_ZEPHYR_IMAGE_SENSOR_0
	if (!strcmp(dev->config->name, IMAGE_SENSOR0_NAME)) {
		drv_data->host_info.i2c =
			device_get_binding(
			DT_INST_0_ZEPHYR_IMAGE_SENSOR_BUS_NAME);
		drv_data->host_info.pwr_gpio =
			device_get_binding(
			DT_INST_0_ZEPHYR_IMAGE_SENSOR_PWR_GPIOS_CONTROLLER);
		drv_data->host_info.pin =
			DT_INST_0_ZEPHYR_IMAGE_SENSOR_PWR_GPIOS_PIN;
		drv_data->host_info.flag =
			DT_INST_0_ZEPHYR_IMAGE_SENSOR_PWR_GPIOS_FLAGS;
		ret = img_sensor_register(dev);
		if (!ret) {
			LOG_INF("\r\nImage sensor 0 registered.\r\n");
		} else {
			LOG_ERR("\r\nImage sensor 0 un-registered.\r\n");
		}

		return ret;
	}
#endif

#ifdef DT_ZEPHYR_IMAGE_SENSOR_1
	if (!strcmp(dev->config->name, IMAGE_SENSOR1_NAME)) {
		drv_data->host_info.i2c =
			device_get_binding(
			DT_INST_1_ZEPHYR_IMAGE_SENSOR_BUS_NAME);
		drv_data->host_info.pwr_gpio =
			device_get_binding(
			DT_INST_1_ZEPHYR_IMAGE_SENSOR_PWR_GPIOS_CONTROLLER);
		drv_data->host_info.pin =
			DT_INST_1_ZEPHYR_IMAGE_SENSOR_PWR_GPIOS_PIN;
		drv_data->host_info.flag =
			DT_INST_1_ZEPHYR_IMAGE_SENSOR_PWR_GPIOS_FLAGS;
		ret = img_sensor_register(dev);
		if (!ret) {
			LOG_INF("\r\nImage sensor 1 registered.\r\n");
		} else {
			LOG_ERR("\r\nImage sensor 1 un-registered.\r\n");
		}

		return ret;
	}
#endif

	return ret;
}
#endif

static inline int img_sensor_power(
	struct img_sensor_host *host, bool on)
{
	int ret = gpio_pin_configure(host->pwr_gpio,
				host->pin, host->flag);

	if (ret) {
		return -EIO;
	}

	k_busy_wait(1);

	if (host->flag & GPIO_DIR_OUT) {
		if (on)
			ret = gpio_pin_write(host->pwr_gpio,	host->pin, 1);
		else
			ret = gpio_pin_write(host->pwr_gpio,	host->pin, 0);

		if (ret) {
			return -EIO;
		}
	}
	k_busy_wait(1000);

	return 0;
}

static inline void img_sensor_client_dup(
	struct img_sensor_client *drv_client,
	const struct img_sensor_client *scan_client)
{
	memcpy((char *)drv_client,
		(const char *)scan_client, sizeof(struct img_sensor_client));
}
struct device *img_sensor_scan(enum camera_id id)
{
	int i, ret;
	struct device *dev;
	struct img_sensor_data *drv_data;
	const struct img_sensor_info *scan_info;

	(void)k_mutex_lock(&z_img_sensor_lock, K_FOREVER);

	for (i = 0; i < z_img_sensor_num; i++) {
		dev = z_img_sensor_dev[i];
		drv_data = dev->driver_data;
		if (drv_data->host_info.id == CAMERA_NULL_ID) {
			if (drv_data->host_info.pwr_gpio) {
				ret = img_sensor_power(
					&drv_data->host_info,
					true);

				assert(!ret);
			}

			scan_info =
				img_sensor_scan_one(drv_data->host_info.i2c);
			if (scan_info) {
				LOG_INF("%s image sensor (id %d) is "
					"probed.\r\n",
					id == CAMERA_PRIMARY_ID ?
					"Primary" : "Secondary",
					scan_info->sensor_client.sensor_id);
				img_sensor_client_dup(
					&drv_data->client_info,
					&scan_info->sensor_client);

				drv_data->host_info.id = id;
				dev->driver_api =
					scan_info->sensor_api;

				k_mutex_unlock(&z_img_sensor_lock);

				return dev;
			}
			if (drv_data->host_info.pwr_gpio) {
				ret = img_sensor_power(
					&drv_data->host_info,
					false);

				assert(!ret);
			}
		}
	}
	k_mutex_unlock(&z_img_sensor_lock);

	return 0;
}

#ifdef DT_ZEPHYR_IMAGE_SENSOR_0
struct img_sensor_data img_sensor_data0;

DEVICE_AND_API_INIT(img_sensor_dev0,
		IMAGE_SENSOR0_NAME, img_sensor_dev_init,
		&img_sensor_data0, NULL, POST_KERNEL,
		CONFIG_IMAGE_SENSOR_INIT_PRIO,
		0);
#endif

#ifdef DT_ZEPHYR_IMAGE_SENSOR_1
struct img_sensor_data img_sensor_data1;

DEVICE_AND_API_INIT(img_sensor_dev1,
		IMAGE_SENSOR1_NAME, img_sensor_dev_init,
		&img_sensor_data1, NULL, POST_KERNEL,
		CONFIG_IMAGE_SENSOR_INIT_PRIO,
		0);
#endif
