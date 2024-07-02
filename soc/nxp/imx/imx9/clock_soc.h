/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM64_NXP_IMX9_CLOCK_SOC_H_
#define ZEPHYR_SOC_ARM64_NXP_IMX9_CLOCK_SOC_H_

#include "soc.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SOC_MIMX9596)
#include <zephyr/dt-bindings/clock/imx95_clk_scmi.h>
#define SCMI_CLK_SRC_NUM IMX95_CCM_NUM_CLK_SRC
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM64_NXP_IMX9_CLOCK_SOC_H_ */
