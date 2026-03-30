/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This configuration header is used by the HAL driver sl_rail_util_pa_tables from hal_silabs,
 * invoked through drivers that use radio functionality, such as the hci_silabs_efr32 driver.
 * DeviceTree and Kconfig options are converted to config macros expected by the HAL driver.
 */

#ifndef SL_RAIL_UTIL_PA_TABLES_CONFIG_H
#define SL_RAIL_UTIL_PA_TABLES_CONFIG_H

#include <zephyr/devicetree.h>

#ifdef CONFIG_SOC_SILABS_XG21

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, hp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_20dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, mp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_10dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, lp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_0dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, auto)
#if DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm) > 10
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_automode_0_10_20dbm.h"
#else
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_automode_0_10dbm.h"
#endif
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, highest)
#if DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm) > 10
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_20dbm.h"
#else
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_10dbm.h"
#endif
#else
#error "Unknown 2.4 GHz PA configuration"
#endif

#endif /* CONFIG_SOC_SILABS_XG21 */

#ifdef CONFIG_SOC_SILABS_XG22

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, hp) ||                                       \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, highest)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_6dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, lp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_0dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, auto)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_automode_0_6dbm.h"
#else
#error "Unknown 2.4 GHz PA configuration"
#endif

#endif /* CONFIG_SOC_SILABS_XG22 */

#ifdef CONFIG_SOC_SILABS_XG23

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_subghz, hp) ||                                       \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_subghz, highest)
#include STRINGIFY(CONCAT(CONCAT(sl_rail_util_pa_dbm_powersetting_mapping_table_,                  \
				 DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm)), dbm_HP.h))
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_subghz, mp)
#include STRINGIFY(CONCAT(CONCAT(sl_rail_util_pa_dbm_powersetting_mapping_table_,                  \
				 DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm)), dbm_MP.h))
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_subghz, lp)
#include STRINGIFY(CONCAT(CONCAT(sl_rail_util_pa_dbm_powersetting_mapping_table_,                  \
				 DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm)), dbm_LP.h))
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_subghz, llp)
#include STRINGIFY(CONCAT(CONCAT(sl_rail_util_pa_dbm_powersetting_mapping_table_,                  \
				 DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm)), dbm_LLP.h))
#else
#error "Unknown Sub-GHz PA configuration"
#endif

#endif /* CONFIG_SOC_SILABS_XG23 */

#ifdef CONFIG_SOC_SILABS_XG24

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, hp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_20dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, mp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_10dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, lp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_0dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, auto)
#if DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm) > 10
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_20dbm.h"
#else
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_automode_0_10dbm.h"
#endif
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, highest)
#if DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm) > 10
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_20dbm.h"
#else
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_10dbm.h"
#endif
#else
#error "Unknown 2.4 GHz PA configuration"
#endif

#endif /* CONFIG_SOC_SILABS_XG24 */

#ifdef CONFIG_SOC_SILABS_XG26

#ifdef CONFIG_SOC_EFR32MG26B510F3200IL136

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, mp) ||                                       \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, highest)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_bga_10dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, lp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_bga_0dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, auto)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_bga_automode_0_10dbm.h"
#else
#error "Unknown 2.4 GHz PA configuration"
#endif

#else /* CONFIG_SOC_EFR32MG26B510F3200IL136 */

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, hp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_20dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, mp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_10dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, lp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_0dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, auto)
#if DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm) > 10
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_20dbm.h"
#else
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_automode_0_10dbm.h"
#endif
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, highest)
#if DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm) > 10
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_20dbm.h"
#else
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_10dbm.h"
#endif
#else
#error "Unknown 2.4 GHz PA configuration"
#endif

#endif /* CONFIG_SOC_EFR32MG26B510F3200IL136 */

#endif /* CONFIG_SOC_SILABS_XG26 */

#ifdef CONFIG_SOC_SILABS_XG27

#if DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm) > 4

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, hp) ||                                       \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, highest)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_qfn_8dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, lp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_qfn_0dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, auto)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_qfn_automode_0_8dbm.h"
#else
#error "Unknown 2.4 GHz PA configuration"
#endif

#else

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, hp) ||                                       \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, highest)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_csp_4dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, lp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_csp_0dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, auto)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_csp_automode_0_4dbm.h"
#else
#error "Unknown 2.4 GHz PA configuration"
#endif

#endif

#endif /* CONFIG_SOC_SILABS_XG27 */

#ifdef CONFIG_SOC_SILABS_XG28

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_subghz, hp) ||                                       \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_subghz, highest)
#include STRINGIFY(CONCAT(CONCAT(sl_rail_util_pa_dbm_powersetting_mapping_table_,                  \
				 DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm)), dbm_915M_HP.h))
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_subghz, mp)
#include STRINGIFY(CONCAT(CONCAT(sl_rail_util_pa_dbm_powersetting_mapping_table_,                  \
				 DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm)), dbm_915M_MP.h))
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_subghz, lp)
#include STRINGIFY(CONCAT(CONCAT(sl_rail_util_pa_dbm_powersetting_mapping_table_,                  \
				 DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm)), dbm_915M_LP.h))
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_subghz, llp)
#include STRINGIFY(CONCAT(CONCAT(sl_rail_util_pa_dbm_powersetting_mapping_table_,                  \
				 DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm)), dbm_915M_LLP.h))
#else
#error "Unknown Sub-GHz PA configuration"
#endif

#endif /* CONFIG_SOC_SILABS_XG28 */

#ifdef CONFIG_SOC_SILABS_XG29

#if DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm) > 4

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, hp) ||                                       \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, highest)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_qfn_8dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, lp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_qfn_0dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, auto)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_qfn_automode_0_8dbm.h"
#else
#error "Unknown 2.4 GHz PA configuration"
#endif

#else

#if DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, hp) ||                                       \
	DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, highest)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_csp_8dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, lp)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_csp_0dbm.h"
#elif DT_ENUM_HAS_VALUE(DT_NODELABEL(radio), pa_2p4ghz, auto)
#include "sl_rail_util_pa_dbm_powersetting_mapping_table_csp_automode_0_8dbm.h"
#else
#error "Unknown 2.4 GHz PA configuration"
#endif

#endif

#endif /* CONFIG_SOC_SILABS_XG29 */

#endif /* SL_RAIL_UTIL_PA_TABLES_CONFIG_H */
