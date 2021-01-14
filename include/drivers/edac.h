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

typedef int (*edac_api_inject_set_param1_f)(const struct device *dev,
					uint64_t addr);
typedef uint64_t (*edac_api_inject_get_param1_f)(const struct device *dev);

typedef int (*edac_api_inject_set_param2_f)(const struct device *dev,
					     uint64_t mask);
typedef uint64_t (*edac_api_inject_get_param2_f)(const struct device *dev);

typedef int (*edac_api_inject_set_error_type_f)(const struct device *dev,
						uint32_t error_type);
typedef uint32_t (*edac_api_inject_get_error_type_f)(const struct device *dev);

typedef int (*edac_api_inject_error_trigger_f)(const struct device *dev);

typedef uint64_t (*edac_api_ecc_error_log_get_f)(const struct device *dev);
typedef void (*edac_api_ecc_error_log_clear_f)(const struct device *dev);

typedef uint64_t (*edac_api_parity_error_log_get_f)(const struct device *dev);
typedef void (*edac_api_parity_error_log_clear_f)(const struct device *dev);

typedef unsigned int (*edac_api_errors_cor_get_f)(const struct device *dev);
typedef unsigned int (*edac_api_errors_uc_get_f)(const struct device *dev);

typedef void (*edac_notify_callback_f)(const struct device *dev,
				       void *data);
typedef int (*edac_api_notify_cb_set_f)(const struct device *dev,
				      edac_notify_callback_f cb);

/**
 * @defgroup edac Zephyr API
 * @{
 */

/** @brief Correctable error type */
#define EDAC_ERROR_TYPE_DRAM_COR	BIT(0)
/** @brief Uncorrectable error type */
#define EDAC_ERROR_TYPE_DRAM_UC		BIT(1)

/**
 * @brief EDAC driver API
 *
 * This is the mandatory API any EDAC driver needs to expose.
 */
__subsystem struct edac_driver_api {
#if defined(CONFIG_EDAC_ERROR_INJECT)
	/* Error Injection API is disabled by default */
	edac_api_inject_set_param1_f inject_set_param1;
	edac_api_inject_get_param1_f inject_get_param1;
	edac_api_inject_set_param2_f inject_set_param2;
	edac_api_inject_get_param2_f inject_get_param2;
	edac_api_inject_get_error_type_f inject_get_error_type;
	edac_api_inject_set_error_type_f inject_set_error_type;
	edac_api_inject_error_trigger_f inject_error_trigger;
#endif /* CONFIG_EDAC_ERROR_INJECT */

	/* Error Logging  API */
	edac_api_ecc_error_log_get_f ecc_error_log_get;
	edac_api_ecc_error_log_clear_f ecc_error_log_clear;
	edac_api_parity_error_log_get_f parity_error_log_get;
	edac_api_parity_error_log_clear_f parity_error_log_clear;

	/* Error stats API */
	edac_api_errors_cor_get_f errors_cor_get;
	edac_api_errors_uc_get_f errors_uc_get;

	/* Notification callback API */
	edac_api_notify_cb_set_f notify_cb_set;
};

#if defined(CONFIG_EDAC_ERROR_INJECT)

/**
 * @brief Set injection parameter param1
 *
 * This parameter is used to set first error injection parameter value.
 *
 * @param dev Pointer to the device structure
 * @param addr Injection address base
 * @return 0 on success, error code otherwise
 */
static inline int edac_inject_set_param1(const struct device *dev,
					 uint64_t addr)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	return api->inject_set_param1(dev, addr);
}

/**
 * @brief Get injection parameter param1
 *
 * Get first error injection parameter value.
 *
 * @param dev Pointer to the device structure
 * @return Injection parameter param1
 */
static inline uint64_t edac_inject_get_param1(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	return api->inject_get_param1(dev);

}

/**
 * @brief Set injection parameter param2
 *
 * This parameter is used to set second error injection parameter value.
 *
 * @param dev Pointer to the device structure
 * @param addr Injection address mask
 * @return 0 on success, error code otherwise
 */
static inline int edac_inject_set_param2(const struct device *dev,
					 uint64_t mask)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	return api->inject_set_param2(dev, mask);
}

/**
 * @brief Get injection parameter param2
 *
 * @param dev Pointer to the device structure
 * @return Injection parameter param2
 */
static inline uint64_t edac_inject_get_param2(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	return api->inject_get_param2(dev);

}

/**
 * @brief Set error type value
 *
 * Set the value of error type to be injected
 *
 * @param dev Pointer to the device structure
 * @param error_type Error type value
 * @return 0 on success, error code otherwise
 */
static inline int edac_inject_set_error_type(const struct device *dev,
					     uint32_t error_type)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	return api->inject_set_error_type(dev, error_type);
}

/**
 * @brief Get error type value
 *
 * Get the value of error type to be injected
 *
 * @param dev Pointer to the device structure
 * @return error_type Error type value
 */
static inline uint32_t edac_inject_get_error_type(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	return api->inject_get_error_type(dev);
}

/**
 * @brief Set injection control
 *
 * Set injection control register to the value provided.
 *
 * @param dev Pointer to the device structure
 * @param addr Injection control value
 * @return 0 on success, error code otherwise
 */
static inline int edac_inject_error_trigger(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	return api->inject_error_trigger(dev);
}

#endif /* CONFIG_EDAC_ERROR_INJECT */

/**
 * @brief Get ECC Error Log
 *
 * Read value of ECC Error Log
 *
 * @param dev Pointer to the device structure
 * @return ECC Error Log value
 */
static inline uint64_t edac_ecc_error_log_get(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	return api->ecc_error_log_get(dev);
}

/**
 * @brief Clear ECC Error Log
 *
 * Clear value of ECC Error Log
 *
 * @param dev Pointer to the device structure
 */
static inline void edac_ecc_error_log_clear(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	api->ecc_error_log_clear(dev);
}

/**
 * @brief Get Parity Error Log
 *
 * Read value of Parity Error Log
 *
 * @param dev Pointer to the device structure
 * @return Parity Error Log value
 */
static inline uint64_t edac_parity_error_log_get(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

	return api->parity_error_log_get(dev);
}

/**
 * @brief Clear Parity Error Log
 *
 * Clear value of Parity Error Log
 *
 * @param dev Pointer to the device structure
 */
static inline void edac_parity_error_log_clear(const struct device *dev)
{
	const struct edac_driver_api *api =
		(const struct edac_driver_api *)dev->api;

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

	return api->errors_uc_get(dev);
}

/**
 * Register callback function for memory error exception
 *
 * This callback runs in interrupt context
 *
 * @param dev EDAC driver device to install callback
 * @param cb Callback function pointer
 * @return 0 Success, nonzero if an error occurred
 */
static inline int edac_notify_callback_set(const struct device *dev,
					   edac_notify_callback_f cb)
{
	const struct edac_driver_api *api = dev->api;

	return api->notify_cb_set(dev, cb);
}

/**
 * @}
 */

#endif  /* ZEPHYR_INCLUDE_DRIVERS_EDAC_H_ */
