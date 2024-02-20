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
 * @brief hawkBit configuration structure.
 *
 * @details This structure is used to store the hawkBit configuration
 * settings.
 */
struct hawkbit_set_config {
	char *server_addr;
	uint16_t server_port;
	char *auth_token;
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

#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME

/**
 * @brief Set the hawkBit server configuration settings.
 *
 * @param set_config Configuration settings to set.
 * @retval 0 on success.
 * @retval -EAGAIN if probe is currently running.
 */
int hawkbit_config_server(struct hawkbit_set_config *set_config);

/**
 * @brief Set the hawkBit server address.
 *
 * @param addr_str Server address to set.
 * @retval 0 on success.
 * @retval -EAGAIN if probe is currently running.
 */
static inline int hawkbit_set_server_addr(char *addr_str)
{
	struct hawkbit_set_config set_config = {
		.server_addr = addr_str, .server_port = 0, .auth_token = NULL};

	return hawkbit_config_server(&set_config);
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
	struct hawkbit_set_config set_config = {
		.server_addr = NULL, .server_port = port, .auth_token = NULL};

	return hawkbit_config_server(&set_config);
}

#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY

/**
 * @brief Set the hawkBit security token.
 *
 * @param token Security token to set.
 * @retval 0 on success.
 * @retval -EAGAIN if probe is currently running.
 */
static inline int hawkbit_set_ddi_security_token(char *token)
{
	struct hawkbit_set_config set_config = {
		.server_addr = NULL, .server_port = 0, .auth_token = token};

	return hawkbit_config_server(&set_config);
}

#endif /* CONFIG_HAWKBIT_DDI_NO_SECURITY */
#endif /* CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME */

/**
 * @brief Get the hawkBit server address.
 *
 * @return Server address.
 */
char *hawkbit_get_server_addr(void);

/**
 * @brief Get the hawkBit server port.
 *
 * @return Server port.
 */
uint16_t hawkbit_get_server_port(void);

/**
 * @}
 */

#endif /* _HAWKBIT_H_ */
