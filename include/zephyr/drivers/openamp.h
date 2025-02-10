/*
 * Copyright (c) 2024 Maurer Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for RemoteProc drivers and applications
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_OPENAMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_OPENAMP_H_

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fw_rsc_vdev_vring;
struct fw_rsc_vdev;
struct fw_rsc_carveout;

void *openamp_get_rsc_table(void);

size_t openamp_get_rsc_table_size(void);

struct fw_rsc_carveout *openamp_get_carveout_by_name(const char *name);

struct fw_rsc_carveout *openamp_get_carveout_by_index(unsigned int idx);

struct fw_rsc_carveout *openamp_get_carveout_by_address(uint32_t idx);

struct fw_rsc_vdev *openamp_get_vdev(unsigned int idx);

struct rpmsg_device *openamp_get_rpmsg_device(const struct device *dev);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_OPENAMP_H_ */
