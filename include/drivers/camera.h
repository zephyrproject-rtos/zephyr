/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for camera applications
 */

#ifndef ZEPHYR_INCLUDE_CAMERA_H_
#define ZEPHYR_INCLUDE_CAMERA_H_

#include <device.h>
#include <stddef.h>
#include <zephyr/types.h>
#include <drivers/display.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Camera work flow:
 * camera_get_primary/secondary->camera_power->camera_get_cap->
 *                                                          |
 * <--camera_acquire_fb<---camera_start<-camera_configure<--|
 * |                     |
 * |                     |
 * |-->camera_release_fb-^
 *
 */

/**
 * @enum camera_mode
 * @brief Enumeration with camera running mode
 *
 */
enum camera_mode {
	/* If selected, only one frame is captured then
	 * camera stream is stopped.
	 */
	CAMERA_CAPTURE_MODE,
	/* If selected, series of frames are captured if
	 * there are buffers available.
	 */
	CAMERA_PREVIEW_MODE
};

/**
 * @typedef camera_capture_cb
 * @brief Callback API when one frame is received
 * This callback is supposed to be running in
 * interrupt context.
 * @param fb Pointer to frame buffer
 * @param w width of the frame received.
 * @param h height of the frame received.
 * @param bpp bits per pixel of the frame received.
 */
typedef void (*camera_capture_cb)(void *fb, int w, int h, int bpp);

/**
 * @struct camera_fb_attr
 * @brief Structure to describe attribution of
 * camera frame buffer
 *
 */
struct camera_fb_attr {
	u32_t width;
	u32_t height;
	u8_t bpp;
	/*Align the pixel format with display device.*/
	enum display_pixel_format pixformat;
};

/**
 * @enum camera_cfg_mode
 * @brief Enumeration to configure camera
 *
 */
enum camera_cfg_mode {
	/* If selected, configure camera with
	 * default camera settings
	 */
	CAMERA_DEFAULT_CFG,
	/* If selected, configure camera with
	 * camera settings defined by user
	 */
	CAMERA_USER_CFG
};

/**
 * @struct camera_fb_cfg
 * @brief Structure to configure camera
 * Parameter of camera_configure API.
 */
struct camera_fb_cfg {
	enum camera_cfg_mode cfg_mode;
	struct camera_fb_attr fb_attr;
};

/**
 * @struct camera_capability
 * @brief Structure to describe capabilities of camera
 */
struct camera_capability {
	/*Alignment size of point to frame buffer,
	 * depends on camera DMA
	 */
	u8_t fb_alignment;

	/*pixel format support by camera,
	 * partly depends on image sensor connected
	 */
	u32_t pixformat_support;

	/* Resolution support by camera,
	 * for most case, resolution support
	 * depends on image sensor connected.
	 */
	u16_t width_max;
	u16_t height_max;
};

/**
 * @struct camera_driver_api
 * @brief Structure to describe API for user to access camera
 */
struct camera_driver_api {
	int  (*camera_power_cb)(struct device *cam_dev,
		bool power);
	int  (*camera_reset_cb)(struct device *cam_dev);
	int  (*camera_get_cap_cb)(struct device *cam_dev,
		struct camera_capability *cap);
	int  (*camera_configure_cb)(struct device *cam_dev,
		struct camera_fb_cfg *fb_cfg);
	int  (*camera_start_cb)(struct device *cam_dev,
		enum camera_mode mode, void **bufs,
		u8_t buf_num, camera_capture_cb cb);
	int  (*camera_acquire_fb_cb)(struct device *cam_dev,
		void **fb, s32_t timeout);
	int  (*camera_release_fb_cb)(struct device *cam_dev,
		void *fb);
};

/**
 * @brief Power on/off camera
 *
 * @param cam_dev Pointer to camera device structure
 *
 * @param power true on power on else power off
 *
 * @retval 0 on success else negative errno code.
 */
__syscall int camera_power(struct device *cam_dev,
	bool power);

static inline int z_impl_camera_power(struct device *cam_dev,
	bool power)
{
	const struct camera_driver_api *api = cam_dev->driver_api;

	return api->camera_power_cb(cam_dev, power);
}

/**
 * @brief Power reset camera
 *
 * @param cam_dev Pointer to camera device structure
 *
 * @retval 0 on success else negative errno code.
 */
__syscall int camera_reset(struct device *cam_dev);

static inline int z_impl_camera_reset(struct device *cam_dev)
{
	const struct camera_driver_api *api = cam_dev->driver_api;

	return api->camera_reset_cb(cam_dev);
}

/**
 * @brief Get capability of camera
 *
 * @param cam_dev Pointer to camera device structure
 *
 * @param cap Pointer to camera capability structure
 * filled by camera driver
 *
 * @retval 0 on success else negative errno code.
 */
__syscall int camera_get_cap(struct device *cam_dev,
	struct camera_capability *cap);

static inline int z_impl_camera_get_cap(struct device *cam_dev,
	struct camera_capability *cap)
{
	const struct camera_driver_api *api = cam_dev->driver_api;

	return api->camera_get_cap_cb(cam_dev, cap);
}

/**
 * @brief Configure frame buffer of camera
 *
 * @param cam_dev Pointer to camera device structure
 *
 * @param fb_cfg Pointer to frame buffer configuration
 * structure of camera
 *
 * @retval 0 on success else negative errno code.
 */
__syscall int camera_configure(struct device *cam_dev,
	struct camera_fb_cfg *fb_cfg);

static inline int z_impl_camera_configure(struct device *cam_dev,
	struct camera_fb_cfg *fb_cfg)
{
	const struct camera_driver_api *api = cam_dev->driver_api;

	return api->camera_configure_cb(cam_dev, fb_cfg);
}

/**
 * @brief Start camera to capture frames to frame buffer(s)
 *
 * @param cam_dev Pointer to camera device structure
 *
 * @param mode Select camera to work on capture mode
 * or preview mode
 *
 * @param bufs Array of buffers for camera to store frames
 *
 * @param buf_num Number of elements of Array of buffers
 * which at lease be one
 *
 * @param cb Callback function when one frame is received
 * which could be NULL
 *
 * @retval 0 on success else negative errno code.
 */
__syscall int camera_start(struct device *cam_dev,
		enum camera_mode mode, void **bufs, u8_t buf_num,
		camera_capture_cb cb);

static inline int z_impl_camera_start(struct device *cam_dev,
		enum camera_mode mode, void **bufs, u8_t buf_num,
		camera_capture_cb cb)
{
	const struct camera_driver_api *api = cam_dev->driver_api;

	return api->camera_start_cb(cam_dev, mode, bufs, buf_num, cb);
}

/**
 * @brief Acquire one frame from camera
 *
 * @param cam_dev Pointer to camera device structure
 *
 * @param fb Pointer to the address of pointer to frame acquired
 *
 * @param timeout Most ticks to wait for one frame acquired
 *
 * @retval 0 on frame acquired else negative errno code with
 * no frame acquired.
 */
__syscall int camera_acquire_fb(struct device *cam_dev,
		void **fb, s32_t timeout);

static inline int z_impl_camera_acquire_fb(
	struct device *cam_dev, void **fb, s32_t timeout)
{
	struct camera_driver_api *api =
		(struct camera_driver_api *)cam_dev->driver_api;

	return api->camera_acquire_fb_cb(cam_dev, fb, timeout);
}

/**
 * @brief Release one buffer for camera to store frame to receive
 *
 * @param cam_dev Pointer to camera device structure
 *
 * @param fb Pointer to the buffer to release
 *
 * @retval 0 on buffer released else negative errno code with
 * buffer release is rejected.
 */
__syscall int camera_release_fb(struct device *cam_dev,
		void *fb);

static inline int z_impl_camera_release_fb(
	struct device *cam_dev, void *fb)
{
	struct camera_driver_api *api =
		(struct camera_driver_api *)cam_dev->driver_api;

	return api->camera_release_fb_cb(cam_dev, fb);
}

/**
 * @brief Get primary camera device
 *
 * @retval pointer to primary camera device, NULL if not found.
 */
struct device *camera_get_primary(void);

/**
 * @brief Get secondary camera device
 *
 * @retval pointer to secondary camera device, NULL if not found.
 */
struct device *camera_get_secondary(void);

#include <syscalls/camera.h>

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CAMERA_H_*/
