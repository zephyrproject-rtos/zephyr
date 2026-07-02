/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RPMsgFS initialize function declaration
 */

#ifndef ZEPHYR_INCLUDE_FS_RPMSGFS_H_
#define ZEPHYR_INCLUDE_FS_RPMSGFS_H_

#include <openamp/rpmsg.h>

/**
 * @brief Initialize RPMsgFS
 *
 * Passes the RPMsg device to create endpoints required for RPMsgFS
 *
 * @param rpmsg_dev Pointer to RPMsg device
 */
void rpmsgfs_init_rpmsg(struct rpmsg_device *rpmsg_dev);

#endif /* ZEPHYR_INCLUDE_FS_RPMSGFS_H_ */
