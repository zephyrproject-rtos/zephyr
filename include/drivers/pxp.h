/*
 * Copyright 2017-2020 NXP
 * All right reserved.
 *
 * SPDX-License-Identifier: apache-2.0
 */

/**
 * @file
 *
 * @brief Dedicated APIs for the PXP drivers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PXP_H_
#define ZEPHYR_INCLUDE_DRIVERS_PXP_H_

#include <zephyr/types.h>
#include <device.h>

/**
 * @brief PXP Interface
 * @defgroup pxp_interface PXP Interface
 * @{
 */
#include "fsl_pxp.h"
typedef int (*pxp_api_start)(const struct device *dev);
typedef int (*pxp_api_set_process_blocksize)(const struct device *dev,
								pxp_block_size_t size);
typedef int (*pxp_api_set_as_buffer)(const struct device *dev,
							const pxp_as_buffer_config_t *config);
typedef int (*pxp_api_set_as_blend)(const struct device *dev,
							const pxp_as_blend_config_t *config);
typedef int (*pxp_api_set_as_overlay_color)(const struct device *dev,
										uint32_t colorLow,
										uint32_t colorHigh);
typedef int (*pxp_api_enable_as_overlay_color)(const struct device *dev,
										bool enable);
typedef int (*pxp_api_set_as_position)(const struct device *dev,
									uint16_t upperLeftX,
									uint16_t upperLeftY,
									uint16_t lowerRightX,
									uint16_t lowerRightY);
typedef int (*pxp_api_set_ps_bg_color)(const struct device *dev,
									uint32_t bgColor);
typedef int (*pxp_api_set_ps_buffer)(const struct device *dev,
							const pxp_ps_buffer_config_t *config);
typedef int (*pxp_api_set_ps_scaler)(const struct device *dev,
								uint16_t inputWidth,
								uint16_t inputHeight,
								uint16_t outputWidth,
								uint16_t outputHeight);
typedef int (*pxp_api_set_ps_position)(const struct device *dev,
									uint16_t upperLeftX,
									uint16_t upperLeftY,
									uint16_t lowerRightX,
									uint16_t lowerRightY);
typedef int (*pxp_api_set_ps_color)(const struct device *dev,
										uint32_t colorLow,
										uint32_t colorHigh);
typedef int (*pxp_api_set_ps_YUVFormat)(const struct device *dev,
									pxp_ps_yuv_format_t format);
typedef int (*pxp_api_set_output_buffer)(const struct device *dev,
						const pxp_output_buffer_config_t *config);
typedef int (*pxp_api_set_overwritten_alpha_value)(const struct device *dev,
									uint8_t alpha);
typedef int (*pxp_api_enable_overwritten_alpha)(const struct device *dev,
										bool enable);
typedef int (*pxp_api_set_rotate)(const struct device *dev,
							   pxp_rotate_position_t position,
							   pxp_rotate_degree_t degree,
							   pxp_flip_mode_t flipMode);
typedef int (*pxp_api_set_csc1)(const struct device *dev,
							pxp_csc1_mode_t mode);
typedef int (*pxp_api_enable_csc1)(const struct device *dev,
								bool enable);
typedef int (*pxp_api_start_pic_copy)(const struct device *dev,
							const pxp_pic_copy_config_t *config);
typedef int (*pxp_api_start_mem_copy)(const struct device *dev,
									uint32_t srcAddr,
									uint32_t destAddr,
									uint32_t size);
typedef int (*pxp_api_wait_complete)(const struct device *dev);
typedef int (*pxp_api_stop)(const struct device *dev);

__subsystem struct pxp_driver_api {
	pxp_api_start start;
	pxp_api_set_process_blocksize set_process_blocksize;
	pxp_api_set_as_buffer set_as_buffer;
	pxp_api_set_as_blend set_as_blend;
	pxp_api_set_as_overlay_color set_as_overlay_color;
	pxp_api_enable_as_overlay_color enable_as_overlay_color;
	pxp_api_set_as_position set_as_position;
	pxp_api_set_ps_bg_color set_ps_bg_color;
	pxp_api_set_ps_buffer set_ps_buffer;
	pxp_api_set_ps_scaler set_ps_scaler;
	pxp_api_set_ps_position set_ps_position;
	pxp_api_set_ps_color set_ps_color;
	pxp_api_set_ps_YUVFormat set_ps_YUVFormat;
	pxp_api_set_output_buffer set_output_buffer;
	pxp_api_set_overwritten_alpha_value set_overwritten_alpha_value;
	pxp_api_enable_overwritten_alpha enable_overwritten_alpha;
	pxp_api_set_rotate set_rotate;
	pxp_api_set_csc1 set_csc1;
	pxp_api_enable_csc1 enable_csc1;
	pxp_api_start_pic_copy start_pic_copy;
	pxp_api_start_mem_copy start_mem_copy;
	pxp_api_wait_complete wait_complete;
	pxp_api_stop stop;
};

/**
 * @brief Start process
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_start(const struct device *dev);
static inline int z_impl_pxp_start(const struct device *dev)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->start == NULL) {
		return -ENOTSUP;
	}

	return api->start(dev);
}

/**
 * @brief Set the PXP processing block size
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param size The pixel block size.
 *
 * @retval 0 if successful
 * @retval negative on error.

 */

__syscall int pxp_set_process_blocksize(const struct device *dev,
									pxp_block_size_t size);
static inline int z_impl_pxp_set_process_blocksize(const struct device *dev,
									pxp_block_size_t size)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_process_blocksize == NULL) {
		return -ENOTSUP;
	}

	return api->set_process_blocksize(dev, size);
}

/**
 * @brief Set the alpha surface input buffer configuration
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to the configuration.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_as_buffer(const struct device *dev,
							const pxp_as_buffer_config_t *config);
static inline int z_impl_pxp_set_as_buffer(const struct device *dev,
							const pxp_as_buffer_config_t *config)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_as_buffer == NULL) {
		return -ENOTSUP;
	}

	return api->set_as_buffer(dev, config);
}

/**
 * @brief Set the alpha surface blending configuration
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to the configuration structure.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_as_blend(const struct device *dev,
							const pxp_as_blend_config_t *config);
static inline int z_impl_pxp_set_as_blend(const struct device *dev,
							const pxp_as_blend_config_t *config)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_as_blend == NULL) {
		return -ENOTSUP;
	}

	return api->set_as_blend(dev, config);
}

/**
 * @brief Set the alpha surface overlay color key.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param colorLow Color key low range .
 * @param colorHigh Color key high range.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_as_overlay_color(const struct device *dev,
										uint32_t colorLow,
										uint32_t colorHigh);
static inline int z_impl_pxp_set_as_overlay_color(const struct device *dev,
										uint32_t colorLow,
										uint32_t colorHigh)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_as_overlay_color == NULL) {
		return -ENOTSUP;
	}

	return api->set_as_overlay_color(dev, colorLow, colorHigh);
}

/**
 * @brief Enable or disable the alpha surface color key.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param enable True to enable, false to disable.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_enable_as_overlay_color(const struct device *dev,
										bool enable);
static inline int z_impl_pxp_enable_as_overlay_color(const struct device *dev,
										bool enable)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->enable_as_overlay_color == NULL) {
		return -ENOTSUP;
	}

	return api->enable_as_overlay_color(dev, enable);
}

/**
 * @brief Set the alpha surface position in output buffer.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param upperLeftX X of the upper left corner.
 * @param upperLeftY Y of the upper left corner.
 * @param lowerRightX X of the lower right corner.
 * @param lowerRightY Y of the lower right corner.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_as_position(const struct device *dev,
									uint16_t upperLeftX,
									uint16_t upperLeftY,
									uint16_t lowerRightX,
									uint16_t lowerRightY);
static inline int z_impl_pxp_set_as_position(const struct device *dev,
									uint16_t upperLeftX,
									uint16_t upperLeftY,
									uint16_t lowerRightX,
									uint16_t lowerRightY)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_as_position == NULL) {
		return -ENOTSUP;
	}

	return api->set_as_position(dev,
								upperLeftX, upperLeftY,
								lowerRightX, lowerRightY);
}

/**
 * @brief Set the back ground color of PS
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param bgColor Pixel value of the background color.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_ps_bg_color(const struct device *dev,
									uint32_t bgColor);
static inline int z_impl_pxp_set_ps_bg_color(const struct device *dev,
									uint32_t bgColor)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_ps_bg_color == NULL) {
		return -ENOTSUP;
	}

	return api->set_ps_bg_color(dev, bgColor);
}

/**
 * @brief Set the process surface input buffer configuration.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to the configuration.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_ps_buffer(const struct device *dev,
							const pxp_ps_buffer_config_t *config);
static inline int z_impl_pxp_set_ps_buffer(const struct device *dev,
							const pxp_ps_buffer_config_t *config)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_ps_buffer == NULL) {
		return -ENOTSUP;
	}

	return api->set_ps_buffer(dev, config);
}

/**
 * @brief Set the process surface scaler configuration.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param inputWidth Input image width.
 * @param inputHeight Input image height.
 * @param outputWidth Output image width.
 * @param outputHeight Output image height.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_ps_scaler(const struct device *dev,
								uint16_t inputWidth,
								uint16_t inputHeight,
								uint16_t outputWidth,
								uint16_t outputHeight);
static inline int z_impl_pxp_set_ps_scaler(const struct device *dev,
								uint16_t inputWidth,
								uint16_t inputHeight,
								uint16_t outputWidth,
								uint16_t outputHeight)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_ps_scaler == NULL) {
		return -ENOTSUP;
	}

	return api->set_ps_scaler(dev, inputWidth, inputHeight,
								outputWidth, outputHeight);
}

/**
 * @brief Set the process surface position in output buffer.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param upperLeftX X of the upper left corner.
 * @param upperLeftY Y of the upper left corner.
 * @param lowerRightX X of the lower right corner.
 * @param lowerRightY Y of the lower right corner.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_ps_position(const struct device *dev,
									uint16_t upperLeftX,
									uint16_t upperLeftY,
									uint16_t lowerRightX,
									uint16_t lowerRightY);
static inline int z_impl_pxp_set_ps_position(const struct device *dev,
									uint16_t upperLeftX,
									uint16_t upperLeftY,
									uint16_t lowerRightX,
									uint16_t lowerRightY)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_ps_position == NULL) {
		return -ENOTSUP;
	}

	return api->set_ps_position(dev,
								upperLeftX, upperLeftY,
								lowerRightX, lowerRightY);
}

/**
 * @brief Set the process surface color key.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param colorLow Color key low range.
 * @param colorHigh Color key high range.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_ps_color(const struct device *dev,
										uint32_t colorLow,
										uint32_t colorHigh);
static inline int z_impl_pxp_set_ps_color(const struct device *dev,
										uint32_t colorLow,
										uint32_t colorHigh)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_ps_color == NULL) {
		return -ENOTSUP;
	}

	return api->set_ps_color(dev, colorLow, colorHigh);
}

/**
 * @brief Set the process surface input pixel format YUV or YCbCr.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param format The YUV format.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_ps_YUVFormat(const struct device *dev,
									pxp_ps_yuv_format_t format);
static inline int z_impl_pxp_set_ps_YUVFormat(const struct device *dev,
									pxp_ps_yuv_format_t format)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_ps_YUVFormat == NULL) {
		return -ENOTSUP;
	}

	return api->set_ps_YUVFormat(dev, format);
}

/**
 * @brief Set the PXP output buffer configuration.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to the configuration.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_output_buffer(const struct device *dev,
						const pxp_output_buffer_config_t *config);
static inline int z_impl_pxp_set_output_buffer(const struct device *dev,
						const pxp_output_buffer_config_t *config)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_output_buffer == NULL) {
		return -ENOTSUP;
	}

	return api->set_output_buffer(dev, config);
}

/**
 * @brief Set the global overwritten alpha value.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param alpha The alpha value.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_overwritten_alpha_value(const struct device *dev,
										uint8_t alpha);
static inline int z_impl_pxp_set_overwritten_alpha_value(const struct device *dev,
										uint8_t alpha)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_overwritten_alpha_value == NULL) {
		return -ENOTSUP;
	}

	return api->set_overwritten_alpha_value(dev, alpha);
}

/**
 * @brief Enable or disable the global overwritten alpha value.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param enable True to enable, false to disable.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_enable_overwritten_alpha(const struct device *dev,
										bool enable);
static inline int z_impl_pxp_enable_overwritten_alpha(const struct device *dev,
										bool enable)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->enable_overwritten_alpha == NULL) {
		return -ENOTSUP;
	}

	return api->enable_overwritten_alpha(dev, enable);
}

/**
 * @brief Set the rotation configuration.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param position Rotate process surface or output buffer.
 * @param degree Rotate degree.
 * @param flipMode Flip mode.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_rotate(const struct device *dev,
							   pxp_rotate_position_t position,
							   pxp_rotate_degree_t degree,
							   pxp_flip_mode_t flipMode);
static inline int z_impl_pxp_set_rotate(const struct device *dev,
							   pxp_rotate_position_t position,
							   pxp_rotate_degree_t degree,
							   pxp_flip_mode_t flipMode)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_rotate == NULL) {
		return -ENOTSUP;
	}

	return api->set_rotate(dev, position, degree, flipMode);
}

/* omit the command queue */

/* omit the csc2 config */

/**
 * @brief Set the CSC1 mode.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode The conversion mode.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_set_csc1(const struct device *dev,
							pxp_csc1_mode_t mode);
static inline int z_impl_pxp_set_csc1(const struct device *dev,
							pxp_csc1_mode_t mode)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->set_csc1 == NULL) {
		return -ENOTSUP;
	}

	return api->set_csc1(dev, mode);
}

/**
 * @brief Enable or disable the CSC1.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param enable True to enable, false to disable.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_enable_csc1(const struct device *dev,
								bool enable);
static inline int z_impl_pxp_enable_csc1(const struct device *dev,
								bool enable)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->enable_csc1 == NULL) {
		return -ENOTSUP;
	}

	return api->enable_csc1(dev, enable);
}

/* omit pxp lut */

/* omit pxp dither */

/* omit pxp porter */

/**
 * @brief Copy picture from one buffer to another buffer.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to the picture copy configuration structure.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_start_pic_copy(const struct device *dev,
							const pxp_pic_copy_config_t *config);
static inline int z_impl_pxp_start_pic_copy(const struct device *dev,
							const pxp_pic_copy_config_t *config)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->start_pic_copy == NULL) {
		return -ENOTSUP;
	}

	return api->start_pic_copy(dev, config);
}

/**
 * @brief Copy continuous memory, the copy size should be 512 byte aligned.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param srcAddr Source memory address.
 * @param destAddr Destination memory address.
 * @param size How many bytes to copy, should be 512 byte aligned.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_start_mem_copy(const struct device *dev,
									uint32_t srcAddr,
									uint32_t destAddr,
									uint32_t size);
static inline int z_impl_pxp_start_mem_copy(const struct device *dev,
									uint32_t srcAddr,
									uint32_t destAddr,
									uint32_t size)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->start_mem_copy == NULL) {
		return -ENOTSUP;
	}

	return api->start_mem_copy(dev, srcAddr, destAddr, size);
}

/**
 * @brief Wait until the processing done.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_wait_complete(const struct device *dev);
static inline int z_impl_pxp_wait_complete(const struct device *dev)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->wait_complete == NULL) {
		return -ENOTSUP;
	}

	return api->wait_complete(dev);
}

/**
 * @brief Stop the PXP.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful
 * @retval negative on error.
 */

__syscall int pxp_stop(const struct device *dev);
static inline int z_impl_pxp_stop(const struct device *dev)
{
	const struct pxp_driver_api *api =
		(const struct pxp_driver_api *)dev->api;

	if (api->stop == NULL) {
		return -ENOTSUP;
	}

	return api->stop(dev);
}

/**
 * @}
 */

#include <syscalls/pxp.h>
#endif/* ZEPHYR_INCLUDE_DRIVERS_PXP_H_ */
