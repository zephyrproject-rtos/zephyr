/*
 * Copyright (c) 2025 Lexmark
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOG_BACKEND_RPMSG_H__
#define LOG_BACKEND_RPMSG_H__

#include <zephyr/logging/log_backend.h>
#include <openamp/rpmsg.h>

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

#ifdef __cplusplus
}
#endif

#endif /* LOG_BACKEND_RPMSG_H__ */
