/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SL_DEVICE_INIT_DCDC_CONFIG_H
#define SL_DEVICE_INIT_DCDC_CONFIG_H

#include <zephyr/devicetree.h>
#include <soc.h>

#if DT_HAS_COMPAT_STATUS_OKAY(silabs_series2_dcdc)

#define DCDC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(silabs_series2_dcdc)

#define SL_DEVICE_INIT_DCDC_ENABLE DT_PROP(DCDC_NODE, regulator_boot_on)
#define SL_DEVICE_INIT_DCDC_BYPASS DT_PROP(DCDC_NODE, regulator_allow_bypass)

#define SL_DEVICE_INIT_DCDC_TYPE DT_PROP_OR(DCDC_NODE, regulator_initial_mode, 0)

#define SL_DEVICE_INIT_DCDC_BOOST_OUTPUT                                                           \
	(DT_ENUM_HAS_VALUE(DCDC_NODE, regulator_init_microvolt, 1800000)                           \
		 ? emuDcdcBoostOutputVoltage_1v8                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, regulator_init_microvolt, 1900000)                         \
		 ? emuDcdcBoostOutputVoltage_1v9                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, regulator_init_microvolt, 2000000)                         \
		 ? emuDcdcBoostOutputVoltage_2v0                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, regulator_init_microvolt, 2100000)                         \
		 ? emuDcdcBoostOutputVoltage_2v1                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, regulator_init_microvolt, 2200000)                         \
		 ? emuDcdcBoostOutputVoltage_2v2                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, regulator_init_microvolt, 2300000)                         \
		 ? emuDcdcBoostOutputVoltage_2v3                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, regulator_init_microvolt, 2400000)                         \
		 ? emuDcdcBoostOutputVoltage_2v4                                                   \
		 : emuDcdcBoostOutputVoltage_1v8)

#define SL_DEVICE_INIT_DCDC_PFMX_IPKVAL_OVERRIDE                                                   \
	DT_NODE_HAS_PROP(DCDC_NODE, silabs_pfmx_peak_current_microamp)

#define SL_DEVICE_INIT_DCDC_PFMX_IPKVAL                                                            \
	(DT_ENUM_HAS_VALUE(DCDC_NODE, silabs_pfmx_peak_current_microamp, 50000)                    \
		 ? DCDC_PFMXCTRL_IPKVAL_LOAD50MA                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, silabs_pfmx_peak_current_microamp, 65000)                  \
		 ? DCDC_PFMXCTRL_IPKVAL_LOAD65MA                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, silabs_pfmx_peak_current_microamp, 73000)                  \
		 ? DCDC_PFMXCTRL_IPKVAL_LOAD73MA                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, silabs_pfmx_peak_current_microamp, 80000)                  \
		 ? DCDC_PFMXCTRL_IPKVAL_LOAD80MA                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, silabs_pfmx_peak_current_microamp, 86000)                  \
		 ? DCDC_PFMXCTRL_IPKVAL_LOAD86MA                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, silabs_pfmx_peak_current_microamp, 93000)                  \
		 ? DCDC_PFMXCTRL_IPKVAL_LOAD93MA                                                   \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, silabs_pfmx_peak_current_microamp, 100000)                 \
		 ? DCDC_PFMXCTRL_IPKVAL_LOAD100MA                                                  \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, silabs_pfmx_peak_current_microamp, 106000)                 \
		 ? DCDC_PFMXCTRL_IPKVAL_LOAD106MA                                                  \
	 : DT_ENUM_HAS_VALUE(DCDC_NODE, silabs_pfmx_peak_current_microamp, 113000)                 \
		 ? DCDC_PFMXCTRL_IPKVAL_LOAD113MA                                                  \
		 : DCDC_PFMXCTRL_IPKVAL_LOAD120MA)

#endif /* DT_HAS_COMPAT_STATUS_OKAY */

#endif /* SL_DEVICE_INIT_DCDC_CONFIG_H */
