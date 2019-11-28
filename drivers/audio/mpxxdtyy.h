/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPXXDTYY_H
#define MPXXDTYY_H

#include <audio/dmic.h>
#include <zephyr.h>
#include <device.h>
#include "OpenPDMFilter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPXXDTYY_MIN_PDM_FREQ		1200000 /* 1.2MHz */
#define MPXXDTYY_MAX_PDM_FREQ		3250000 /* 3.25MHz */

#define DEV_CFG(dev) \
	((struct mpxxdtyy_config *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct mpxxdtyy_data *const)(dev)->driver_data)

struct mpxxdtyy_data {
	struct device		*comm_master;
	enum dmic_state		state;
	TPDMFilter_InitStruct	pdm_filter[2];
	size_t			pcm_mem_size;
	struct k_mem_slab	*pcm_mem_slab;
};

u16_t sw_filter_lib_init(struct device *dev, struct dmic_cfg *cfg);
int sw_filter_lib_run(TPDMFilter_InitStruct *pdm_filter,
		      void *pdm_block, void *pcm_block,
		      size_t pdm_size, size_t pcm_size);

#ifdef DT_ST_MPXXDTYY_BUS_I2S
int mpxxdtyy_i2s_read(struct device *dev, u8_t stream, void **buffer,
		      size_t *size, s32_t timeout);
int mpxxdtyy_i2s_trigger(struct device *dev, enum dmic_trigger cmd);
int mpxxdtyy_i2s_configure(struct device *dev, struct dmic_cfg *cfg);
#endif /* DT_ST_MPXXDTYY_BUS_I2S */

#ifdef __cplusplus
}
#endif

#endif /* MPXXDTYY_H */
