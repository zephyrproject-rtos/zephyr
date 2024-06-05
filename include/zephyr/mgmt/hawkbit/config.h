/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief hawkBit configuration header file
 */

/**
 * @brief hawkBit configuration API.
 * @defgroup hawkbit_config hawkBit configuration API
 * @ingroup hawkbit
 * @{
 */

#ifndef ZEPHYR_INCLUDE_MGMT_HAWKBIT_CONFIG_H_
#define ZEPHYR_INCLUDE_MGMT_HAWKBIT_CONFIG_H_

#include <stdint.h>
#include <zephyr/net/tls_credentials.h>

/**
 * @brief hawkBit configuration structure.
 *
 * @details This structure is used to store the hawkBit configuration
 * settings.
 */
struct hawkbit_runtime_config {
	/** Server address */
	char *server_addr;
	/** Server port */
	uint16_t server_port;
	/** Security token */
	char *auth_token;
	/** TLS tag */
	sec_tag_t tls_tag;
};

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
		.server_addr = addr_str,
		.server_port = 0,
		.auth_token = NULL,
		.tls_tag = 0,
	};

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
		.server_addr = NULL,
		.server_port = port,
		.auth_token = NULL,
		.tls_tag = 0,
	};

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
		.server_addr = NULL,
		.server_port = 0,
		.auth_token = token,
		.tls_tag = 0,
	};

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
		.server_addr = NULL,
		.server_port = 0,
		.auth_token = NULL,
		.tls_tag = tag,
	};

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
 * @brief Get the hawkBit poll interval.
 *
 * @return Poll interval.
 */
uint32_t hawkbit_get_poll_interval(void);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_HAWKBIT_CONFIG_H_ */
