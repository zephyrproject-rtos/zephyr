/*
 * Copyright (c) 2024 Maurer Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for RemoteProc drivers and applications
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_REMOTEPROC_VDEV_H_
#define ZEPHYR_INCLUDE_DRIVERS_REMOTEPROC_VDEV_H_

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rpmsg_device;

struct rpmsg_device *vdev_get_rpmsg_device(const struct device *dev);



#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* ZEPHYR_INCLUDE_DRIVERS_REMOTEPROC_VDEV_H_ */
