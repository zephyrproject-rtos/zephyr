/*
 * Copyright (c) 2024 Brill Power Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CRC_H
#define ZEPHYR_INCLUDE_DRIVERS_CRC_H

#include "autoconf.h"
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CRC driver APIs
 * @defgroup crc_interface CRC driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @struct crc_data
 * @brief Runtime driver configuration
 * @param mutex Zephyr mutex object to make driver thread-safe when driver
 *				is used within multiple threads
 */
struct crc_data {
#ifdef CONFIG_CRC_THREADSAFE
	struct k_mutex mutex;
	k_timeout_t crc_8_timeout;
	k_timeout_t crc_32_timeout;
#endif /* CONFIG_CRC_THREADSAFE* */
};

#ifdef CONFIG_CRC_HW

typedef int (*crc_api_setup_crc_8_ccitt)(const struct device *dev);
typedef uint8_t (*crc_api_crc_8_ccitt)(const struct device *dev, uint8_t initial_value,
				       const void *buf, size_t len);
typedef uint8_t (*crc_api_verify_crc_8_ccitt)(const struct device *dev, uint8_t expected,
					      uint8_t initial_value, const void *buf, size_t len);
typedef int (*crc_api_setup_crc_32_ieee)(const struct device *dev);
typedef uint32_t (*crc_api_crc_32_ieee)(const struct device *dev, const void *buf, size_t len);
typedef uint32_t (*crc_api_verify_crc_32_ieee)(const struct device *dev, uint32_t expected,
					       const void *buf, size_t len);

/**
 * @struct crc_driver_api
 * @brief Publicly available API accessible as part of device struct
 */
struct crc_driver_api {
	crc_api_setup_crc_8_ccitt setup_crc_8_ccitt;
	crc_api_crc_8_ccitt crc_8_ccitt;
	crc_api_verify_crc_8_ccitt verify_crc_8_ccitt;
	crc_api_setup_crc_32_ieee setup_crc_32_ieee;
	crc_api_crc_32_ieee crc_32_ieee;
	crc_api_verify_crc_32_ieee verify_crc_32_ieee;
};

#endif /* CONFIG_CRC_HW* */

#ifdef CONFIG_CRC_HW

#ifdef CONFIG_CRC_THREADSAFE

/**
 * @brief Attempt to acquire CRC mutex lock
 * @param timeout Time in milliseconds to wait, or K_NO_WAIT / K_FOREVER
 * @retval 0 Mutex successfully acquired
 * @retval -EBUSY Returned without waiting
 * @retval -EAGAIN Timeout after waiting for timeout period
 */
static inline int crc_lock(k_timeout_t timeout)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc1));
	struct crc_data *data = (struct crc_data *)dev->data;

	return k_mutex_lock(&data->mutex, timeout);
}

/**
 * @brief Unlock the CRC mutex
 * @retval 0 Mutex successfully unlocked
 * @retval -EPERM Current thread does not own mutex
 * @retval -EINVAL Mutex is not currently locked
 */
static inline int crc_unlock(void)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc1));
	struct crc_data *data = (struct crc_data *)dev->data;

	struct k_mutex crc_mtx = data->mutex;
	int res = 0;

	/**
	 * The lock count can be greater than 1 if the same thread acquires
	 * a lock multiple times, so we have to iterate until lock_count
	 * is zero
	 */
	while (crc_mtx.lock_count != 0) {
		res = k_mutex_unlock(&data->mutex);

		if (res != 0) {
			return res;
		}
	}

	return res;
}

#endif /* CONFIG_CRC_THREADSAFE* */

/**
 * @brief Configure CRC unit to calculate CRC 8 CCITT
 * @retval See crc_lock()
 */
static inline int setup_crc_8_ccitt(void)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc1));
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	/**
	 * Attempt to take mutex lock, if we fail then return crc_lock() retvals,
	 * else return the setup function which will return 0
	 */
#ifdef CONFIG_CRC_THREADSAFE
	const struct crc_data *data = dev->data;
	int mtx = crc_lock(data->crc_8_timeout);

	if (mtx != 0) {
		return mtx;
	}
#endif /* CONFIG_CRC_THREADSAFE* */

	return api->setup_crc_8_ccitt(dev);
}

/**
 * @brief Get 8-bit CRC CCITT on array *buf of length len
 * @param initial_value Initial value to be preloaded into CRC calculation unit
 * @param *buf Pointer to array to be CRCd
 * @param len Number of bytes in *buf
 * @retval Calculated 8-bit CRC CCITT
 */
static inline uint8_t crc_8_ccitt(uint8_t initial_value, const void *buf, size_t len)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc1));
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	/**
	 * We must check that the calling thread has access to the resource if in
	 * thread-safe mode
	 */
#ifdef CONFIG_CRC_THREADSAFE
	struct crc_data *data = (struct crc_data *)dev->data;
	int mtx = crc_lock(data->crc_8_timeout);

	if (mtx != 0) {
		return mtx;
	}
#endif /* CONFIG_CRC_THREADSAFE* */

	return api->crc_8_ccitt(dev, initial_value, buf, len);
}

/**
 * @brief Verify 8-bit CRC CCITT of array *buf of length len
 * @param expected Expected CRC result
 * @param initial_value Initial value to be preloaded into CRC calculation unit
 * @param *buf Pointer to array to be CRC'd
 * @param len Number of bytes in *buf
 * @retval Calculated 8-bit CRC CCITT
 */
static inline uint8_t verify_crc_8_ccitt(uint8_t expected, uint8_t initial_value, const void *buf,
					 size_t len)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc1));
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	return api->verify_crc_8_ccitt(dev, expected, initial_value, buf, len);
}

/**
 * @brief Configure CRC unit to calculate CRC 32 IEEE
 * @retval See crc_lock()
 */
static inline int setup_crc_32_ieee(void)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc1));
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	/**
	 * Attempt to take mutex lock, if we fail then return crc_lock() retvals,
	 * else return the setup function which will return 0
	 */
#ifdef CONFIG_CRC_THREADSAFE
	const struct crc_data *data = dev->data;
	int mtx = crc_lock(data->crc_32_timeout);

	if (mtx != 0) {
		return mtx;
	}
#endif /* CONFIG_CRC_THREADSAFE* */

	return api->setup_crc_32_ieee(dev);
}

/**
 * @brief Get 32-bit CRC on array *buf of length len
 * @param *buf Pointer to array to be CRCd
 * @param len Number of bytes in *buf
 * @retval Calculated 32-bit CRC
 */
static inline uint32_t crc_32_ieee(const void *buf, size_t len)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc1));
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	/**
	 * We must check that the calling thread has access to the resource if in
	 * thread-safe mode
	 */
#ifdef CONFIG_CRC_THREADSAFE
	struct crc_data *data = (struct crc_data *)dev->data;
	int mtx = crc_lock(data->crc_32_timeout);

	if (mtx != 0) {
		return mtx;
	}
#endif /* CONFIG_CRC_THREADSAFE* */

	return api->crc_32_ieee(dev, buf, len);
}

/**
 * @brief Verify 32-bit CRC of array *buf of length len
 * @param expected Expected CRC result
 * @param *buf Pointer to array to be CRC'd
 * @param len Number of bytes in *buf
 * @retval Calculated 32-bit CRC
 */
static inline uint32_t verify_crc_32_ieee(uint32_t expected, const void *buf, size_t len)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc1));
	const struct crc_driver_api *api = (const struct crc_driver_api *)dev->api;

	return api->verify_crc_32_ieee(dev, expected, buf, len);
}

#endif /* CONFIG_CRC_HW* */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CRC_H */

/**
 * @}
 */
