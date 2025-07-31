/*
 * Copyright (c) 2025 Xtooltech
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public CRC driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CRC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CRC_H_

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CRC polynomial types
 */
enum crc_poly_type {
	CRC_POLY_TYPE_CUSTOM = 0,
	CRC_POLY_TYPE_CRC8,
	CRC_POLY_TYPE_CRC8_CCITT,
	CRC_POLY_TYPE_CRC16,
	CRC_POLY_TYPE_CRC16_CCITT,
	CRC_POLY_TYPE_CRC32,
	CRC_POLY_TYPE_CRC32C,
};

/**
 * @brief CRC configuration
 */
struct crc_config {
	enum crc_poly_type type;
	uint32_t poly;          /**< Custom polynomial (used when type is CUSTOM) */
	uint32_t seed;          /**< Initial seed value */
	bool reflect_input;     /**< Reflect input bytes */
	bool reflect_output;    /**< Reflect output CRC */
	bool complement_input;  /**< Complement input bytes */
	bool complement_output; /**< Complement output CRC */
};

/**
 * @typedef crc_api_configure_t
 * @brief Configure CRC engine
 */
typedef int (*crc_api_configure_t)(const struct device *dev,
				   const struct crc_config *config);

/**
 * @typedef crc_api_compute_t
 * @brief Compute CRC of data buffer
 */
typedef uint32_t (*crc_api_compute_t)(const struct device *dev,
				      const uint8_t *data,
				      size_t len);

/**
 * @typedef crc_api_append_t
 * @brief Append data to ongoing CRC calculation
 */
typedef int (*crc_api_append_t)(const struct device *dev,
				const uint8_t *data,
				size_t len);

/**
 * @typedef crc_api_get_result_t
 * @brief Get current CRC result
 */
typedef uint32_t (*crc_api_get_result_t)(const struct device *dev);

/**
 * @typedef crc_api_reset_t
 * @brief Reset CRC to configured seed value
 */
typedef void (*crc_api_reset_t)(const struct device *dev);

/**
 * @brief CRC driver API
 */
__subsystem struct crc_driver_api {
	crc_api_configure_t configure;
	crc_api_compute_t compute;
	crc_api_append_t append;
	crc_api_get_result_t get_result;
	crc_api_reset_t reset;
};

/**
 * @brief Configure CRC engine
 *
 * @param dev CRC device
 * @param config CRC configuration
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid configuration
 * @retval -ENOTSUP Configuration not supported
 */
__syscall int crc_configure(const struct device *dev,
			     const struct crc_config *config);

static inline int z_impl_crc_configure(const struct device *dev,
				       const struct crc_config *config)
{
	const struct crc_driver_api *api =
		(const struct crc_driver_api *)dev->api;

	return api->configure(dev, config);
}

/**
 * @brief Compute CRC of data buffer
 *
 * This function resets the CRC, processes the data, and returns the result.
 *
 * @param dev CRC device
 * @param data Data buffer
 * @param len Length of data
 *
 * @return CRC result
 */
__syscall uint32_t crc_compute(const struct device *dev,
			       const uint8_t *data,
			       size_t len);

static inline uint32_t z_impl_crc_compute(const struct device *dev,
					  const uint8_t *data,
					  size_t len)
{
	const struct crc_driver_api *api =
		(const struct crc_driver_api *)dev->api;

	return api->compute(dev, data, len);
}

/**
 * @brief Append data to ongoing CRC calculation
 *
 * @param dev CRC device
 * @param data Data buffer
 * @param len Length of data
 *
 * @retval 0 If successful
 * @retval -EINVAL Invalid parameters
 */
__syscall int crc_append(const struct device *dev,
			 const uint8_t *data,
			 size_t len);

static inline int z_impl_crc_append(const struct device *dev,
				    const uint8_t *data,
				    size_t len)
{
	const struct crc_driver_api *api =
		(const struct crc_driver_api *)dev->api;

	return api->append(dev, data, len);
}

/**
 * @brief Get current CRC result
 *
 * @param dev CRC device
 *
 * @return Current CRC value
 */
__syscall uint32_t crc_get_result(const struct device *dev);

static inline uint32_t z_impl_crc_get_result(const struct device *dev)
{
	const struct crc_driver_api *api =
		(const struct crc_driver_api *)dev->api;

	return api->get_result(dev);
}

/**
 * @brief Reset CRC to configured seed value
 *
 * @param dev CRC device
 */
__syscall void crc_reset(const struct device *dev);

static inline void z_impl_crc_reset(const struct device *dev)
{
	const struct crc_driver_api *api =
		(const struct crc_driver_api *)dev->api;

	api->reset(dev);
}

/**
 * @brief Get default CRC configuration
 *
 * @param type CRC polynomial type
 * @param config Configuration structure to fill
 *
 * @retval 0 If successful
 * @retval -ENOTSUP Type not supported
 */
int crc_get_default_config(enum crc_poly_type type,
			   struct crc_config *config);

#ifdef __cplusplus
}
#endif

#include <syscalls/crc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_CRC_H_ */