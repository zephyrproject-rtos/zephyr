/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NXP_RESOURCE_TABLE_H__
#define NXP_RESOURCE_TABLE_H__

#include <resource_table.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * struct fw_rsc_imx_dsp - i.MX DSP specific info
 *
 * @type: type of the resource entry
 * @len: length of the resource entry, without @type, but including @len field
 * @magic_num: 32-bit magic number
 * @version: version of data structure
 * @features: feature flags supported by the i.MX DSP firmware
 *
 * This represents a DSP-specific resource in the firmware's
 * resource table, providing information on supported features.
 */
struct fw_rsc_imx_dsp {
	uint32_t type;
	uint32_t len;
	uint32_t magic_num;
	uint32_t version;
	uint32_t features;
} __packed;


struct nxp_resource_table {
	struct resource_table hdr;
	uint32_t offset[2];
	struct fw_rsc_imx_dsp imx_vs_entry;
	struct fw_rsc_vdev vdev;
	struct fw_rsc_vdev_vring vring0;
	struct fw_rsc_vdev_vring vring1;
} METAL_PACKED_END;

#ifdef __cplusplus
}
#endif

#endif
