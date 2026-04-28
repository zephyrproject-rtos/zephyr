/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INFINEON_KCONFIG_H__
#define INFINEON_KCONFIG_H__

/*
 * These are mappings of Kconfig options enabling Infineon SOCs and particular
 * peripheral instances to the corresponding symbols used inside of Infineon code.
 */

#if defined(CONFIG_SOC_PSE846GPS2DBZC4A)

#ifndef PSE846GPS2DBZC4A
#define PSE846GPS2DBZC4A
#endif /* PSE846GPS2DBZC4A */

#if defined(CONFIG_CPU_CORTEX_M33)

#if defined(CONFIG_TRUSTED_EXECUTION_SECURE)
#define COMPONENT_SECURE_DEVICE
#endif /* CONFIG_TRUSTED_EXECUTION_SECURE* */

#ifndef COMPONENT_CM33
#define COMPONENT_CM33
#endif /* COMPONENT_CM33 */
#ifndef CORE_NAME_CM33_0
#define CORE_NAME_CM33_0
#endif /* CORE_NAME_CM33_0 */

#elif defined(CONFIG_CPU_CORTEX_M55)

#ifndef COMPONENT_CM55
#define COMPONENT_CM55
#endif /* COMPONENT_CM55 */
#ifndef CORE_NAME_CM55_0
#define CORE_NAME_CM55_0
#endif /* CORE_NAME_CM55_0 */

#endif /* CONFIG_CPU_CORTEXT_M33* */
#endif /* CONFIG_SOC_PSE846GPS2DBZC4A* */

#if defined(CONFIG_SOC_SERIES_PSOC4100TP)

/*
 * Map Zephyr Kconfig selections for PSOC4 SoCs to the macros expected by the
 * ModusToolbox PDL headers. We define both the bare part macro and the variant
 * with a trailing underscore to retain compatibility with existing build logic.
 */

#if defined(CONFIG_SOC_CY8C4146LQI_T403)
#ifndef CY8C4146LQI_T403
#define CY8C4146LQI_T403
#endif
#ifndef CY8C4146LQI_T403_
#define CY8C4146LQI_T403_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AXI_T403)
#ifndef CY8C4146AXI_T403
#define CY8C4146AXI_T403
#endif
#ifndef CY8C4146AXI_T403_
#define CY8C4146AXI_T403_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AZI_T403)
#ifndef CY8C4146AZI_T403
#define CY8C4146AZI_T403
#endif
#ifndef CY8C4146AZI_T403_
#define CY8C4146AZI_T403_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AZI_T405)
#ifndef CY8C4146AZI_T405
#define CY8C4146AZI_T405
#endif
#ifndef CY8C4146AZI_T405_
#define CY8C4146AZI_T405_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146LQI_T413)
#ifndef CY8C4146LQI_T413
#define CY8C4146LQI_T413
#endif
#ifndef CY8C4146LQI_T413_
#define CY8C4146LQI_T413_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AXI_T413)
#ifndef CY8C4146AXI_T413
#define CY8C4146AXI_T413
#endif
#ifndef CY8C4146AXI_T413_
#define CY8C4146AXI_T413_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AZI_T413)
#ifndef CY8C4146AZI_T413
#define CY8C4146AZI_T413
#endif
#ifndef CY8C4146AZI_T413_
#define CY8C4146AZI_T413_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AZI_T415)
#ifndef CY8C4146AZI_T415
#define CY8C4146AZI_T415
#endif
#ifndef CY8C4146AZI_T415_
#define CY8C4146AZI_T415_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146LQI_T453)
#ifndef CY8C4146LQI_T453
#define CY8C4146LQI_T453
#endif
#ifndef CY8C4146LQI_T453_
#define CY8C4146LQI_T453_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AXI_T453)
#ifndef CY8C4146AXI_T453
#define CY8C4146AXI_T453
#endif
#ifndef CY8C4146AXI_T453_
#define CY8C4146AXI_T453_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AZI_T453)
#ifndef CY8C4146AZI_T453
#define CY8C4146AZI_T453
#endif
#ifndef CY8C4146AZI_T453_
#define CY8C4146AZI_T453_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AZI_T455)
#ifndef CY8C4146AZI_T455
#define CY8C4146AZI_T455
#endif
#ifndef CY8C4146AZI_T455_
#define CY8C4146AZI_T455_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147LQI_T403)
#ifndef CY8C4147LQI_T403
#define CY8C4147LQI_T403
#endif
#ifndef CY8C4147LQI_T403_
#define CY8C4147LQI_T403_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AXI_T403)
#ifndef CY8C4147AXI_T403
#define CY8C4147AXI_T403
#endif
#ifndef CY8C4147AXI_T403_
#define CY8C4147AXI_T403_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZI_T403)
#ifndef CY8C4147AZI_T403
#define CY8C4147AZI_T403
#endif
#ifndef CY8C4147AZI_T403_
#define CY8C4147AZI_T403_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZI_T405)
#ifndef CY8C4147AZI_T405
#define CY8C4147AZI_T405
#endif
#ifndef CY8C4147AZI_T405_
#define CY8C4147AZI_T405_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147LQI_T413)
#ifndef CY8C4147LQI_T413
#define CY8C4147LQI_T413
#endif
#ifndef CY8C4147LQI_T413_
#define CY8C4147LQI_T413_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AXI_T413)
#ifndef CY8C4147AXI_T413
#define CY8C4147AXI_T413
#endif
#ifndef CY8C4147AXI_T413_
#define CY8C4147AXI_T413_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZI_T413)
#ifndef CY8C4147AZI_T413
#define CY8C4147AZI_T413
#endif
#ifndef CY8C4147AZI_T413_
#define CY8C4147AZI_T413_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZI_T415)
#ifndef CY8C4147AZI_T415
#define CY8C4147AZI_T415
#endif
#ifndef CY8C4147AZI_T415_
#define CY8C4147AZI_T415_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147LQI_T453)
#ifndef CY8C4147LQI_T453
#define CY8C4147LQI_T453
#endif
#ifndef CY8C4147LQI_T453_
#define CY8C4147LQI_T453_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AXI_T453)
#ifndef CY8C4147AXI_T453
#define CY8C4147AXI_T453
#endif
#ifndef CY8C4147AXI_T453_
#define CY8C4147AXI_T453_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZI_T453)
#ifndef CY8C4147AZI_T453
#define CY8C4147AZI_T453
#endif
#ifndef CY8C4147AZI_T453_
#define CY8C4147AZI_T453_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZI_T455)
#ifndef CY8C4147AZI_T455
#define CY8C4147AZI_T455
#endif
#ifndef CY8C4147AZI_T455_
#define CY8C4147AZI_T455_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147LQI_T463)
#ifndef CY8C4147LQI_T463
#define CY8C4147LQI_T463
#endif
#ifndef CY8C4147LQI_T463_
#define CY8C4147LQI_T463_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AXI_T463)
#ifndef CY8C4147AXI_T463
#define CY8C4147AXI_T463
#endif
#ifndef CY8C4147AXI_T463_
#define CY8C4147AXI_T463_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZI_T463)
#ifndef CY8C4147AZI_T463
#define CY8C4147AZI_T463
#endif
#ifndef CY8C4147AZI_T463_
#define CY8C4147AZI_T463_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZI_T465)
#ifndef CY8C4147AZI_T465
#define CY8C4147AZI_T465
#endif
#ifndef CY8C4147AZI_T465_
#define CY8C4147AZI_T465_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147LQI_T473)
#ifndef CY8C4147LQI_T473
#define CY8C4147LQI_T473
#endif
#ifndef CY8C4147LQI_T473_
#define CY8C4147LQI_T473_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AXI_T473)
#ifndef CY8C4147AXI_T473
#define CY8C4147AXI_T473
#endif
#ifndef CY8C4147AXI_T473_
#define CY8C4147AXI_T473_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZI_T473)
#ifndef CY8C4147AZI_T473
#define CY8C4147AZI_T473
#endif
#ifndef CY8C4147AZI_T473_
#define CY8C4147AZI_T473_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZI_T475)
#ifndef CY8C4147AZI_T475
#define CY8C4147AZI_T475
#endif
#ifndef CY8C4147AZI_T475_
#define CY8C4147AZI_T475_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147LQQ_T493)
#ifndef CY8C4147LQQ_T493
#define CY8C4147LQQ_T493
#endif
#ifndef CY8C4147LQQ_T493_
#define CY8C4147LQQ_T493_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AXQ_T493)
#ifndef CY8C4147AXQ_T493
#define CY8C4147AXQ_T493
#endif
#ifndef CY8C4147AXQ_T493_
#define CY8C4147AXQ_T493_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZQ_T493)
#ifndef CY8C4147AZQ_T493
#define CY8C4147AZQ_T493
#endif
#ifndef CY8C4147AZQ_T493_
#define CY8C4147AZQ_T493_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZQ_T495)
#ifndef CY8C4147AZQ_T495
#define CY8C4147AZQ_T495
#endif
#ifndef CY8C4147AZQ_T495_
#define CY8C4147AZQ_T495_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AZQ_T413)
#ifndef CY8C4146AZQ_T413
#define CY8C4146AZQ_T413
#endif
#ifndef CY8C4146AZQ_T413_
#define CY8C4146AZQ_T413_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4146AZQ_T453)
#ifndef CY8C4146AZQ_T453
#define CY8C4146AZQ_T453
#endif
#ifndef CY8C4146AZQ_T453_
#define CY8C4146AZQ_T453_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZQ_T415)
#ifndef CY8C4147AZQ_T415
#define CY8C4147AZQ_T415
#endif
#ifndef CY8C4147AZQ_T415_
#define CY8C4147AZQ_T415_
#endif
#endif
#if defined(CONFIG_SOC_CY8C4147AZQ_T455)
#ifndef CY8C4147AZQ_T455
#define CY8C4147AZQ_T455
#endif
#ifndef CY8C4147AZQ_T455_
#define CY8C4147AZQ_T455_
#endif
#endif

#endif /* CONFIG_SOC_SERIES_PSOC4100TP */

#if defined(CONFIG_SOC_SERIES_PSOC4100SMAX)

#if defined(CONFIG_SOC_CY8C4149AZI_S598)
#ifndef CY8C4149AZI_S598
#define CY8C4149AZI_S598
#endif
#ifndef CY8C4149AZI_S598_
#define CY8C4149AZI_S598_
#endif
#endif

#endif /* CONFIG_SOC_SERIES_PSOC4100SMAX */

#endif /* INFINEON_KCONFIG_H__ */
