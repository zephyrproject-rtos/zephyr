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
	HAWKBIT_NOT_INITIALIZED,
	HAWKBIT_PROBE_IN_PROGRESS,
};

/**
 * @brief hawkBit configuration structure.
 *
 * @details This structure is used to store the hawkBit configuration
 * settings.
 */
struct hawkbit_runtime_config {
	char *server_addr;
	uint16_t server_port;
	char *auth_token;
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
 * @return 0 on success.
 * @return -EINVAL if the callback is NULL.
 */
int hawkbit_set_device_identity_cb(hawkbit_get_device_identity_cb_handler_t cb);

/**
 * @brief Set the hawkBit server configuration settings.
 *
 * @param config Configuration settings to set.
 * @retval 0 on success.
 * @retval -EAGAIN if probe is currently running.
 */
int hawkbit_set_config(struct hawkbit_runtime_config *config);

/**
 * @brief Get the hawkBit server configuration settings.
 *
 * @return Configuration settings.
 */
struct hawkbit_runtime_config hawkbit_get_config(void);

/**
 * @brief Set the hawkBit server address.
 *
 * @param addr_str Server address to set.
 * @retval 0 on success.
 * @retval -EAGAIN if probe is currently running.
 */
static inline int hawkbit_set_server_addr(char *addr_str)
{
	struct hawkbit_runtime_config set_config = {
		.server_addr = addr_str, .server_port = 0, .auth_token = NULL};

	return hawkbit_set_config(&set_config);
}

/**
 * @brief Set the hawkBit server port.
 *
 * @param port Server port to set.
 * @retval 0 on success.
 * @retval -EAGAIN if probe is currently running.
 */
static inline int hawkbit_set_server_port(uint16_t port)
{
	struct hawkbit_runtime_config set_config = {
		.server_addr = NULL, .server_port = port, .auth_token = NULL};

	return hawkbit_set_config(&set_config);
}

/**
 * @brief Set the hawkBit security token.
 *
 * @param token Security token to set.
 * @retval 0 on success.
 * @retval -EAGAIN if probe is currently running.
 */
static inline int hawkbit_set_ddi_security_token(char *token)
{
	struct hawkbit_runtime_config set_config = {
		.server_addr = NULL, .server_port = 0, .auth_token = token};

	return hawkbit_set_config(&set_config);
}

/**
 * @brief Get the hawkBit server address.
 *
 * @return Server address.
 */
static inline char *hawkbit_get_server_addr(void)
{
	return hawkbit_get_config().server_addr;
}

/**
 * @brief Get the hawkBit server port.
 *
 * @return Server port.
 */
static inline uint16_t hawkbit_get_server_port(void)
{
	return hawkbit_get_config().server_port;
}

/**
 * @brief Get the hawkBit security token.
 *
 * @return Security token.
 */
static inline char *hawkbit_get_ddi_security_token(void)
{
	return hawkbit_get_config().auth_token;
}

/**
 * @}
 */

#endif /* _HAWKBIT_H_ */
