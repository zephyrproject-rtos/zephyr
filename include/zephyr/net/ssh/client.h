/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SSH_CLIENT_H_
#define ZEPHYR_INCLUDE_NET_SSH_CLIENT_H_

/**
 * @file client.h
 *
 * @brief SSH client API
 *
 * @defgroup ssh_client SSH Client Library
 * @since 4.5
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ssh/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

struct ssh_client;

struct ssh_client *ssh_client_instance(int instance);

/** @endcond */

/**
 * @brief Start an SSH client connection
 *
 * @param ssh SSH client instance
 * @param user_name Username to use for authentication
 * @param addr Address of the SSH server to connect to
 * @param host_key_index Index of the host key to use for verifying the server's identity
 * @param callback Callback to be called on SSH transport events
 * @param user_data User data to be passed to the callback
 *
 * @return 0 on success, negative error code on failure
 */
int ssh_client_start(struct ssh_client *ssh,
		     const char *user_name,
		     const struct net_sockaddr *addr,
		     int host_key_index,
		     ssh_transport_event_callback_t callback,
		     void *user_data);

/**
 * @brief Stop an SSH client connection
 *
 * @param ssh SSH client instance
 *
 * @return 0 on success, negative error code on failure
 */
int ssh_client_stop(struct ssh_client *ssh);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_SSH_CLIENT_H_ */
