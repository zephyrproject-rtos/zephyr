/*
 * Copyright (c) 2023 David Corbeil
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LOG_BACKEND_NET_H_
#define ZEPHYR_LOG_BACKEND_NET_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allows user to set a server IP address at runtime
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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LOG_BACKEND_NET_H_ */
