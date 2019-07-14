/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for camera drivers
 */

#ifndef ZEPHYR_INCLUDE_CAMERA_DRV_H_
#define ZEPHYR_INCLUDE_CAMERA_DRV_H_

#include <drivers/camera.h>
#include <drivers/camera_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct camera_driver_data
 * @brief Structure to describe camera driver data
 * Camera driver is supposed contain this data.
 */
struct camera_driver_data {
	struct camera_fb_attr fb_attr;
	struct camera_capability cap;
	camera_capture_cb customer_cb;
	enum camera_mode mode;
	enum camera_id id;

	/*Image sensor device associated to
	 * this camera, whihc could be no present
	 * for USB camera, PCIe camera..
	 */
	struct device *sensor_dev;

	/*Point to the camera driver priviate data*/
	u32_t priv[];
};

/**
 * @brief Alloc camera driver data
 * This function is called by camera driver.
 *
 * @param priv_size size of private data of camera driver
 *
 * @param id Primary camera or secondary camera
 *
 * @param clear TRUE to clear camera driver data
 *
 * @retval camera driver data.
 *
 */
struct camera_driver_data *camera_drv_data_alloc(
	u32_t priv_size, enum camera_id id, bool clear);


/**
 * @brief Get camera driver private data
 * This function is called by camera driver.
 *
 * @param data Pointer to camera driver structure
 *
 * @retval camera driver private data.
 *
 */
static inline void *camera_data_priv(
	struct camera_driver_data *data)
{
	return (void *)data->priv;
}

/**
 * @brief Default function to get capability of camera
 * This function is called by camera driver.
 *
 * @param cam_dev Pointer to camera device structure
 *
 * @param cap Pointer to camera capability structure
 * filled by camera driver
 *
 * @retval 0 on success else negative errno code.
 */
int camera_dev_get_cap(struct device *cam_dev,
		struct camera_capability *cap);

/**
 * @brief Default funciotn to configure frame buffer of camera
 * This function is called by camera driver.
 *
 * @param cam_dev Pointer to camera device structure
 *
 * @param fb_cfg Pointer to frame buffer configuration
 * structure of camera
 *
 * @retval 0 on success else negative errno code.
 */

int camera_dev_configure(struct device *cam_dev,
		struct camera_fb_cfg *fb_cfg);


/**
 * @brief Register camera device to camera subsys
 * This function is called by camera driver.
 *
 * @param cam_dev Pointer to camera device structure
 *
 * @retval 0 on success else negative errno code.
 *
 */
int camera_dev_register(struct device *cam_dev);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CAMERA_SRV_H_*/
