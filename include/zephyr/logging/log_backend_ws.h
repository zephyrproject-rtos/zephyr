/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the websocket log backend API
 * @ingroup log_backend_ws
 */

#ifndef ZEPHYR_LOG_BACKEND_WS_H_
#define ZEPHYR_LOG_BACKEND_WS_H_

/**
 * @brief Websocket log backend API
 * @defgroup log_backend_ws Websocket log backend API
 * @ingroup log_backend
 * @{
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register websocket socket where the logging output is sent.
 *
 * @param fd Websocket socket value.
 *
 * @return 0 if ok, <0 if error
 */
int log_backend_ws_register(int fd);

/**
 * @brief Unregister websocket socket where the logging output was sent.
 *
 * @details After this the websocket output is disabled.
 *
 * @param fd Websocket socket value.
 *
 * @return 0 if ok, <0 if error
 */
int log_backend_ws_unregister(int fd);

/**
 * @brief Get the websocket logger backend
 *
 * @details This function returns the websocket logger backend.
 *
 * @return Pointer to the websocket logger backend.
 */
const struct log_backend *log_backend_ws_get(void);

/**
 * @brief Start the websocket logger backend
 *
 * @details This function starts the websocket logger backend.
 */
void log_backend_ws_start(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_LOG_BACKEND_WS_H_ */
