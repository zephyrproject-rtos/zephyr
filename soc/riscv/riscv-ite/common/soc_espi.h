/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ITE_IT8XXX2_SOC_ESPI_H_
#define _ITE_IT8XXX2_SOC_ESPI_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESPI_IT8XXX2_SOC_DEV DEVICE_DT_GET(DT_NODELABEL(espi0))

/**
 * @brief eSPI input pad gating
 *
 * @param dev pointer to eSPI device
 * @param enable/disable eSPI pad
 */
void espi_it8xxx2_enable_pad_ctrl(const struct device *dev, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* _ITE_IT8XXX2_SOC_ESPI_H_ */
