/*
 * Copyright (c) 2023 David Corbeil
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LOG_BACKEND_NET_H_
#define ZEPHYR_LOG_BACKEND_NET_H_

#include <stdbool.h>
#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allows user to set a server IP address, provided as string, at runtime
 *
 * @details This function allows the user to set an IPv4 or IPv6 address at runtime. It can be
 *          called either before or after the backend has been initialized. If it gets called when
 *          the net logger backend context is running, it'll release it and create another one with
 *          the new address next time process() gets called.
 *
 * @param addr     String that contains the IP address.
 *
 * @return True if parsing could be done, false otherwise.
 */
bool log_backend_net_set_addr(const char *addr);

/**
 * @brief Allows user to set a server IP address, provided as sockaddr structure, at runtime
 *
 * @details This function allows the user to set an IPv4 or IPv6 address at runtime. It can be
 *          called either before or after the backend has been initialized. If it gets called when
 *          the net logger backend context is running, it'll release it and create another one with
 *          the new address next time process() gets called.
 *
 * @param addr     Pointer to the sockaddr structure that contains the IP address.
 *
 * @return True if address could be set, false otherwise.
 */
bool log_backend_net_set_ip(const struct sockaddr *addr);

/**
 * @brief update the hostname
 *
 * @details This function allows to update the hostname displayed by the logging backend. It will be
 *          called by the network stack if the hostname is set with net_hostname_set().
 *
 * @param hostname new hostname as char array.
 * @param len Length of the hostname array.
 */
#if defined(CONFIG_NET_HOSTNAME_ENABLE)
void log_backend_net_hostname_set(char *hostname, size_t len);
#else
static inline void log_backend_net_hostname_set(const char *hostname, size_t len)
{
	ARG_UNUSED(hostname);
	ARG_UNUSED(len);
}
#endif

/**
 * @brief Get the net logger backend
 *
 * @details This function returns the net logger backend.
 *
 * @return Pointer to the net logger backend.
 */
const struct log_backend *log_backend_net_get(void);

/**
 * @brief Start the net logger backend
 *
 * @details This function starts the net logger backend.
 */
void log_backend_net_start(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LOG_BACKEND_NET_H_ */
