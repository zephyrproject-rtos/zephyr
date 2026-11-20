/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DAI_AMD_ACP_DMIC_DAI_H_
#define ZEPHYR_DRIVERS_DAI_AMD_ACP_DMIC_DAI_H_

#include <zephyr/logging/log.h>
#include <zephyr/drivers/dai.h>
#include <zephyr/device.h>
#if defined(CONFIG_SOC_ACP_7_X)
#include <acp7x_chip_offsets.h>
#endif

/* ACP private data */
struct sof_ipc_dai_acpdmic_params {
	uint32_t pdm_rate;
	uint32_t pdm_ch;
} __packed __aligned(4);

struct dai_plat_fifo_data {
	uint32_t offset;
	uint32_t width;
	uint32_t depth;
	uint32_t watermark;
	uint32_t handshake;
};

struct acp_dmic_dai_config {
	enum dai_type type;
	uint32_t dai_index;
	struct dai_config cfg;
	struct dai_properties prop;
	struct sof_ipc_dai_acpdmic_params *dmic_params;
	struct sof_ipc_dai_acpdmic_params *dmic1_params;
	void (*irq_config)(void);
};

/* ACP private data */
struct acp_dmic_dai_pdata {

	struct acp_dmic_dai_config *priv_data;
	struct k_spinlock lock;
};

#define dai_get_drvdata(dai)       dai->priv_data
#define dai_base(dai)              dai->plat_data.base
#define dai_set_drvdata(dai, data) (dai->priv_data = data)
#endif /* ZEPHYR_DRIVERS_DAI_AMD_ACP_DMIC_DAI_H_ */
