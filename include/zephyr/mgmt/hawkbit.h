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
 * @brief Callback to get the device identity.
 *
 * @param id Pointer to the buffer to store the device identity
 * @param id_max_len The maximum length of the buffer
 */
typedef bool (*hawkbit_get_device_identity_cb_handler_t)(char *id, int id_max_len);

#ifdef CONFIG_HAWKBIT_CUSTOM_DEVICE_ID
/**
 * @brief Set the device identity callback.
 * @details This function is used to set a custom device identity callback.
 * @param cb The callback function.
 * @retval 0 on success.
 * @retval -EINVAL if the callback is NULL.
 */
int hawkbit_set_device_identity_cb(hawkbit_get_device_identity_cb_handler_t cb);
#endif /* CONFIG_HAWKBIT_CUSTOM_DEVICE_ID */

/**
 * @}
 */

#endif /* _HAWKBIT_H_ */
