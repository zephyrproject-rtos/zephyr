/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup edac_interface
 * @brief Main header file for EDAC (Error Detection and Correction) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EDAC_H_
#define ZEPHYR_INCLUDE_DRIVERS_EDAC_H_

#include <errno.h>

#include <sys/types.h>

/**
 * @brief Interfaces for Error Detection and Correction (EDAC) controllers.
 * @defgroup edac_interface EDAC
 * @since 2.5
 * @version 0.8.0
 * @ingroup io_interfaces
 * @{
 *
 * @defgroup edac_interface_ext Device-specific EDAC API extensions
 * @{
 * @}
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
 * @brief Notification callback function signature for memory error exceptions.
 *
 * @param dev Pointer to the device structure
 * @param data Pointer to the callback data
 */
typedef void (*edac_notify_callback_f)(const struct device *dev, void *data);

/**
 * @def_driverbackendgroup{EDAC,edac_interface}
 * @{
 */

/**
 * @brief Callback API to set injection parameter param1.
 * See edac_inject_set_param1() for argument description.
 */
typedef int (*edac_inject_set_param1_f)(const struct device *dev, uint64_t value);

/**
 * @brief Callback API to get injection parameter param1.
 * See edac_inject_get_param1() for argument description.
 */
typedef int (*edac_inject_get_param1_f)(const struct device *dev, uint64_t *value);

/**
 * @brief Callback API to set injection parameter param2.
 * See edac_inject_set_param2() for argument description.
 */
typedef int (*edac_inject_set_param2_f)(const struct device *dev, uint64_t value);

/**
 * @brief Callback API to get injection parameter param2.
 * See edac_inject_get_param2() for argument description.
 */
typedef int (*edac_inject_get_param2_f)(const struct device *dev, uint64_t *value);

/**
 * @brief Callback API to set the error type to be injected.
 * See edac_inject_set_error_type() for argument description.
 */
typedef int (*edac_inject_set_error_type_f)(const struct device *dev, uint32_t value);

/**
 * @brief Callback API to get the error type to be injected.
 * See edac_inject_get_error_type() for argument description.
 */
typedef int (*edac_inject_get_error_type_f)(const struct device *dev, uint32_t *value);

/**
 * @brief Callback API to trigger error injection.
 * See edac_inject_error_trigger() for argument description.
 */
typedef int (*edac_inject_error_trigger_f)(const struct device *dev);

/**
 * @brief Callback API to read the ECC Error Log.
 * See edac_ecc_error_log_get() for argument description.
 */
typedef int (*edac_ecc_error_log_get_f)(const struct device *dev, uint64_t *value);

/**
 * @brief Callback API to clear the ECC Error Log.
 * See edac_ecc_error_log_clear() for argument description.
 */
typedef int (*edac_ecc_error_log_clear_f)(const struct device *dev);

/**
 * @brief Callback API to read the Parity Error Log.
 * See edac_parity_error_log_get() for argument description.
 */
typedef int (*edac_parity_error_log_get_f)(const struct device *dev, uint64_t *value);

/**
 * @brief Callback API to clear the Parity Error Log.
 * See edac_parity_error_log_clear() for argument description.
 */
typedef int (*edac_parity_error_log_clear_f)(const struct device *dev);

/**
 * @brief Callback API to get the number of correctable errors.
 * See edac_errors_cor_get() for argument description.
 */
typedef int (*edac_errors_cor_get_f)(const struct device *dev);

/**
 * @brief Callback API to get the number of uncorrectable errors.
 * See edac_errors_uc_get() for argument description.
 */
typedef int (*edac_errors_uc_get_f)(const struct device *dev);

/**
 * @brief Callback API to register a notification callback for memory error exceptions.
 * See edac_notify_callback_set() for argument description.
 */
typedef int (*edac_notify_cb_set_f)(const struct device *dev,
				    edac_notify_callback_f cb);

/**
 * @driver_ops{EDAC}
 */
__subsystem struct edac_driver_api {
	/* Error Injection API is disabled by default */
	/** @driver_ops_optional @copybrief edac_inject_set_param1 */
	edac_inject_set_param1_f inject_set_param1;
	/** @driver_ops_optional @copybrief edac_inject_get_param1 */
	edac_inject_get_param1_f inject_get_param1;
	/** @driver_ops_optional @copybrief edac_inject_set_param2 */
	edac_inject_set_param2_f inject_set_param2;
	/** @driver_ops_optional @copybrief edac_inject_get_param2 */
	edac_inject_get_param2_f inject_get_param2;
	/** @driver_ops_optional @copybrief edac_inject_set_error_type */
	edac_inject_set_error_type_f inject_set_error_type;
	/** @driver_ops_optional @copybrief edac_inject_get_error_type */
	edac_inject_get_error_type_f inject_get_error_type;
	/** @driver_ops_optional @copybrief edac_inject_error_trigger */
	edac_inject_error_trigger_f inject_error_trigger;

	/* Error Logging  API */
	/** @driver_ops_optional @copybrief edac_ecc_error_log_get */
	edac_ecc_error_log_get_f ecc_error_log_get;
	/** @driver_ops_optional @copybrief edac_ecc_error_log_clear */
	edac_ecc_error_log_clear_f ecc_error_log_clear;
	/** @driver_ops_optional @copybrief edac_parity_error_log_get */
	edac_parity_error_log_get_f parity_error_log_get;
	/** @driver_ops_optional @copybrief edac_parity_error_log_clear */
	edac_parity_error_log_clear_f parity_error_log_clear;

	/* Error stats API */
	/** @driver_ops_optional @copybrief edac_errors_cor_get */
	edac_errors_cor_get_f errors_cor_get;
	/** @driver_ops_optional @copybrief edac_errors_uc_get */
	edac_errors_uc_get_f errors_uc_get;

	/* Notification callback API */
	/** @driver_ops_optional @copybrief edac_notify_callback_set */
	edac_notify_cb_set_f notify_cb_set;
};

/** @} */

/**
 * @name Optional interfaces
 * @{
 *
 * EDAC Optional Interfaces
 */

/**
 * @brief Set injection parameter param1
 *
 * Set first error injection parameter value.
 *
 * @param dev Pointer to the device structure
 * @param value First injection parameter
 *
 * @retval -ENOSYS if the optional interface is not implemented
 * @retval 0 on success, other error code otherwise
 */
static inline int edac_inject_set_param1(const struct device *dev,
					 uint64_t value)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->inject_set_param1 == NULL) {
		return -ENOSYS;
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
 * @retval -ENOSYS if the optional interface is not implemented
 * @retval 0 on success, error code otherwise
 */
static inline int edac_inject_get_param1(const struct device *dev,
					 uint64_t *value)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->inject_get_param1 == NULL) {
		return -ENOSYS;
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
 * @retval -ENOSYS if the optional interface is not implemented
 * @retval 0 on success, error code otherwise
 */
static inline int edac_inject_set_param2(const struct device *dev,
					 uint64_t value)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->inject_set_param2 == NULL) {
		return -ENOSYS;
	}

	return api->inject_set_param2(dev, value);
}

/**
 * @brief Get injection parameter param2
 *
 * @param dev Pointer to the device structure
 * @param value Pointer to the second injection parameter
 *
 * @retval -ENOSYS if the optional interface is not implemented
 * @retval 0 on success, error code otherwise
 */
static inline int edac_inject_get_param2(const struct device *dev,
					 uint64_t *value)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->inject_get_param2 == NULL) {
		return -ENOSYS;
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
 * @retval -ENOSYS if the optional interface is not implemented
 * @retval 0 on success, error code otherwise
 */
static inline int edac_inject_set_error_type(const struct device *dev,
					     uint32_t error_type)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->inject_set_error_type == NULL) {
		return -ENOSYS;
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
 * @retval -ENOSYS if the optional interface is not implemented
 * @retval 0 on success, error code otherwise
 */
static inline int edac_inject_get_error_type(const struct device *dev,
					     uint32_t *error_type)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->inject_get_error_type == NULL) {
		return -ENOSYS;
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
 * @retval -ENOSYS if the optional interface is not implemented
 * @retval 0 on success, error code otherwise
 */
static inline int edac_inject_error_trigger(const struct device *dev)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->inject_error_trigger == NULL) {
		return -ENOSYS;
	}

	return api->inject_error_trigger(dev);
}

/** @} */ /* End of EDAC Optional Interfaces */

/**
 * @name Mandatory interfaces
 * @{
 *
 * EDAC Mandatory Interfaces
 */

/**
 * @brief Get ECC Error Log
 *
 * Read value of ECC Error Log.
 *
 * @param dev Pointer to the device structure
 * @param value Pointer to the ECC Error Log value
 *
 * @retval 0 on success, error code otherwise
 * @retval -ENOSYS if the mandatory interface is not implemented
 */
static inline int edac_ecc_error_log_get(const struct device *dev,
					 uint64_t *value)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->ecc_error_log_get == NULL) {
		return -ENOSYS;
	}

	return api->ecc_error_log_get(dev, value);
}

/**
 * @brief Clear ECC Error Log
 *
 * Clear value of ECC Error Log.
 *
 * @param dev Pointer to the device structure
 *
 * @retval 0 on success, error code otherwise
 * @retval -ENOSYS if the mandatory interface is not implemented
 */
static inline int edac_ecc_error_log_clear(const struct device *dev)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->ecc_error_log_clear == NULL) {
		return -ENOSYS;
	}

	return api->ecc_error_log_clear(dev);
}

/**
 * @brief Get Parity Error Log
 *
 * Read value of Parity Error Log.
 *
 * @param dev Pointer to the device structure
 * @param value Pointer to the parity Error Log value
 *
 * @retval 0 on success, error code otherwise
 * @retval -ENOSYS if the mandatory interface is not implemented
 */
static inline int edac_parity_error_log_get(const struct device *dev,
					    uint64_t *value)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->parity_error_log_get == NULL) {
		return -ENOSYS;
	}

	return api->parity_error_log_get(dev, value);
}

/**
 * @brief Clear Parity Error Log
 *
 * Clear value of Parity Error Log.
 *
 * @param dev Pointer to the device structure
 *
 * @retval 0 on success, error code otherwise
 * @retval -ENOSYS if the mandatory interface is not implemented
 */
static inline int edac_parity_error_log_clear(const struct device *dev)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->parity_error_log_clear == NULL) {
		return -ENOSYS;
	}

	return api->parity_error_log_clear(dev);
}

/**
 * @brief Get number of correctable errors
 *
 * @param dev Pointer to the device structure
 *
 * @retval num Number of correctable errors
 * @retval -ENOSYS if the mandatory interface is not implemented
 */
static inline int edac_errors_cor_get(const struct device *dev)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->errors_cor_get == NULL) {
		return -ENOSYS;
	}

	return api->errors_cor_get(dev);
}

/**
 * @brief Get number of uncorrectable errors
 *
 * @param dev Pointer to the device structure
 *
 * @retval num Number of uncorrectable errors
 * @retval -ENOSYS if the mandatory interface is not implemented
 */
static inline int edac_errors_uc_get(const struct device *dev)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->errors_uc_get == NULL) {
		return -ENOSYS;
	}

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
 * @retval 0 on success, error code otherwise
 * @retval -ENOSYS if the mandatory interface is not implemented
 */
static inline int edac_notify_callback_set(const struct device *dev,
					   edac_notify_callback_f cb)
{
	const struct edac_driver_api *api = DEVICE_API_GET(edac, dev);

	if (api->notify_cb_set == NULL) {
		return -ENOSYS;
	}

	return api->notify_cb_set(dev, cb);
}


/** @} */ /* End of EDAC Mandatory Interfaces */

/** @} */ /* End of EDAC API */

#endif  /* ZEPHYR_INCLUDE_DRIVERS_EDAC_H_ */
