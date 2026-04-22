/*
 * Copyright (c) 2025 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup biometrics_interface
 * @brief Main header file for biometrics driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BIOMETRICS_H_
#define ZEPHYR_INCLUDE_DRIVERS_BIOMETRICS_H_

/**
 * @brief Interfaces for biometric sensors.
 * @defgroup biometrics_interface Biometrics
 * @since 4.4
 * @version 0.2.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <stddef.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Biometric modality flags */
#define BIOMETRIC_MODALITY_FINGERPRINT BIT(0) /**< Fingerprint sensor */
#define BIOMETRIC_MODALITY_IRIS        BIT(1) /**< Iris scanner */
#define BIOMETRIC_MODALITY_FACE        BIT(2) /**< Face recognition */
#define BIOMETRIC_MODALITY_VOICE       BIT(3) /**< Voice recognition */
#define BIOMETRIC_MODALITY_PALM        BIT(4) /**< Palm recognition */

/** @brief Auto-assign template ID (sensor picks next available) */
#define BIOMETRIC_TEMPLATE_ID_AUTO 0

/** @brief Maximum serial number string length */
#define BIOMETRIC_SERIAL_NUMBER_MAX_LEN 16

/** @brief Maximum user name string length */
#define BIOMETRIC_USER_NAME_MAX_LEN 32

/**
 * @brief Biometric matching modes
 */
enum biometric_match_mode {
	BIOMETRIC_MATCH_VERIFY,   /**< Verify against specific template */
	BIOMETRIC_MATCH_IDENTIFY, /**< Search entire database */
};

/**
 * @brief Biometric storage modes
 */
#define BIOMETRIC_STORAGE_DEVICE BIT(0) /**< Store on sensor device */
#define BIOMETRIC_STORAGE_HOST   BIT(1) /**< Store on host system */

/**
 * @brief Biometric LED states
 */
enum biometric_led_state {
	BIOMETRIC_LED_OFF,    /**< LED off */
	BIOMETRIC_LED_ON,     /**< LED on continuously */
	BIOMETRIC_LED_BLINK,  /**< LED blinking */
	BIOMETRIC_LED_BREATHE /**< LED breathing effect */
};

/**
 * @brief Biometric attribute types
 */
enum biometric_attribute {
	/** Minimum confidence score for match acceptance (sensor-specific range) */
	BIOMETRIC_ATTR_MATCH_THRESHOLD,
	/** Minimum quality score for enrollment acceptance (sensor-specific range) */
	BIOMETRIC_ATTR_ENROLLMENT_QUALITY,
	/** Security level (1-10), higher values reduce false accepts */
	BIOMETRIC_ATTR_SECURITY_LEVEL,
	/** Default operation timeout in milliseconds */
	BIOMETRIC_ATTR_TIMEOUT_MS,
	/** Anti-spoofing detection sensitivity (1-10) */
	BIOMETRIC_ATTR_ANTI_SPOOF_LEVEL,
	/** Last captured image quality score (sensor-specific range), read-only */
	BIOMETRIC_ATTR_IMAGE_QUALITY,
	/** Duplicate enrollment policy (0 = reject duplicates, 1 = allow) */
	BIOMETRIC_ATTR_DUPLICATE_POLICY,
	/**
	 * Number of all common biometric attributes.
	 */
	BIOMETRIC_ATTR_COMMON_COUNT,
	/**
	 * This and higher values are sensor specific.
	 * Refer to the sensor header file.
	 */
	BIOMETRIC_ATTR_PRIV_START = BIOMETRIC_ATTR_COMMON_COUNT,
	/**
	 * Maximum value describing a biometric attribute type.
	 */
	BIOMETRIC_ATTR_MAX = INT16_MAX,
};

/**
 * @brief Biometric sensor capabilities
 */
struct biometric_capabilities {
	/** Bitmask of supported modalities */
	uint32_t supported_modalities;
	/** Maximum templates device can store */
	uint16_t max_templates;
	/** Size of each template in bytes */
	uint16_t template_size;
	/** Bitmask of supported storage modes */
	uint8_t storage_modes;
	/** Number of samples needed for enrollment */
	uint8_t enrollment_samples_required;
	/** Device serial number (empty if unavailable) */
	char serial_number[BIOMETRIC_SERIAL_NUMBER_MAX_LEN];
};

/**
 * @brief Result from a biometric match operation
 */
struct biometric_match_result {
	/** Confidence/match score (sensor-specific scale, higher is better) */
	int32_t confidence;
	/** Matched template ID (for IDENTIFY mode, or verified ID for VERIFY mode) */
	uint16_t template_id;
	/** Quality score of the captured sample used for matching (0-100) */
	uint8_t image_quality;
	/** Modality that produced this result */
	uint32_t modality;
};

/**
 * @brief Callback handler for asynchronous match results
 *
 * Called by the driver when a match result is available during continuous
 * matching.
 *
 * @param dev Pointer to the biometric device
 * @param result Pointer to the match result (only valid during callback)
 * @param user_data User data pointer provided during callback registration
 */
typedef void (*biometric_match_handler_t)(const struct device *dev,
					  const struct biometric_match_result *result,
					  void *user_data);

/**
 * @brief Result from an enrollment capture operation
 *
 * Contains progress information and quality feedback for the capture.
 */
struct biometric_capture_result {
	/** Number of samples captured so far */
	uint8_t samples_captured;
	/** Total number of samples required for enrollment */
	uint8_t samples_required;
	/** Quality score of the captured sample (0-100) */
	uint8_t quality;
	/** Assigned template ID */
	uint16_t template_id;
};

/**
 * @brief On-device user metadata
 */
struct biometric_user_info {
	/** User name */
	char name[BIOMETRIC_USER_NAME_MAX_LEN];
	/** Privilege level (0 = normal, 1 = admin) */
	uint8_t privilege;
};

/**
 * @def_driverbackendgroup{Biometrics,biometrics_interface}
 * @{
 */

/**
 * @brief Callback API to get sensor capabilities.
 * See biometric_get_capabilities() for argument description
 */
typedef int (*biometric_api_get_capabilities)(const struct device *dev,
					      struct biometric_capabilities *caps);

/**
 * @brief Callback API to set a sensor attribute.
 * See biometric_attr_set() for argument description.
 */
typedef int (*biometric_api_attr_set)(const struct device *dev, enum biometric_attribute attr,
				      int32_t val);

/**
 * @brief Callback API to get a sensor attribute.
 * See biometric_attr_get() for argument description
 */
typedef int (*biometric_api_attr_get)(const struct device *dev, enum biometric_attribute attr,
				      int32_t *val);

/**
 * @brief Callback API to start enrollment.
 * See biometric_enroll_start() for argument description
 */
typedef int (*biometric_api_enroll_start)(const struct device *dev, uint16_t template_id);

/**
 * @brief Callback API to capture enrollment samples.
 * See biometric_enroll_capture() for argument description
 */
typedef int (*biometric_api_enroll_capture)(const struct device *dev, k_timeout_t timeout,
					    struct biometric_capture_result *result);

/**
 * @brief Callback API to finalize enrollment.
 * See biometric_enroll_finalize() for argument description
 */
typedef int (*biometric_api_enroll_finalize)(const struct device *dev);

/**
 * @brief Callback API to abort enrollment.
 * See biometric_enroll_abort() for argument description
 */
typedef int (*biometric_api_enroll_abort)(const struct device *dev);

/**
 * @brief Callback API to store template.
 * See biometric_template_store() for argument description
 */
typedef int (*biometric_api_template_store)(const struct device *dev, uint16_t id,
					    const uint8_t *data, size_t size);

/**
 * @brief Callback API to read template data.
 * See biometric_template_read() for argument description
 */
typedef int (*biometric_api_template_read)(const struct device *dev, uint16_t id, uint8_t *data,
					   size_t size);

/**
 * @brief Callback API to delete template.
 * See biometric_template_delete() for argument description
 */
typedef int (*biometric_api_template_delete)(const struct device *dev, uint16_t id);

/**
 * @brief Callback API to delete all templates.
 * See biometric_template_delete_all() for argument description
 */
typedef int (*biometric_api_template_delete_all)(const struct device *dev);

/**
 * @brief Callback API to list template IDs.
 * See biometric_template_list() for argument description
 */
typedef int (*biometric_api_template_list)(const struct device *dev, uint16_t *ids,
					   size_t max_count, size_t *actual_count);

/**
 * @brief Callback API to start matching operation.
 * See biometric_match() for argument description
 */
typedef int (*biometric_api_match)(const struct device *dev, enum biometric_match_mode mode,
				   uint16_t template_id, k_timeout_t timeout,
				   struct biometric_match_result *result);

/**
 * @brief Callback API to control LED state.
 * See biometric_led_control() for argument description
 */
typedef int (*biometric_api_led_control)(const struct device *dev, enum biometric_led_state state);

/**
 * @brief Callback API to get user info.
 * See biometric_user_info_get() for argument description
 */
typedef int (*biometric_api_user_info_get)(const struct device *dev, uint16_t template_id,
					   struct biometric_user_info *info);

/**
 * @brief Callback API to set user info.
 * See biometric_user_info_set() for argument description
 */
typedef int (*biometric_api_user_info_set)(const struct device *dev, uint16_t template_id,
					   const struct biometric_user_info *info);

/**
 * @brief Callback API to get extra match data.
 * See biometric_match_get_data() for argument description
 */
typedef int (*biometric_api_match_get_data)(const struct device *dev, uint8_t *buf, size_t max_len,
					    size_t *len);

/**
 * @brief Callback API to set match result callback.
 * See biometric_match_set_callback() for argument description
 */
typedef int (*biometric_api_match_set_cb)(const struct device *dev,
					  biometric_match_handler_t handler, void *user_data);

/**
 * @brief Callback API to start continuous matching.
 * See biometric_match_start() for argument description
 */
typedef int (*biometric_api_match_start)(const struct device *dev, enum biometric_match_mode mode,
					 uint16_t template_id);

/**
 * @brief Callback API to stop continuous matching.
 * See biometric_match_stop() for argument description
 */
typedef int (*biometric_api_match_stop)(const struct device *dev);

/**
 * @brief Callback API to enroll from image data.
 * See biometric_enroll_from_image() for argument description
 */
typedef int (*biometric_api_enroll_from_image)(const struct device *dev, uint16_t template_id,
					       const uint8_t *data, size_t size);

/**
 * @driver_ops{Biometrics}
 */
__subsystem struct biometric_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief biometric_get_capabilities
	 */
	biometric_api_get_capabilities get_capabilities;
	/**
	 * @driver_ops_optional @copybrief biometric_attr_set
	 */
	biometric_api_attr_set attr_set;
	/**
	 * @driver_ops_optional @copybrief biometric_attr_get
	 */
	biometric_api_attr_get attr_get;
	/**
	 * @driver_ops_mandatory @copybrief biometric_enroll_start
	 */
	biometric_api_enroll_start enroll_start;
	/**
	 * @driver_ops_mandatory @copybrief biometric_enroll_capture
	 */
	biometric_api_enroll_capture enroll_capture;
	/**
	 * @driver_ops_mandatory @copybrief biometric_enroll_finalize
	 */
	biometric_api_enroll_finalize enroll_finalize;
	/**
	 * @driver_ops_optional @copybrief biometric_enroll_abort
	 */
	biometric_api_enroll_abort enroll_abort;
	/**
	 * @driver_ops_optional @copybrief biometric_template_store
	 */
	biometric_api_template_store template_store;
	/**
	 * @driver_ops_optional @copybrief biometric_template_read
	 */
	biometric_api_template_read template_read;
	/**
	 * @driver_ops_mandatory @copybrief biometric_template_delete
	 */
	biometric_api_template_delete template_delete;
	/**
	 * @driver_ops_optional @copybrief biometric_template_delete_all
	 */
	biometric_api_template_delete_all template_delete_all;
	/**
	 * @driver_ops_optional @copybrief biometric_template_list
	 */
	biometric_api_template_list template_list;
	/**
	 * @driver_ops_mandatory @copybrief biometric_match
	 */
	biometric_api_match match;
	/**
	 * @driver_ops_optional @copybrief biometric_led_control
	 */
	biometric_api_led_control led_control;
	/**
	 * @driver_ops_optional @copybrief biometric_user_info_get
	 */
	biometric_api_user_info_get user_info_get;
	/**
	 * @driver_ops_optional @copybrief biometric_user_info_set
	 */
	biometric_api_user_info_set user_info_set;
	/**
	 * @driver_ops_optional @copybrief biometric_match_get_data
	 */
	biometric_api_match_get_data match_get_data;
	/**
	 * @driver_ops_optional @copybrief biometric_match_set_callback
	 */
	biometric_api_match_set_cb match_set_cb;
	/**
	 * @driver_ops_optional @copybrief biometric_match_start
	 */
	biometric_api_match_start match_start;
	/**
	 * @driver_ops_optional @copybrief biometric_match_stop
	 */
	biometric_api_match_stop match_stop;
	/**
	 * @driver_ops_optional @copybrief biometric_enroll_from_image
	 */
	biometric_api_enroll_from_image enroll_from_image;
};

/**
 * @}
 */

/**
 * @brief Get biometric sensor capabilities
 *
 * @param dev Pointer to the biometric device
 * @param caps Pointer to capabilities structure to populate
 *
 * @retval 0 Success
 * @retval -errno Negative errno code on failure
 */
__syscall int biometric_get_capabilities(const struct device *dev,
					 struct biometric_capabilities *caps);

static inline int z_impl_biometric_get_capabilities(const struct device *dev,
						    struct biometric_capabilities *caps)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	__ASSERT_NO_MSG(caps != NULL);

	return api->get_capabilities(dev, caps);
}

/**
 * @brief Set a biometric sensor attribute
 *
 * @param dev Pointer to the biometric device
 * @param attr The attribute to set
 * @param val The value to set
 *
 * @retval 0 Success
 * @retval -ENOSYS Not supported by device
 * @retval -EINVAL Invalid attribute or value
 * @retval -errno Negative errno code on failure
 */
__syscall int biometric_attr_set(const struct device *dev, enum biometric_attribute attr,
				 int32_t val);

static inline int z_impl_biometric_attr_set(const struct device *dev, enum biometric_attribute attr,
					    int32_t val)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	if (api->attr_set == NULL) {
		return -ENOSYS;
	}

	return api->attr_set(dev, attr, val);
}

/**
 * @brief Get a biometric sensor attribute
 *
 * @param dev Pointer to the biometric device
 * @param attr The attribute to get
 * @param val Pointer to store the attribute value
 *
 * @retval 0 Success
 * @retval -ENOSYS Not supported by device
 * @retval -EINVAL Invalid attribute
 * @retval -errno Negative errno code on failure
 */
__syscall int biometric_attr_get(const struct device *dev, enum biometric_attribute attr,
				 int32_t *val);

static inline int z_impl_biometric_attr_get(const struct device *dev, enum biometric_attribute attr,
					    int32_t *val)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	__ASSERT_NO_MSG(val != NULL);

	if (api->attr_get == NULL) {
		return -ENOSYS;
	}

	return api->attr_get(dev, attr, val);
}

/**
 * @brief Start biometric enrollment
 *
 * Begins enrollment for a new template. After calling this, use
 * biometric_enroll_capture() to capture samples, then biometric_enroll_finalize()
 * to complete, or biometric_enroll_abort() to cancel.
 *
 * @param dev Biometric device
 * @param template_id Template ID to assign (1-based: valid range is 1 to max_templates)
 *
 * @retval 0 on success
 * @retval -EINVAL Invalid template_id or enrollment in progress
 * @retval -ENOSPC No space for new template
 * @retval -errno code on failure
 */
__syscall int biometric_enroll_start(const struct device *dev, uint16_t template_id);

static inline int z_impl_biometric_enroll_start(const struct device *dev, uint16_t template_id)
{
	return DEVICE_API_GET(biometric, dev)->enroll_start(dev, template_id);
}

/**
 * @brief Capture enrollment samples
 *
 * Captures a sample for enrollment. This function blocks until a sample is captured
 * or the timeout expires. Call this function multiple times (typically twice) to
 * collect all required samples before finalizing enrollment.
 *
 * @param dev Biometric device
 * @param timeout Timeout for sample capture
 * @param result Optional pointer to capture result structure for progress/quality info.
 *               Pass NULL if not needed.
 *
 * @retval 0 Sample successfully captured
 * @retval -EINVAL No enrollment started or already have enough samples
 * @retval -ETIMEDOUT Timeout waiting for sample
 * @retval -errno Other negative errno code on failure
 */
__syscall int biometric_enroll_capture(const struct device *dev, k_timeout_t timeout,
				       struct biometric_capture_result *result);

static inline int z_impl_biometric_enroll_capture(const struct device *dev, k_timeout_t timeout,
						  struct biometric_capture_result *result)
{
	return DEVICE_API_GET(biometric, dev)->enroll_capture(dev, timeout, result);
}

/**
 * @brief Finalize enrollment
 *
 * Completes enrollment and stores the template.
 *
 * @param dev Biometric device
 *
 * @retval 0 on success
 * @retval -EINVAL Insufficient samples
 * @retval -ENOSPC No space to store template
 * @retval -errno code on failure
 */
__syscall int biometric_enroll_finalize(const struct device *dev);

static inline int z_impl_biometric_enroll_finalize(const struct device *dev)
{
	return DEVICE_API_GET(biometric, dev)->enroll_finalize(dev);
}

/**
 * @brief Abort enrollment
 *
 * Cancels ongoing enrollment and resets enrollment state.
 *
 * @param dev Biometric device
 *
 * @retval 0 on success
 * @retval -EALREADY No enrollment in progress
 * @retval -ENOSYS Not supported by device
 * @retval -errno code on failure
 */
__syscall int biometric_enroll_abort(const struct device *dev);

static inline int z_impl_biometric_enroll_abort(const struct device *dev)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	if (api->enroll_abort == NULL) {
		return -ENOSYS;
	}

	return api->enroll_abort(dev);
}

/**
 * @brief Store biometric template
 *
 * @details Stores a pre-generated template. Used for host-based storage
 * systems or restoring backed-up templates.
 *
 * @param dev Pointer to the biometric device
 * @param id Template ID
 * @param data Pointer to template data
 * @param size Size of template data in bytes
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid template data
 * @retval -ENOSPC Storage full
 * @retval -ENOSYS Not supported by device
 * @retval -errno Negative errno code on failure
 */
__syscall int biometric_template_store(const struct device *dev, uint16_t id, const uint8_t *data,
				       size_t size);

static inline int z_impl_biometric_template_store(const struct device *dev, uint16_t id,
						  const uint8_t *data, size_t size)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	__ASSERT_NO_MSG(data != NULL);

	if (api->template_store == NULL) {
		return -ENOSYS;
	}

	return api->template_store(dev, id, data, size);
}

/**
 * @brief Read biometric template
 *
 * @details Retrieves template data. The data buffer must be allocated by
 * caller with sufficient size (check capabilities for template_size).
 *
 * @param dev Pointer to the biometric device
 * @param id Template ID to read
 * @param data Pointer to buffer for template data
 * @param size Size of data buffer
 *
 * @retval >=0 Number of bytes written
 * @retval -EINVAL Invalid template_id or buffer too small
 * @retval -ENOENT Template not found
 * @retval -ENOSYS Not supported by device
 * @retval -errno Negative errno code on failure
 */
__syscall int biometric_template_read(const struct device *dev, uint16_t id, uint8_t *data,
				      size_t size);

static inline int z_impl_biometric_template_read(const struct device *dev, uint16_t id,
						 uint8_t *data, size_t size)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	__ASSERT_NO_MSG(data != NULL);

	if (api->template_read == NULL) {
		return -ENOSYS;
	}

	return api->template_read(dev, id, data, size);
}

/**
 * @brief Delete biometric template
 *
 * @param dev Pointer to the biometric device
 * @param id Template ID to delete
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid template_id
 * @retval -ENOENT Template not found
 * @retval -errno Negative errno code on failure
 */
__syscall int biometric_template_delete(const struct device *dev, uint16_t id);

static inline int z_impl_biometric_template_delete(const struct device *dev, uint16_t id)
{
	return DEVICE_API_GET(biometric, dev)->template_delete(dev, id);
}

/**
 * @brief Delete all biometric templates
 *
 * @param dev Pointer to the biometric device
 *
 * @retval 0 Success
 * @retval -ENOSYS Not supported by device
 * @retval -errno Negative errno code on failure
 */
__syscall int biometric_template_delete_all(const struct device *dev);

static inline int z_impl_biometric_template_delete_all(const struct device *dev)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	if (api->template_delete_all == NULL) {
		return -ENOSYS;
	}

	return api->template_delete_all(dev);
}

/**
 * @brief List stored template IDs
 *
 * @param dev Pointer to the biometric device
 * @param ids Pointer to array to populate with template IDs
 * @param max_count Maximum number of IDs array can hold
 * @param actual_count Pointer to store actual number of IDs returned
 *
 * @retval 0 Success
 * @retval -ENOSYS Not supported by device
 * @retval -errno Negative errno code on failure
 */
__syscall int biometric_template_list(const struct device *dev, uint16_t *ids, size_t max_count,
				      size_t *actual_count);

static inline int z_impl_biometric_template_list(const struct device *dev, uint16_t *ids,
						 size_t max_count, size_t *actual_count)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	__ASSERT_NO_MSG(ids != NULL);
	__ASSERT_NO_MSG(actual_count != NULL);

	if (api->template_list == NULL) {
		return -ENOSYS;
	}

	return api->template_list(dev, ids, max_count, actual_count);
}

/**
 * @brief Perform biometric matching
 *
 * Captures a sample and performs matching. This function blocks until matching
 * completes or timeout expires. For BIOMETRIC_MATCH_VERIFY, verifies against
 * template_id. For BIOMETRIC_MATCH_IDENTIFY, searches all templates.
 *
 * @param dev Biometric device
 * @param mode Verify or identify mode
 * @param template_id Template ID for verify mode (ignored in identify mode)
 * @param timeout Timeout for sample capture and matching
 * @param result Optional pointer to match result structure for detailed info.
 *               Pass NULL if only match/no-match status is needed.
 *
 * @retval 0 Match successful
 * @retval -ETIMEDOUT Timeout waiting for sample
 * @retval -ENOENT No match found
 * @retval -errno Other negative errno code on failure
 */
__syscall int biometric_match(const struct device *dev, enum biometric_match_mode mode,
			      uint16_t template_id, k_timeout_t timeout,
			      struct biometric_match_result *result);

static inline int z_impl_biometric_match(const struct device *dev, enum biometric_match_mode mode,
					 uint16_t template_id, k_timeout_t timeout,
					 struct biometric_match_result *result)
{
	return DEVICE_API_GET(biometric, dev)->match(dev, mode, template_id, timeout, result);
}

/**
 * @brief Control biometric sensor LED
 *
 * Controls LED state for user feedback during operations.
 *
 * @param dev Biometric device
 * @param state Desired LED state
 *
 * @retval 0 on success
 * @retval -ENOSYS Not supported by device
 * @retval -EINVAL Invalid state
 * @retval -errno code on failure
 */
__syscall int biometric_led_control(const struct device *dev, enum biometric_led_state state);

static inline int z_impl_biometric_led_control(const struct device *dev,
					       enum biometric_led_state state)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	if (api->led_control == NULL) {
		return -ENOSYS;
	}

	return api->led_control(dev, state);
}

/**
 * @brief Get on-device user metadata
 *
 * @param dev Biometric device
 * @param template_id Template ID to query
 * @param info Pointer to user info structure to populate
 *
 * @retval 0 on success
 * @retval -ENOSYS Not supported by device
 * @retval -ENOENT Template not found
 * @retval -errno code on failure
 */
__syscall int biometric_user_info_get(const struct device *dev, uint16_t template_id,
				      struct biometric_user_info *info);

static inline int z_impl_biometric_user_info_get(const struct device *dev, uint16_t template_id,
						 struct biometric_user_info *info)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	__ASSERT_NO_MSG(info != NULL);

	if (api->user_info_get == NULL) {
		return -ENOSYS;
	}

	return api->user_info_get(dev, template_id, info);
}

/**
 * @brief Set on-device user metadata
 *
 * @param dev Biometric device
 * @param template_id Template ID to set metadata for
 * @param info Pointer to user info to store
 *
 * @retval 0 on success
 * @retval -ENOSYS Not supported by device
 * @retval -ENOENT Template not found
 * @retval -errno code on failure
 */
__syscall int biometric_user_info_set(const struct device *dev, uint16_t template_id,
				      const struct biometric_user_info *info);

static inline int z_impl_biometric_user_info_set(const struct device *dev, uint16_t template_id,
						 const struct biometric_user_info *info)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	__ASSERT_NO_MSG(info != NULL);

	if (api->user_info_set == NULL) {
		return -ENOSYS;
	}

	return api->user_info_set(dev, template_id, info);
}

/**
 * @brief Retrieve extra data from the last match result
 *
 * Returns modality-specific auxiliary data when provided by the driver.
 * Must be called before the next match operation (data is overwritten).
 *
 * @param dev Biometric device
 * @param buf Buffer to receive data
 * @param max_len Buffer size
 * @param len Actual data length written
 *
 * @retval 0 on success
 * @retval -ENODATA No extra data available
 * @retval -ENOSYS Not supported by device
 * @retval -errno code on failure
 */
__syscall int biometric_match_get_data(const struct device *dev, uint8_t *buf, size_t max_len,
				       size_t *len);

static inline int z_impl_biometric_match_get_data(const struct device *dev, uint8_t *buf,
						  size_t max_len, size_t *len)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len != NULL);

	if (api->match_get_data == NULL) {
		return -ENOSYS;
	}

	return api->match_get_data(dev, buf, max_len, len);
}

/**
 * @brief Set callback for asynchronous match results
 *
 * Registers a callback function that will be invoked for each match result
 * during continuous matching.
 *
 * @param dev Biometric device
 * @param handler Callback function, or NULL to clear
 * @param user_data User data passed to callback
 *
 * @retval 0 on success
 * @retval -EBUSY Continuous matching is active
 * @retval -ENOSYS Not supported by device
 * @retval -errno code on failure
 */
__syscall int biometric_match_set_callback(const struct device *dev,
					   biometric_match_handler_t handler, void *user_data);

static inline int z_impl_biometric_match_set_callback(const struct device *dev,
						      biometric_match_handler_t handler,
						      void *user_data)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	if (api->match_set_cb == NULL) {
		return -ENOSYS;
	}

	return api->match_set_cb(dev, handler, user_data);
}

/**
 * @brief Start continuous biometric matching
 *
 * Begins continuous matching. Each time a biometric is recognized, the
 * callback registered with biometric_match_set_callback() is invoked.
 * Call biometric_match_stop() to end continuous matching.
 *
 * @param dev Biometric device
 * @param mode Verify or identify mode
 * @param template_id Template ID for verify mode (ignored in identify mode)
 *
 * @retval 0 on success
 * @retval -EINVAL No callback registered
 * @retval -EBUSY Already in continuous matching mode
 * @retval -ENOSYS Not supported by device
 * @retval -errno code on failure
 */
__syscall int biometric_match_start(const struct device *dev, enum biometric_match_mode mode,
				    uint16_t template_id);

static inline int z_impl_biometric_match_start(const struct device *dev,
					       enum biometric_match_mode mode, uint16_t template_id)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	if (api->match_start == NULL) {
		return -ENOSYS;
	}

	return api->match_start(dev, mode, template_id);
}

/**
 * @brief Stop continuous biometric matching
 *
 * @param dev Biometric device
 *
 * @retval 0 on success
 * @retval -EALREADY Not currently in continuous matching mode
 * @retval -ENOSYS Not supported by device
 * @retval -errno code on failure
 */
__syscall int biometric_match_stop(const struct device *dev);

static inline int z_impl_biometric_match_stop(const struct device *dev)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	if (api->match_stop == NULL) {
		return -ENOSYS;
	}

	return api->match_stop(dev);
}

/**
 * @brief Enroll a biometric template from image data
 *
 * Enrolls from a pre-captured image rather than live sensor capture.
 *
 * @param dev Biometric device
 * @param template_id Template ID to assign (or BIOMETRIC_TEMPLATE_ID_AUTO)
 * @param data Image data (format is sensor-dependent, like JPEG)
 * @param size Image data size in bytes
 *
 * @retval 0 on success
 * @retval -ENOSYS Not supported by device
 * @retval -EINVAL Invalid image data
 * @retval -ENOSPC Storage full
 * @retval -errno code on failure
 */
__syscall int biometric_enroll_from_image(const struct device *dev, uint16_t template_id,
					  const uint8_t *data, size_t size);

static inline int z_impl_biometric_enroll_from_image(const struct device *dev, uint16_t template_id,
						     const uint8_t *data, size_t size)
{
	const struct biometric_driver_api *api = DEVICE_API_GET(biometric, dev);

	__ASSERT_NO_MSG(data != NULL);

	if (api->enroll_from_image == NULL) {
		return -ENOSYS;
	}

	return api->enroll_from_image(dev, template_id, data, size);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/biometrics.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_BIOMETRICS_H_ */
