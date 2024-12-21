/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This configuration header is used by the HAL driver device_init_dcdc from hal_silabs,
 * invoked through the soc_early_init hook. DeviceTree options are converted to config macros
 * expected by the HAL driver.
 */

#ifndef SL_DEVICE_INIT_DCDC_CONFIG_H
#define SL_DEVICE_INIT_DCDC_CONFIG_H

#include <zephyr/devicetree.h>

#if DT_HAS_COMPAT_STATUS_OKAY(silabs_series2_dcdc)

#define DCDC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(silabs_series2_dcdc)

#define SL_DEVICE_INIT_DCDC_ENABLE DT_PROP(DCDC_NODE, regulator_boot_on)
#define SL_DEVICE_INIT_DCDC_BYPASS DT_PROP(DCDC_NODE, regulator_allow_bypass)

#define SL_DEVICE_INIT_DCDC_TYPE DT_PROP_OR(DCDC_NODE, regulator_initial_mode, 0)

#define SL_DEVICE_INIT_DCDC_BOOST_OUTPUT DT_ENUM_IDX(DCDC_NODE, regulator_init_microvolt)

#define SL_DEVICE_INIT_DCDC_PFMX_IPKVAL_OVERRIDE                                                   \
	DT_NODE_HAS_PROP(DCDC_NODE, silabs_pfmx_peak_current_milliamp)

#define SL_DEVICE_INIT_DCDC_PFMX_IPKVAL                                                            \
	(DT_ENUM_IDX(DCDC_NODE, silabs_pfmx_peak_current_milliamp) + 3)

#endif /* DT_HAS_COMPAT_STATUS_OKAY */

#endif /* SL_DEVICE_INIT_DCDC_CONFIG_H */
