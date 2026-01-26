/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MDK_CONFIG_H__
#define MDK_CONFIG_H__

#include <zephyr/devicetree.h>

#if defined(CONFIG_SOC_SERIES_NRF51) && !defined(NRF51)
#define NRF51
#endif

#if defined(CONFIG_SOC_NRF51822_QFAA) && !defined(NRF51422_XXAA)
#define NRF51422_XXAA
#endif

#if defined(CONFIG_SOC_NRF51822_QFAB) && !defined(NRF51422_XXAB)
#define NRF51422_XXAB
#endif

#if defined(CONFIG_SOC_NRF51822_QFAC) && !defined(NRF51422_XXAC)
#define NRF51422_XXAC
#endif

#if defined(CONFIG_SOC_NRF52805) && !defined(NRF52805_XXAA)
#define NRF52805_XXAA
#endif

#if defined(CONFIG_SOC_NRF52810) && !defined(NRF52810_XXAA)
#define NRF52810_XXAA
#endif

#if defined(CONFIG_SOC_NRF52811) && !defined(NRF52811_XXAA)
#define NRF52811_XXAA
#endif

#if defined(CONFIG_SOC_NRF52820) && !defined(NRF52820_XXAA)
#define NRF52820_XXAA
#endif

#if defined(CONFIG_SOC_NRF52832) && !defined(NRF52832_XXAA)
#define NRF52832_XXAA
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF52833) && !defined(NRF52833_XXAA)
#define NRF52833_XXAA
#endif

#if defined(CONFIG_SOC_NRF52840) && !defined(NRF52840_XXAA)
#define NRF52840_XXAA
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUAPP) && !defined(NRF5340_XXAA_APPLICATION)
#define NRF5340_XXAA_APPLICATION
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET) && !defined(NRF5340_XXAA_NETWORK)
#define NRF5340_XXAA_NETWORK
#endif

#if defined(CONFIG_SOC_NRF54H20) && !defined(NRF54H20_XXAA)
#define NRF54H20_XXAA
#endif

#if defined(CONFIG_SOC_NRF54H20_CPUAPP) && !defined(NRF_APPLICATION)
#define NRF_APPLICATION
#endif

#if defined(CONFIG_SOC_NRF54H20_CPURAD) && !defined(NRF_RADIOCORE)
#define NRF_RADIOCORE
#endif

#if defined(CONFIG_SOC_NRF54H20_CPUPPR) && !defined(NRF_PPR)
#define NRF_PPR
#endif

#if defined(CONFIG_SOC_NRF54H20_CPUFLPR) && !defined(NRF_FLPR)
#define NRF54H20_XXAA
#define NRF_FLPR
#endif

#if defined(CONFIG_SOC_NRF54L05) && !defined(NRF54L05_XXAA) && !defined(DEVELOP_IN_NRF54L15)
#define NRF54L05_XXAA
#define DEVELOP_IN_NRF54L15
#endif

#if defined(CONFIG_SOC_NRF54L05_CPUAPP) && !defined(NRF_APPLICATION)
#define NRF_APPLICATION
#endif

#if defined(CONFIG_SOC_NRF54L05_CPUFLPR) && !defined(NRF_FLPR)
#define NRF_FLPR
#endif

#if defined(CONFIG_SOC_NRF54L10) && !defined(NRF54L10_XXAA) && !defined(DEVELOP_IN_NRF54L15)
#define NRF54L10_XXAA
#define DEVELOP_IN_NRF54L15
#endif

#if defined(CONFIG_SOC_NRF54L10_CPUAPP) && !defined(NRF_APPLICATION)
#define NRF_APPLICATION
#endif

#if defined(CONFIG_SOC_NRF54L10_CPUFLPR) && !defined(NRF_FLPR)
#define NRF_FLPR
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF54L15) && !defined(NRF54L15_XXAA)
#define NRF54L15_XXAA
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF54L15_CPUAPP) && !defined(NRF_APPLICATION)
#define NRF_APPLICATION
#endif

#if defined(CONFIG_SOC_NRF54L15_CPUFLPR) && !defined(NRF_FLPR)
#define NRF_FLPR
#endif

#if defined(CONFIG_SOC_NRF54LM20A_ENGA) && !defined(NRF54LM20A_ENGA_XXAA)
#define NRF54LM20A_ENGA_XXAA
#endif

#if defined(CONFIG_SOC_NRF54LM20A_ENGA_CPUAPP) && !defined(NRF_APPLICATION)
#define NRF_APPLICATION
#endif

#if defined(CONFIG_SOC_NRF54LM20A_ENGA_CPUFLPR) && !defined(NRF_FLPR)
#define NRF_FLPR
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF54LM20A) && !defined(NRF54LM20A_ENGA_XXAA)
#define NRF54LM20A_ENGA_XXAA
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF54LM20A_CPUAPP) && !defined(NRF_APPLICATION)
#define NRF_APPLICATION
#endif

#if defined(CONFIG_SOC_NRF7120_ENGA) && !defined(NRF7120_ENGA_XXAA)
#define NRF7120_ENGA_XXAA
#endif

#if defined(CONFIG_SOC_NRF7120_ENGA_CPUAPP) && !defined(NRF_APPLICATION)
#define NRF_APPLICATION
#endif

#if defined(CONFIG_SOC_NRF7120_ENGA_CPUFLPR) && !defined(NRF_FLPR)
#define NRF_FLPR
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF7120_ENGA) && !defined(NRF7120_ENGA_XXAA)
#define NRF7120_ENGA_XXAA
#endif

#if defined(CONFIG_SOC_COMPATIBLE_NRF7120_ENGA_CPUAPP) && !defined(NRF_APPLICATION)
#define NRF_APPLICATION
#endif

#if defined(CONFIG_SOC_NRF9120) && !defined(NRF9120_XXAA)
#define NRF9120_XXAA
#endif

#if defined(CONFIG_SOC_NRF9160) && !defined(NRF9160_XXAA)
#define NRF9160_XXAA
#endif

#if defined(CONFIG_SOC_NRF9230_ENGB) && !defined(NRF9230_ENGB_XXAA)
#define NRF9230_ENGB_XXAA
#endif

#if defined(CONFIG_SOC_NRF9230_ENGB_CPUAPP) && !defined(NRF_APPLICATION)
#define NRF_APPLICATION
#endif

#if defined(CONFIG_SOC_NRF9230_ENGB_CPURAD) && !defined(NRF_RADIOCORE)
#define NRF_RADIOCORE
#endif

#if defined(CONFIG_SOC_NRF9230_ENGB_CPUPPR) && !defined(NRF_PPR)
#define NRF_PPR
#endif

#if defined(CONFIG_NRF_APPROTECT_LOCK)
#define ENABLE_APPROTECT
#endif

#if defined(CONFIG_NRF_APPROTECT_USER_HANDLING)
#define ENABLE_APPROTECT_USER_HANDLING
#define ENABLE_AUTHENTICATED_APPROTECT
#endif

#if defined(CONFIG_NRF_SECURE_APPROTECT_LOCK)
#define ENABLE_SECURE_APPROTECT
#define ENABLE_SECUREAPPROTECT
#endif

#if defined(CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING)
#define ENABLE_SECURE_APPROTECT_USER_HANDLING
#define ENABLE_AUTHENTICATED_SECUREAPPROTECT
#endif

#if defined(CONFIG_NRF_TRACE_PORT)
#define ENABLE_TRACE
#endif

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#define NRF_SKIP_FICR_NS_COPY_TO_RAM
#endif

#if defined(CONFIG_SOC_SERIES_NRF91)
#define NRF_SKIP_FICR_NS_COPY_TO_RAM
#endif

/* Connect Kconfig compilation option for Non-Secure software with option required by MDK/nrfx */

#if defined(CONFIG_ARM_NONSECURE_FIRMWARE)
#define NRF_TRUSTZONE_NONSECURE
#endif

#if defined(CONFIG_LOG_BACKEND_SWO)
#define ENABLE_SWO
#endif

/*
 * Inject HAL "NFCT_PINS_AS_GPIOS" definition if user requests to
 * configure the NFCT pins as GPIOS. Do the same with "CONFIG_GPIO_AS_PINRESET"
 * to configure the reset GPIO as nRESET. This way, the HAL will take care of
 * doing the proper configuration sequence during system init.
 */

#if DT_NODE_EXISTS(DT_NODELABEL(uicr))

#if DT_PROP(DT_NODELABEL(uicr), nfct_pins_as_gpios)
#define NRF_CONFIG_NFCT_PINS_AS_GPIOS
#endif

#if DT_PROP(DT_NODELABEL(uicr), gpio_as_nreset)
#define NRF_CONFIG_GPIO_AS_PINRESET
#endif

#endif /* DT_NODE_EXISTS(DT_NODELABEL(uicr)) */

#if DT_NODE_EXISTS(DT_NODELABEL(tampc)) && DT_PROP(DT_NODELABEL(tampc), swd_pins_as_gpios)
#define NRF_CONFIG_SWD_PINS_AS_GPIOS
#endif

#if defined(CONFIG_SOC_NRF54L_CPUAPP_COMMON)
#define NRF_CONFIG_CPU_FREQ_MHZ (DT_PROP(DT_PATH(clocks, hfpll), clock_frequency) / 1000000)
#endif

#if defined(CONFIG_SOC_NRF7120_ENGA_CPUAPP) && !defined(NRF_CONFIG_CPU_FREQ_MHZ)
#define NRF_CONFIG_CPU_FREQ_MHZ (DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) / 1000000)
#endif

#if defined(CONFIG_NRF_SKIP_CLOCK_CONFIG)
#define NRF_SKIP_CLOCK_CONFIGURATION
#endif

#if defined(CONFIG_SOC_NRF54LX_DISABLE_FICR_TRIMCNF)
#define NRF_DISABLE_FICR_TRIMCNF
#endif

#if defined(CONFIG_SOC_NRF54LX_SKIP_GLITCHDETECTOR_DISABLE)
#define NRF_SKIP_GLITCHDETECTOR_DISABLE
#endif

#if !defined(CONFIG_SOC_NRF54L_ANOMALY_56_WORKAROUND)
#define NRF54L_CONFIGURATION_56_ENABLE 0
#endif

#endif /* MDK_CONFIG_H__ */
