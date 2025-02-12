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

#include <zephyr/net/tls_credentials.h>

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
	sec_tag_t tls_tag;
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
 * @brief Runs hawkBit probe and hawkBit update automatically
 *
 * @details The hawkbit_autohandler handles the whole process
 * in pre-determined time intervals.
 */
void hawkbit_autohandler(void);

/**
 * @brief The hawkBit probe verify if there is some update to be performed.
 *
 * @retval HAWKBIT_NETWORKING_ERROR fail to connect to the hawkBit server.
 * @retval HAWKBIT_UNCONFIRMED_IMAGE image is unconfirmed.
 * @retval HAWKBIT_PERMISSION_ERROR fail to get the permission to access the hawkBit server.
 * @retval HAWKBIT_METADATA_ERROR fail to parse or to encode the metadata.
 * @retval HAWKBIT_DOWNLOAD_ERROR fail while downloading the update package.
 * @retval HAWKBIT_OK if the image was already updated.
 * @retval HAWKBIT_UPDATE_INSTALLED if an update was installed. Reboot is required to apply it.
 * @retval HAWKBIT_NO_UPDATE if no update was available.
 * @retval HAWKBIT_CANCEL_UPDATE if the update was cancelled by the server.
 * @retval HAWKBIT_NOT_INITIALIZED if hawkBit is not initialized.
 * @retval HAWKBIT_PROBE_IN_PROGRESS if probe is currently running.
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
		.server_addr = addr_str, .server_port = 0, .auth_token = NULL, .tls_tag = 0};

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
		.server_addr = NULL, .server_port = port, .auth_token = NULL, .tls_tag = 0};

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
		.server_addr = NULL, .server_port = 0, .auth_token = token, .tls_tag = 0};

	return hawkbit_set_config(&set_config);
}

/**
 * @brief Set the hawkBit TLS tag
 *
 * @param tag TLS tag to set.
 * @retval 0 on success.
 * @retval -EAGAIN if probe is currently running.
 */
static inline int hawkbit_set_tls_tag(sec_tag_t tag)
{
	struct hawkbit_runtime_config set_config = {
		.server_addr = NULL, .server_port = 0, .auth_token = NULL, .tls_tag = tag};

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
 * @brief Get the hawkBit TLS tag.
 *
 * @return TLS tag.
 */
static inline sec_tag_t hawkbit_get_tls_tag(void)
{
	return hawkbit_get_config().tls_tag;
}

/**
 * @brief Get the hawkBit action id.
 *
 * @return Action id.

*/
int32_t hawkbit_get_action_id(void);

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

#endif /* _HAWKBIT_H_ */
