/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_H_
#define ZEPHYR_MCTP_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <libmctp.h>

/**
 * @brief Register a bus with the global MCTP context
 */
int zephyr_mctp_register_bus(struct mctp_binding *binding);

/**
 * @brief Open a socket-like connection to a MCTP peer
 */
int zephyr_mctp_open(uint8_t endpoint_id);

/**
 * @brief Close a MCTP socket
 */
int zephyr_mctp_close(int sock);

/**
 * @brief Setup the socket to be a listener
 *
 * Returns a socket id to accept on
 */
int zephyr_mctp_listen(void);

/**
 * @brief Accept new connections from peers
 *
 * Returns a new socket id for a peer
 */
int zephyr_mctp_accept(int listen_sock);

/**
 * @brief Get the MCTP peer endpoint for a handle
 */
int zephyr_mctp_endpoint(int sock, uint8_t *endpoint_id);

/**
 * @brief Initialize a poll event to await activity on a socket
 */
int zephyr_mctp_poll_event_init(int sock, struct k_poll_event *evt);

/**
 * @brief Write to a socket like handle for a connection to a peer over MCTP
 */
int zephyr_mctp_write(int sock, const uint8_t *msg, size_t len);

/**
 * @brief Read from a socket like handle up to some number of available bytes
 */
int zephyr_mctp_read(int sock, uint8_t *msg, size_t *len);

/**
 * @brief Read exactly the number of bytes for an endpoint
 */
int zephyr_mctp_read_exact(int sock, uint8_t *msg, size_t len);

#endif /* ZEPHYR_MCTP_H */
