/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ADC_NPCX_V2T_H_
#define _ADC_NPCX_V2T_H_

#include <zephyr/device.h>

#ifdef CONFIG_ADC_V2T_NPCX

/**
 * @brief Set ADC V2T channels
 *
 *
 * @param dev       Pointer to the ADC device instance.
 * @param channels  Bit-mask indicating the channels to be configured as V2T
 *
 * @returns 0 on success, or a negative value if configuration fails.
 */
int adc_npcx_v2t_set_channels(const struct device *dev, uint32_t channels);

/**
 * @brief Get ADC V2T configured channels
 *
 *
 * @param dev  Pointer to the ADC device instance.
 *
 * @returns Configured v2t channel in bit-mask.
 *          0 for adc mode, 1 for v2t mode.
 */
uint32_t adc_npcx_v2t_get_channels(const struct device *dev);

#endif /* CONFIG_ADC_V2T_NPCX */
#endif /*_ADC_NPCX_V2T_H_ */
