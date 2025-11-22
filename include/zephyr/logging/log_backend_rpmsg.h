/*
 * Copyright (c) 2025 Lexmark
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LOG_BACKEND_RPMSG_H_
#define ZEPHYR_LOG_BACKEND_RPMSG_H_

#include <openamp/rpmsg.h>
#include <zephyr/logging/log_backend.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Initialize the log backend using the provided @p rpmsg_dev device.
 *
 * @param rpmsg_dev A pointer to an RPMsg device
 * @return 0 on success or a negative value on error
 */
int log_backend_rpmsg_init_transport(struct rpmsg_device *rpmsg_dev);

/**
 * @brief Deinitialize the log backend and release resources.
 */
void log_backend_rpmsg_deinit_transport(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LOG_BACKEND_RPMSG_H_ */
