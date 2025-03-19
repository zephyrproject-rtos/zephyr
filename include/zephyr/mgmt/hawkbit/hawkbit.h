/*
 * Copyright (c) 2020 Linumiz
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief hawkBit main header file
 */

/**
 * @brief hawkBit Firmware Over-the-Air for Zephyr Project.
 * @defgroup hawkbit hawkBit Firmware Over-the-Air
 * @ingroup third_party
 * @{
 */

#ifndef ZEPHYR_INCLUDE_MGMT_HAWKBIT_HAWKBIT_H_
#define ZEPHYR_INCLUDE_MGMT_HAWKBIT_HAWKBIT_H_

#include <stdint.h>

/**
 * @brief Response message from hawkBit.
 *
 * @details These messages are used to inform the server and the
 * user about the process status of the hawkBit and also
 * used to standardize the errors that may occur.
 *
 */
enum hawkbit_response {
	/** matching events were not received within the specified time */
	HAWKBIT_NO_RESPONSE,
	/** an update was installed. Reboot is required to apply it */
	HAWKBIT_UPDATE_INSTALLED,
	/** no update was available */
	HAWKBIT_NO_UPDATE,
	/** fail to connect to the hawkBit server */
	HAWKBIT_NETWORKING_ERROR,
	/** image is unconfirmed */
	HAWKBIT_UNCONFIRMED_IMAGE,
	/** fail to get the permission to access the hawkBit server */
	HAWKBIT_PERMISSION_ERROR,
	/** fail to parse or to encode the metadata */
	HAWKBIT_METADATA_ERROR,
	/** fail while downloading the update package */
	HAWKBIT_DOWNLOAD_ERROR,
	/** fail to allocate memory */
	HAWKBIT_ALLOC_ERROR,
	/** hawkBit is not initialized */
	HAWKBIT_NOT_INITIALIZED,
	/** probe is currently running */
	HAWKBIT_PROBE_IN_PROGRESS,
};

/**
 * @brief Callback to provide the custom data to the hawkBit server.
 *
 * @details This callback is used to provide the custom data to the hawkBit server.
 * The custom data is used to provide the hawkBit server with the device specific
 * data.
 *
 * @param device_id The device ID.
 * @param buffer The buffer to store the json.
 * @param buffer_size The size of the buffer.
 */
typedef int (*hawkbit_config_device_data_cb_handler_t)(const char *device_id, uint8_t *buffer,
						  const size_t buffer_size);

/**
 * @brief Set the custom data callback.
 *
 * @details This function is used to set the custom data callback.
 * The callback is used to provide the custom data to the hawkBit server.
 *
 * @param cb The callback function.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the callback is NULL.
 */
int hawkbit_set_custom_data_cb(hawkbit_config_device_data_cb_handler_t cb);

/**
 * @brief Init the flash partition
 *
 * @retval 0 on success.
 * @retval -errno if init fails.
 */
int hawkbit_init(void);

/**
 * @brief The hawkBit probe verify if there is some update to be performed.
 *
 * @return A value from ::hawkbit_response.
 */
enum hawkbit_response hawkbit_probe(void);

/**
 * @brief Request system to reboot.
 */
void hawkbit_reboot(void);

/**
 * @brief Callback to get the device identity.
 *
 * @param id Pointer to the buffer to store the device identity
 * @param id_max_len The maximum length of the buffer
 */
typedef bool (*hawkbit_get_device_identity_cb_handler_t)(char *id, int id_max_len);

/**
 * @brief Set the device identity callback.
 *
 * @details This function is used to set a custom device identity callback.
 *
 * @param cb The callback function.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the callback is NULL.
 */
int hawkbit_set_device_identity_cb(hawkbit_get_device_identity_cb_handler_t cb);

/**
 * @brief Resets the hawkBit action id, that is saved in settings.
 *
 * @details This should be done after changing the hawkBit server.
 *
 * @retval 0 on success.
 * @retval -EAGAIN if probe is currently running.
 * @retval -EIO if the action id could not be reset.
 *
 */
int hawkbit_reset_action_id(void);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_HAWKBIT_HAWKBIT_H_ */
