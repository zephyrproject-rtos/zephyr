/** @file
 *  @brief Latency Monitor API
 */

/*
 * Copyright (c) 2025 Jorge A. Ramirez Ortiz <jorge.ramirez@oss.qualcomm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_LATMON_H_
#define ZEPHYR_INCLUDE_NET_LATMON_H_

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Latency Monitor
 * @defgroup latmon Latency Monitor
 * @ingroup networking
 * @{
 */

/**
 * @typedef net_latmon_measure_t
 * @brief Callback function type for retrieving latency deltas.
 *
 * @param delta Pointer to store the calculated latency delta in cycles.
 * @return 0 on success, negative errno code on failure.
 */
typedef int (*net_latmon_measure_t)(uint32_t *delta);

/**
 * @brief Start the latency monitor.
 *
 * @details This function starts the latency monitor, which measures
 * latency using the provided callback function to calculate deltas. Samples
 * are sent to the connected Latmus client.
 *
 * @param latmus A valid socket descriptor connected to latmus
 * @param measure_func A callback function to execute the delta calculation.
 */
void net_latmon_start(int latmus, net_latmon_measure_t measure_func);

/**
 * @brief Wait for a connection from a Latmus client.
 *
 * @details This function blocks until a Latmus client connects to the
 * specified socket. Once connected, the client's IP address is stored
 * in the provided `ip` structure.
 *
 * @param socket A valid socket descriptor for listening.
 * @param ip The client's IP address.
 * @return A valid client socket descriptor connected to latmus on success,
 * negative errno code on failure.
 *
 */
int net_latmon_connect(int socket, struct in_addr *ip);

/**
 * @brief Get a socket for the Latmus service.
 *
 * @details This function creates and returns a socket to wait for Latmus
 * connections
 *
 * @param bind_addr The address to bind the socket to. If NULL, the socket
 * will be bound to the first available address using the build time configured
 * latmus port.
 *
 * @return A valid socket descriptor on success, negative errno code on failure.
 */
int net_latmon_get_socket(struct sockaddr *bind_addr);

/**
 * @brief Check if the latency monitor is running.
 *
 * @details This function checks whether the latency monitor is currently
 * active and running.
 *
 * @return True if the latency monitor is running, false if it is waiting for a
 * Latmus connection
 */
bool net_latmon_running(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_LATMON_H_ */
