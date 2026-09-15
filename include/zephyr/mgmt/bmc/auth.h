/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_BMC_AUTH_H_
#define ZEPHYR_INCLUDE_MGMT_BMC_AUTH_H_

/**
 * @file
 * @brief Authentication backend used by the BMC network services.
 */

#include <errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BMC authentication
 * @defgroup bmc_auth BMC authentication
 * @ingroup bmc_api
 * @{
 */

/**
 * @brief Operations used to authenticate BMC clients.
 *
 * The BMC core ships a backend that checks the credentials against the single
 * administrator account stored in the BMC configuration. Products that need a
 * different scheme, for example multiple accounts or a session service,
 * register their own implementation.
 */
struct bmc_auth_ops {
	/**
	 * @brief Check a user name and password pair.
	 *
	 * @param user User name presented by the client.
	 * @param password Password presented by the client.
	 *
	 * @return 0 if the credentials are valid, negative errno otherwise.
	 */
	int (*check)(const char *user, const char *password);
};

/**
 * @brief Register the authentication backend.
 *
 * A later registration replaces an earlier one. The @p ops structure must
 * remain valid for as long as it is registered.
 *
 * @param ops Operations to install, or NULL to restore the built-in backend.
 *
 * @return 0 on success.
 */
int bmc_auth_ops_register(const struct bmc_auth_ops *ops);

/**
 * @brief Check a user name and password pair against the active backend.
 *
 * @param user User name presented by the client.
 * @param password Password presented by the client.
 *
 * @retval 0 if the credentials are valid.
 * @retval -EACCES if they are not.
 */
int bmc_auth_check(const char *user, const char *password);

/**
 * @brief Check credentials given as a single separated string.
 *
 * Accepts the `user<sep>password` form used by HTTP basic authentication
 * (separator `:`) and by the websocket authentication handshake
 * (separator `_`).
 *
 * @param credentials Combined credentials string.
 * @param separator Character separating user name from password.
 *
 * @retval 0 if the credentials are valid.
 * @retval -EACCES if they are not.
 * @retval -EINVAL if @p credentials does not contain @p separator.
 */
int bmc_auth_check_pair(const char *credentials, char separator);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_BMC_AUTH_H_ */
