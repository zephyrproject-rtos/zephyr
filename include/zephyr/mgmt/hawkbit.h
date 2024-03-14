/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief hawkBit Firmware Over-the-Air for Zephyr Project.
 * @defgroup hawkbit hawkBit Firmware Over-the-Air
 * @ingroup third_party
 * @{
 */
#ifndef ZEPHYR_INCLUDE_MGMT_HAWKBIT_H_
#define ZEPHYR_INCLUDE_MGMT_HAWKBIT_H_

#define HAWKBIT_JSON_URL "/default/controller/v1"

/**
 * @brief Response message from hawkBit.
 *
 * @details These messages are used to inform the server and the
 * user about the process status of the hawkBit and also
 * used to standardize the errors that may occur.
 *
 */
enum hawkbit_response {
	HAWKBIT_NETWORKING_ERROR,
	HAWKBIT_UNCONFIRMED_IMAGE,
	HAWKBIT_PERMISSION_ERROR,
	HAWKBIT_METADATA_ERROR,
	HAWKBIT_DOWNLOAD_ERROR,
	HAWKBIT_OK,
	HAWKBIT_UPDATE_INSTALLED,
	HAWKBIT_NO_UPDATE,
	HAWKBIT_CANCEL_UPDATE,
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
 * @return 0 on success.
 * @return -EINVAL if the callback is NULL.
 */
int hawkbit_set_custom_data_cb(hawkbit_config_device_data_cb_handler_t cb);

/**
 * @brief Init the flash partition
 *
 * @return 0 on success, negative on error.
 */
int hawkbit_init(void);

/**
 * @brief Runs hawkBit probe and hawkBit update automatically
 *
 * @details The hawkbit_autohandler handles the whole process
 * in pre-determined time intervals.
 */
void hawkbit_autohandler(void);

/**
 * @brief The hawkBit probe verify if there is some update to be performed.
 *
 * @return HAWKBIT_UPDATE_INSTALLED has an update available.
 * @return HAWKBIT_NO_UPDATE no update available.
 * @return HAWKBIT_NETWORKING_ERROR fail to connect to the hawkBit server.
 * @return HAWKBIT_METADATA_ERROR fail to parse or to encode the metadata.
 * @return HAWKBIT_OK if success.
 * @return HAWKBIT_DOWNLOAD_ERROR fail while downloading the update package.
 */
enum hawkbit_response hawkbit_probe(void);

/**
 * @brief Request system to reboot.
 */
void hawkbit_reboot(void);

/**
 * @}
 */

#endif /* _HAWKBIT_H_ */
