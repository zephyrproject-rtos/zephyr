/*
 * Copyright (c) 2024 Maurer Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for RemoteProc drivers and applications
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_REMOTEPROC_H_
#define ZEPHYR_INCLUDE_DRIVERS_REMOTEPROC_H_

#include <zephyr/types.h>


#ifdef __cplusplus
extern "C" {
#endif

#define VDEV_ID                 0xFF
#define VRING0_ID 0 /* (master to remote) fixed to 0 for Linux compatibility */
#define VRING1_ID 1 /* (remote to master) fixed to 1 for Linux compatibility */

#define VRING_COUNT             2



struct fw_resource_table;
struct fw_rsc_vdev_vring;
struct fw_rsc_vdev;
struct fw_rsc_carveout;
struct metal_io_region;

struct fw_resource_table *remoteproc_get_rsc_table(void);

size_t remoteproc_get_rsc_table_size(void);

struct metal_io_region *remoteproc_get_io_region(void);

struct fw_rsc_carveout *remoteproc_get_carveout_by_name(const char *name);

struct fw_rsc_carveout *remoteproc_get_carveout_by_idx(unsigned int idx);

struct fw_rsc_vdev *remoteproc_get_vdev(unsigned int vdev_idx);

struct fw_rsc_vdev_vring *remoteproc_get_vring(unsigned int vdev_idx, unsigned int vring_idx);

struct metal_io_region *remoteproc_get_carveout_io_region(const struct fw_rsc_carveout *carveout);




#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* ZEPHYR_INCLUDE_DRIVERS_REMOTEPROC_H_ */
