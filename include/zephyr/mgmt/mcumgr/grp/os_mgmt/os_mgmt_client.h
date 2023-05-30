/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_OS_MGMT_CLIENT_
#define H_OS_MGMT_CLIENT_

#include <inttypes.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OS mgmt client object
 */
struct os_mgmt_client {
	/** SMP client object  */
	struct smp_client_object *smp_client;
	/** Command status */
	int status;
};

/**
 * @brief Initialize OS management client.
 *
 * @param client OS mgmt client object
 * @param smp_client SMP client object
 *
 */
void os_mgmt_client_init(struct os_mgmt_client *client, struct smp_client_object *smp_client);

/**
 * @brief Send SMP message for Echo command.
 *
 * @param client OS mgmt client object
 * @param echo_string Echo string
 *
 * @return 0 on success.
 * @return @ref mcumgr_err_t code on failure.
 */
int os_mgmt_client_echo(struct os_mgmt_client *client, const char *echo_string);

/**
 * @brief Send SMP Reset command.
 *
 * @param client OS mgmt client object
 *
 * @return 0 on success.
 * @return @ref mcumgr_err_t code on failure.
 */
int os_mgmt_client_reset(struct os_mgmt_client *client);

#ifdef __cplusplus
}
#endif

#endif /* H_OS_MGMT_CLIENT_ */
