/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *				 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FPGA_H_
#define ZEPHYR_INCLUDE_DRIVERS_FPGA_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/crypto/cipher.h>

#ifdef __cplusplus
extern "C" {
#endif

enum FPGA_status {
	/* Inactive is when the FPGA cannot accept the bitstream
	 * and will not be programmed correctly
	 */
	FPGA_STATUS_INACTIVE,
	/* Active is when the FPGA can accept the bitstream and
	 * can be programmed correctly
	 */
	FPGA_STATUS_ACTIVE
};

enum FPGA_TRANSFER_TYPE {
	FPGA_TRANSFER_TYPE_TX = 0,
	FPGA_TRANSFER_TYPE_RX = 1,
	FPGA_TRANSFER_TYPE_UNDEFINED
};

struct fpga_transfer_param {
  enum FPGA_TRANSFER_TYPE FPGA_Transfer_Type;
  uint32_t FCB_Bitstream_Size;       // bytes
  uint16_t FCB_Transfer_Block_Size;  // bytes
};

struct fpga_ctx;

typedef int (*bitstream_load_hndlr)(struct fpga_ctx *ctx); // Custom bitstream load function

typedef enum FPGA_status (*fpga_api_get_status)(const struct device *dev);
typedef int (*fpga_api_session_start)(const struct device *dev, struct fpga_ctx *ctx);
typedef int (*fpga_api_session_free)(const struct device *dev);				 
typedef int (*fpga_api_load)(const struct device *dev, uint32_t *image_ptr,
			     uint32_t img_size);
typedef int (*fpga_api_reset)(const struct device *dev);
typedef int (*fpga_api_on)(const struct device *dev);
typedef int (*fpga_api_off)(const struct device *dev);
typedef const char *(*fpga_api_get_info)(const struct device *dev);

__subsystem struct fpga_driver_api {
	fpga_api_get_status get_status;
	fpga_api_reset reset;
	fpga_api_load load;
	fpga_api_on on;
	fpga_api_off off;
	fpga_api_get_info get_info;
	fpga_api_session_start session_start;
	fpga_api_session_free session_free;
};

struct fpga_ctx {
	// In case a custom bitstream load function is to be executed
	// by the application. The register configurations should still be 
	// handled by the fpga_load API.
	bitstream_load_hndlr bitstr_load_hndlr;	
	// The device driver instance this context relates to. Will be
	// populated by the session_start() API.
	const struct device *device;
	// User Data to be Preprocessed at the time of session begin.
	// This can be a metadata or anything else.
	void *meta_data;
	// The length of user data
	size_t meta_data_len;
	// whether meta_data is per block or the entire bitstream
	bool meta_data_per_block;
	// dest_addr to be filled in by driver if bitstream is to be written otherwise applicatoin
	uint8_t *dest_addr;
	// src_addr to be filled in by driver if bitstream is to be read otherwise applicatoin
	uint8_t *src_addr;
};

/**
 * @brief Read the status of FPGA.
 *
 * @param dev FPGA device structure.
 *
 * @retval 0 if the FPGA is in INACTIVE state.
 * @retval 1 if the FPGA is in ACTIVE state.
 */
static inline enum FPGA_status fpga_get_status(const struct device *dev)
{
	const struct fpga_driver_api *api =
		(const struct fpga_driver_api *)dev->api;

	return api->get_status(dev);
}

/**
 * @brief Reset the FPGA.
 *
 * @param dev FPGA device structure.
 *
 * @retval 0 if successful.
 * @retval Failed Otherwise.
 */
static inline int fpga_reset(const struct device *dev)
{
	const struct fpga_driver_api *api =
		(const struct fpga_driver_api *)dev->api;

	return api->reset(dev);
}

/**
 * @brief Load the bitstream and program the FPGA
 *
 * @param dev FPGA device structure.
 * @param image_ptr Pointer to bitstream.
 * @param img_size Bitstream size in bytes.
 *
 * @retval 0 if successful.
 * @retval Failed Otherwise.
 */
static inline int fpga_load(const struct device *dev, uint32_t *image_ptr,
			    uint32_t img_size)
{
	const struct fpga_driver_api *api =
		(const struct fpga_driver_api *)dev->api;

	return api->load(dev, image_ptr, img_size);
}

/**
 * @brief Turns on the FPGA.
 *
 * @param dev FPGA device structure.
 *
 * @retval 0 if successful.
 * @retval negative errno code on failure.
 */
static inline int fpga_on(const struct device *dev)
{
	const struct fpga_driver_api *api =
		(const struct fpga_driver_api *)dev->api;

	if (api->on == NULL) {
		return -ENOTSUP;
	}

	return api->on(dev);
}

/**
 * @brief Returns information about the FPGA.
 *
 * @param dev FPGA device structure.
 *
 * @return String containing information.
 */
static inline const char *fpga_get_info(const struct device *dev)
{
	const struct fpga_driver_api *api =
		(const struct fpga_driver_api *)dev->api;

	return api->get_info(dev);
}

/**
 * @brief Turns off the FPGA.
 *
 * @param dev FPGA device structure.
 *
 * @retval 0 if successful.
 * @retval negative errno code on failure.
 */
static inline int fpga_off(const struct device *dev)
{
	const struct fpga_driver_api *api =
		(const struct fpga_driver_api *)dev->api;

	if (api->off == NULL) {
		return -ENOTSUP;
	}

	return api->off(dev);
}

/**
 * @brief Sets up the session to load the bistream.
 * 		  The setup can include fpga configuration
 * 		  block settings passed in the form of 
 * 		  user configuration data.
 *
 * @param dev FPGA device structure.
 * @param user_config the user configuration data specific to the driver
 *
 * @retval 0 if successful.
 * @retval negative errno code on failure.
 */
static inline int fpga_session_start(const struct device *dev, struct fpga_ctx *ctx)
{
	const struct fpga_driver_api *api =
		(const struct fpga_driver_api *)dev->api;

	if (api->session_start == NULL) {
		return -ENOTSUP;
	}

	return api->session_start(dev, ctx); 
}

/**
 * @brief Frees up the session.
 *
 * @param dev FPGA device structure.
 *
 * @retval 0 if successful.
 * @retval negative errno code on failure.
 */
static inline int fpga_session_free(const struct device *dev)
{
	const struct fpga_driver_api *api =
		(const struct fpga_driver_api *)dev->api;

	if (api->session_free == NULL) {
		return -ENOTSUP;
	}

	return api->session_free(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_FPGA_H_ */