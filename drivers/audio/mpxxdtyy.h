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
	((const struct mpxxdtyy_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct mpxxdtyy_data *const)(dev)->data)

struct mpxxdtyy_data {
	const struct device *comm_master;
	enum dmic_state		state;
	TPDMFilter_InitStruct	pdm_filter[2];
	size_t			pcm_mem_size;
	struct k_mem_slab	*pcm_mem_slab;
};

uint16_t sw_filter_lib_init(const struct device *dev, struct dmic_cfg *cfg);
int sw_filter_lib_run(TPDMFilter_InitStruct *pdm_filter,
		      void *pdm_block, void *pcm_block,
		      size_t pdm_size, size_t pcm_size);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2s)
int mpxxdtyy_i2s_read(const struct device *dev, uint8_t stream, void **buffer,
		      size_t *size, int32_t timeout);
int mpxxdtyy_i2s_trigger(const struct device *dev, enum dmic_trigger cmd);
int mpxxdtyy_i2s_configure(const struct device *dev, struct dmic_cfg *cfg);
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2s) */

#ifdef __cplusplus
}
#endif

#endif /* MPXXDTYY_H */
