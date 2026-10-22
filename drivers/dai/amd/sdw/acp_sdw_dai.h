/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DAI_AMD_ACP_SDW_DAI_H_
#define ZEPHYR_DRIVERS_DAI_AMD_ACP_SDW_DAI_H_

#include <zephyr/drivers/dai.h>
#include <zephyr/device.h>
#ifdef CONFIG_SOC_ACP_7_0
#include <acp70_chip_offsets.h>
#endif

/* ACP Configuration Request - SOF_IPC_DAI_AMD_CONFIG */
#define ACP_SDW_DEFAULT_RATE     48000
#define ACP_SDW_DEFAULT_CHANNELS 2

/* This needs to match SOF struct sof_ipc_dai_acp_sdw_params */
struct sof_ipc_dai_acp_sdw_params {
	uint32_t reserved0;
	uint32_t rate;
	uint32_t channels;
} __packed __aligned(4);

struct dai_plat_fifo_data {
	uint32_t offset;
	uint32_t width;
	uint32_t depth;
	uint32_t watermark;
	uint32_t handshake;
};

struct acp_dai_config {
	enum dai_type type;
	uint32_t dai_index;
	struct dai_config cfg;
	struct dai_properties prop;
	struct sof_ipc_dai_acp_sdw_params *acp_params;
	void (*irq_config)(void);
};

/* ACP private data */
struct acp_dai_pdata {
	struct acp_dai_config *priv_data;
	struct k_spinlock lock;
};

#define dai_get_drvdata(dai)       (&(dai)->priv_data)
#define dai_base(dai)              ((dai)->plat_data.base)
#define dai_set_drvdata(dai, data) ((dai)->priv_data = (data))

#define DAI_DIR_PLAYBACK 0
#define DAI_DIR_CAPTURE  1
#endif /* ZEPHYR_DRIVERS_DAI_AMD_ACP_SDW_DAI_H_*/
