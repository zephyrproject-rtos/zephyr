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

#endif /* INFINEON_KCONFIG_H__ */
