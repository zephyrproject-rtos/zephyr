/*
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCE_TABLE_H__
#define RESOURCE_TABLE_H__

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VDEV_ID                 0xFF
#define VRING0_ID 0 /* (master to remote) fixed to 0 for Linux compatibility */
#define VRING1_ID 1 /* (remote to master) fixed to 1 for Linux compatibility */

#define VRING_COUNT             2



struct fw_resource_table;


struct fw_resource_table *rsc_table_get(void);

size_t rsc_table_get_size(void);

struct metal_io_region *rsc_table_get_io_region(void);

struct fw_rsc_carveout *rsc_table_get_carveout_by_name(const char *name);

struct fw_rsc_carveout *rsc_table_get_carveout_by_idx(unsigned int idx);

struct fw_rsc_vdev *rsc_table_get_vdev(unsigned int vdev_idx);

struct fw_rsc_vdev_vring *rsc_table_get_vring0(unsigned int vdev_idx);

struct fw_rsc_vdev_vring *rsc_table_get_vring1(unsigned int vdev_idx);

struct metal_io_region *rsc_table_get_carveout_io_region(const struct fw_rsc_carveout *carveout);


#ifdef __cplusplus
}
#endif

#endif
