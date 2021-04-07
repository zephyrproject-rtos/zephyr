/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EDAC API header file
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EDAC_H_
#define ZEPHYR_INCLUDE_DRIVERS_EDAC_H_

#include <sys/types.h>

typedef void (*edac_notify_callback_f)(const struct device *dev, void *data);

/**
 * @defgroup edac EDAC API
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief EDAC error type
 */
enum edac_error_type {
	/** Correctable error type */
	EDAC_ERROR_TYPE_DRAM_COR = BIT(0),
	/** Uncorrectable error type */
	EDAC_ERROR_TYPE_DRAM_UC = BIT(1)
};

/**
 * @brief EDAC driver API
 *
 * This is the mandatory API any EDAC driver needs to expose.
 */
__subsystem struct edac_driver_api {
	/* Error Injection API is disabled by default */
	int (*inject_set_param1)(const struct device *dev, uint64_t value);
	int (*inject_get_param1)(const struct device *dev, uint64_t *value);
	int (*inject_set_param2)(const struct device *dev, uint64_t value);
	int (*inject_get_param2)(const struct device *dev, uint64_t *value);
	int (*inject_set_error_type)(const struct device *dev, uint32_t value);
	int (*inject_get_error_type)(const struct device *dev, uint32_t *value);
	int (*inject_error_trigger)(const struct device *dev);

	/* Error Logging  API */
	int (*ecc_error_log_get)(const struct device *dev, uint64_t *value);
	void (*ecc_error_log_clear)(const struct device *dev);
	int (*parity_error_log_get)(const struct device *dev, uint64_t *value);
	void (*parity_error_log_clear)(const struct device *dev);

	/* Error stats API */
	unsigned int (*errors_cor_get)(const struct device *dev);
	unsigned int (*errors_uc_get)(const struct device *dev);

	/* Notification callback API */
	int (*notify_cb_set)(const struct device *dev,
			     edac_notify_callback_f cb);
};

/**
 * @brief Set injection parameter param1
 *
 * Set first error injection parameter value.
 *
 * @param dev Pointer to the device structure
 * @param value First injection parameter
 *
 * @return -ENOTSUP if not supported
 * @return 0 on success, other error code otherwise
 */
static inline int edac_inject_set_param1(const struct device *dev,
					 uint64_t value)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	if (!api->inject_set_param1) {
		return -ENOTSUP;
	}

	return api->inject_set_param1(dev, value);
}

/**
 * @brief Get injection parameter param1
 *
 * Get first error injection parameter value.
 *
 * @param dev Pointer to the device structure
 * @param value Pointer to the first injection parameter
 *
 * @return -ENOTSUP if not supported
 * @return 0 on success, error code otherwise
 */
static inline int edac_inject_get_param1(const struct device *dev,
					 uint64_t *value)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	if (!api->inject_get_param1) {
		return -ENOTSUP;
	}

	return api->inject_get_param1(dev, value);

}

/**
 * @brief Set injection parameter param2
 *
 * Set second error injection parameter value.
 *
 * @param dev Pointer to the device structure
 * @param value Second injection parameter
 *
 * @return -ENOTSUP if not supported
 * @return 0 on success, error code otherwise
 */
static inline int edac_inject_set_param2(const struct device *dev,
					 uint64_t value)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	if (!api->inject_set_param2) {
		return -ENOTSUP;
	}

	return api->inject_set_param2(dev, value);
}

/**
 * @brief Get injection parameter param2
 *
 * @param dev Pointer to the device structure
 * @param value Pointer to the second injection parameter
 *
 * @return -ENOTSUP if not supported
 * @return 0 on success, error code otherwise
 */
static inline uint64_t edac_inject_get_param2(const struct device *dev,
					      uint64_t *value)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	if (!api->inject_get_param2) {
		return -ENOTSUP;
	}

	return api->inject_get_param2(dev, value);
}

/**
 * @brief Set error type value
 *
 * Set the value of error type to be injected
 *
 * @param dev Pointer to the device structure
 * @param error_type Error type value
 *
 * @return -ENOTSUP if not supported
 * @return 0 on success, error code otherwise
 */
static inline int edac_inject_set_error_type(const struct device *dev,
					     uint32_t error_type)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	if (!api->inject_set_error_type) {
		return -ENOTSUP;
	}

	return api->inject_set_error_type(dev, error_type);
}

/**
 * @brief Get error type value
 *
 * Get the value of error type to be injected
 *
 * @param dev Pointer to the device structure
 * @param error_type Pointer to error type value
 *
 * @return -ENOTSUP if not supported
 * @return 0 on success, error code otherwise
 */
static inline uint32_t edac_inject_get_error_type(const struct device *dev,
						  uint32_t *error_type)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	if (!api->inject_get_error_type) {
		return -ENOTSUP;
	}

	return api->inject_get_error_type(dev, error_type);
}

/**
 * @brief Set injection control
 *
 * Trigger error injection.
 *
 * @param dev Pointer to the device structure
 *
 * @return -ENOTSUP if not supported
 * @return 0 on success, error code otherwise
 */
static inline int edac_inject_error_trigger(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	if (!api->inject_error_trigger) {
		return -ENOTSUP;
	}

	return api->inject_error_trigger(dev);
}

/**
 * @brief Get ECC Error Log
 *
 * Read value of ECC Error Log.
 *
 * @param dev Pointer to the device structure
 * @param value Pointer to the ECC Error Log value
 *
 * @return 0 on success, error code otherwise
 */
static inline int edac_ecc_error_log_get(const struct device *dev,
					 uint64_t *value)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	__ASSERT(api->ecc_error_log_get,
		 "ecc_error_log_get must be implemented by driver");

	return api->ecc_error_log_get(dev, value);
}

/**
 * @brief Clear ECC Error Log
 *
 * Clear value of ECC Error Log.
 *
 * @param dev Pointer to the device structure
 */
static inline void edac_ecc_error_log_clear(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	__ASSERT(api->ecc_error_log_clear,
		 "ecc_error_log_clear must be implemented by driver");

	api->ecc_error_log_clear(dev);
}

/**
 * @brief Get Parity Error Log
 *
 * Read value of Parity Error Log.
 *
 * @param dev Pointer to the device structure
 * @param value Pointer to the parity Error Log value
 *
 * @return 0 on success, error code otherwise
 */
static inline int edac_parity_error_log_get(const struct device *dev,
					    uint64_t *value)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	__ASSERT(api->parity_error_log_get,
		 "parity_error_log_get must be implemented by driver");

	return api->parity_error_log_get(dev, value);
}

/**
 * @brief Clear Parity Error Log
 *
 * Clear value of Parity Error Log.
 *
 * @param dev Pointer to the device structure
 */
static inline void edac_parity_error_log_clear(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	__ASSERT(api->parity_error_log_clear,
		 "parity_error_log_clear must be implemented by driver");

	api->parity_error_log_clear(dev);
}

/**
 * @brief Get number of correctable errors
 *
 * @param dev Pointer to the device structure
 * @return Number of correctable errors
 */
static inline unsigned int edac_errors_cor_get(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	__ASSERT(api->errors_cor_get,
		 "errors_cor_get must be implemented by driver");

	return api->errors_cor_get(dev);
}

/**
 * @brief Get number of uncorrectable errors
 *
 * @param dev Pointer to the device structure
 * @return Number of uncorrectable errors
 */
static inline unsigned int edac_errors_uc_get(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	__ASSERT(api->errors_uc_get,
		 "errors_uc_get must be implemented by driver");

	return api->errors_uc_get(dev);
}

/**
 * Register callback function for memory error exception
 *
 * This callback runs in interrupt context
 *
 * @param dev EDAC driver device to install callback
 * @param cb Callback function pointer
 *
 * @return 0 Success, nonzero if an error occurred
 */
static inline int edac_notify_callback_set(const struct device *dev,
					   edac_notify_callback_f cb)
{
	const struct edac_driver_api *api = dev->api;

	__ASSERT(api->notify_cb_set,
		 "notify_cb_set must be implemented by driver");

	return api->notify_cb_set(dev, cb);
}

/**
 * @}
 */

#endif  /* ZEPHYR_INCLUDE_DRIVERS_EDAC_H_ */
