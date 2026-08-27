/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_DAI_DRIVER_HDA_H__
#define __INTEL_DAI_DRIVER_HDA_H__

#include <stdint.h>
#include <zephyr/drivers/dai.h>

#define dai_get_drvdata(dai) &dai->priv_data
#define dai_base(dai) dai->plat_data.base

#define DAI_INTEL_HDA_DEFAULT_WORD_SIZE 16

struct dai_intel_ipc_hda_params {
	uint32_t reserved0;
	uint32_t link_dma_ch;
	uint32_t rate;
	uint32_t channels;
} __packed;

struct dai_intel_hda_pdata {
	struct dai_config config;
	struct dai_properties props;
	struct dai_intel_ipc_hda_params params;
};

struct dai_intel_hda {
	uint32_t index;
	struct k_spinlock lock;
	struct dai_intel_hda_pdata priv_data;
};

#endif
