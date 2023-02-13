/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_DAI_DRIVER_ALH_H__
#define __INTEL_DAI_DRIVER_ALH_H__

#include <stdint.h>
#include <zephyr/drivers/dai.h>

#include "alh_map.h"

#define DAI_NUM_ALH_BI_DIR_LINKS_GROUP 4

#define ALH_STREAM_OFFSET 0x4

#define IPC4_ALH_MAX_NUMBER_OF_GTW 16
#define IPC4_ALH_DAI_INDEX_OFFSET 7

/* copier id = (group id << 4) + codec id + IPC4_ALH_DAI_INDEX_OFFSET
 * dai_index = (group id << 8) + codec id;
 */
#define IPC4_ALH_DAI_INDEX(x) ((((x) & 0xF0) << DAI_NUM_ALH_BI_DIR_LINKS_GROUP) + \
				(((x) & 0xF) - IPC4_ALH_DAI_INDEX_OFFSET))

#define ALH_GPDMA_BURST_LENGTH 4

#define ALH_SET_BITS(b_hi, b_lo, x)	\
	(((x) & ((1ULL << ((b_hi) - (b_lo) + 1ULL)) - 1ULL)) << (b_lo))
#define ALHASCTL_OSEL(x)          ALH_SET_BITS(25, 24, x)

#define dai_get_drvdata(dai) &dai->priv_data
#define dai_base(dai) dai->plat_data.base

#define DAI_DIR_PLAYBACK 0
#define DAI_DIR_CAPTURE 1

#define ALH_CHANNELS_DEFAULT 2
#define ALH_RATE_DEFAULT 48000
#define ALH_WORD_SIZE_DEFAULT 32

#if CONFIG_INTEL_ADSP_CAVS
#define ALH_TXDA_OFFSET 0x400
#define ALH_RXDA_OFFSET 0x500
#else
#define ALH_TXDA_OFFSET 0
#define ALH_RXDA_OFFSET 0x100
#endif

union dai_intel_ipc4_gateway_attributes {
	/**< Raw value */
	uint32_t dw;

	/**< Access to the fields */
	struct {
		/**< Gateway data requested in low power memory. */
		uint32_t lp_buffer_alloc : 1;

		/**< Gateway data requested in register file memory. */
		uint32_t alloc_from_reg_file : 1;

		/**< Reserved field */
		uint32_t _rsvd : 30;
	} bits; /**<< Bits */
} __packed;

/* ALH Configuration Request - SOF_IPC_DAI_ALH_CONFIG */
struct dai_intel_ipc3_alh_params {
	uint32_t reserved0;
	uint32_t stream_id;
	uint32_t rate;
	uint32_t channels;

	/* reserved for future use */
	uint32_t reserved[13];
} __packed;

struct ipc4_alh_multi_gtw_cfg {
	/* Number of single channels (valid items in mapping array). */
	uint32_t count;
	/* Single to multi aggregation mapping item. */
	struct {
		/* Vindex of a single ALH channel aggregated. */
		uint32_t alh_id;
		/* Channel mask */
		uint32_t channel_mask;
	} mapping[IPC4_ALH_MAX_NUMBER_OF_GTW]; /* < Mapping items */
} __packed;

struct dai_intel_ipc4_alh_configuration_blob {
	union dai_intel_ipc4_gateway_attributes gtw_attributes;
	struct ipc4_alh_multi_gtw_cfg alh_cfg;
} __packed;

struct dai_intel_alh_plat_data {
	uint32_t base;
	uint32_t fifo_depth[2];
};

struct dai_intel_alh_pdata {
	struct dai_config config;
	struct dai_properties props;
	struct dai_intel_ipc3_alh_params params;
};

struct dai_intel_alh {
	uint32_t index;		/**< index */
	struct k_spinlock lock;	/**< locking mechanism */
	int sref;		/**< simple ref counter, guarded by lock */
	struct dai_intel_alh_plat_data plat_data;
	struct dai_intel_alh_pdata priv_data;
};

#endif
