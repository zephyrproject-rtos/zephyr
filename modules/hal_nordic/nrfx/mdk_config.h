/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MDK_CONFIG_H__
#define MDK_CONFIG_H__

/*
 * These are mappings of Kconfig options describing build target
 * to the corresponding symbols used inside of nrfx-based MDK.
 */

#ifdef CONFIG_ARM_NONSECURE_FIRMWARE
#ifndef NRF_TRUSTZONE_NONSECURE
#define NRF_TRUSTZONE_NONSECURE 1
#endif
#endif

#ifdef CONFIG_SOC_SERIES_NRF51
#ifndef NRF51
#define NRF51 1
#endif
#endif

#ifdef CONFIG_SOC_NRF51822_QFAA
#ifndef NRF51422_XXAA
#define NRF51422_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_NRF51822_QFAB
#ifndef NRF51422_XXAB
#define NRF51422_XXAB 1
#endif
#endif

#ifdef CONFIG_SOC_NRF51822_QFAC
#ifndef NRF51422_XXAC
#define NRF51422_XXAC 1
#endif
#endif

#ifdef CONFIG_SOC_NRF52805
#ifndef NRF52805_XXAA
#define NRF52805_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_NRF52810
#ifndef NRF52810_XXAA
#define NRF52810_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_NRF52811
#ifndef NRF52811_XXAA
#define NRF52811_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_NRF52820
#ifndef NRF52820_XXAA
#define NRF52820_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_NRF52832
#ifndef NRF52832_XXAA
#define NRF52832_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_COMPATIBLE_NRF52833
#ifndef NRF52833_XXAA
#define NRF52833_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_NRF52840
#ifndef NRF52840_XXAA
#define NRF52840_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_NRF5340_CPUAPP
#ifndef NRF_SKIP_FICR_NS_COPY_TO_RAM
#define NRF_SKIP_FICR_NS_COPY_TO_RAM 1
#endif
#endif

#ifdef CONFIG_SOC_COMPATIBLE_NRF5340_CPUAPP
#ifndef NRF5340_XXAA_APPLICATION
#define NRF5340_XXAA_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_COMPATIBLE_NRF5340_CPUNET
#ifndef NRF5340_XXAA_NETWORK
#define NRF5340_XXAA_NETWORK 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54H20_CPUAPP
#ifndef NRF54H20_XXAA
#define NRF54H20_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54H20_CPURAD
#ifndef NRF54H20_XXAA
#define NRF54H20_XXAA 1
#endif
#ifndef NRF_RADIOCORE
#define NRF_RADIOCORE 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54H20_CPUPPR
#ifndef NRF54H20_XXAA
#define NRF54H20_XXAA 1
#endif
#ifndef NRF_PPR
#define NRF_PPR 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54H20_CPUFLPR
#ifndef NRF54H20_XXAA
#define NRF54H20_XXAA 1
#endif
#ifndef NRF_FLPR
#define NRF_FLPR 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54L05_CPUAPP
#ifndef NRF54L05_XXAA
#define NRF54L05_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54L05_CPUFLPR
#ifndef NRF54L05_XXAA
#define NRF54L05_XXAA 1
#endif
#ifndef NRF_FLPR
#define NRF_FLPR 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54L05_DEVELOP_IN_NRF54L15
#ifndef DEVELOP_IN_NRF54L15
#define DEVELOP_IN_NRF54L15 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54L10_CPUAPP
#ifndef NRF54L10_XXAA
#define NRF54L10_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54L10_CPUFLPR
#ifndef NRF54L10_XXAA
#define NRF54L10_XXAA 1
#endif
#ifndef NRF_FLPR
#define NRF_FLPR 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54L10_DEVELOP_IN_NRF54L15
#ifndef DEVELOP_IN_NRF54L15
#define DEVELOP_IN_NRF54L15 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54L15_CPUAPP
#ifndef NRF54L15_XXAA
#define NRF54L15_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54L15_CPUFLPR
#ifndef NRF54L15_XXAA
#define NRF54L15_XXAA 1
#endif
#ifndef NRF_FLPR
#define NRF_FLPR 1
#endif
#endif

#ifdef CONFIG_SOC_COMPATIBLE_NRF54L15
#ifndef NRF54L15_XXAA
#define NRF54L15_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_COMPATIBLE_NRF54L15_CPUAPP
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LM20A_CPUAPP
#ifndef NRF54LM20A_XXAA
#define NRF54LM20A_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LM20A_CPUFLPR
#ifndef NRF54LM20A_XXAA
#define NRF54LM20A_XXAA 1
#endif
#ifndef NRF_FLPR
#define NRF_FLPR 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LM20A_DEVELOP_IN_NRF54LM20B
#ifndef DEVELOP_IN_NRF54LM20B
#define DEVELOP_IN_NRF54LM20B 1
#endif
#endif

#ifdef CONFIG_SOC_COMPATIBLE_NRF54LM20A
#ifndef NRF54LM20A_XXAA
#define NRF54LM20A_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_COMPATIBLE_NRF54LM20A_CPUAPP
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LM20B_CPUAPP
#ifndef NRF54LM20B_XXAA
#define NRF54LM20B_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LM20B_CPUFLPR
#ifndef NRF54LM20A_XXAA
#define NRF54LM20A_XXAA 1
#endif
#ifndef NRF_FLPR
#define NRF_FLPR 1
#endif
#endif

#ifdef CONFIG_SOC_NRF7120_ENGA_CPUAPP
#ifndef NRF7120_ENGA_XXAA
#define NRF7120_ENGA_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF7120_ENGA_CPUFLPR
#ifndef NRF7120_ENGA_XXAA
#define NRF7120_ENGA_XXAA 1
#endif
#ifndef NRF_FLPR
#define NRF_FLPR 1
#endif
#endif

#ifdef CONFIG_SOC_COMPATIBLE_NRF7120_ENGA
#ifndef NRF7120_ENGA_XXAA
#define NRF7120_ENGA_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_COMPATIBLE_NRF7120_ENGA_CPUAPP
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_SERIES_NRF91
#ifndef NRF_SKIP_FICR_NS_COPY_TO_RAM
#define NRF_SKIP_FICR_NS_COPY_TO_RAM 1
#endif
#endif

#ifdef CONFIG_SOC_NRF9120
#ifndef NRF9120_XXAA
#define NRF9120_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_NRF9160
#ifndef NRF9160_XXAA
#define NRF9160_XXAA 1
#endif
#endif

#ifdef CONFIG_SOC_NRF9230_ENGB_CPUAPP
#ifndef NRF9230_ENGB_XXAA
#define NRF9230_ENGB_XXAA 1
#endif
#ifndef NRF_APPLICATION
#define NRF_APPLICATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF9230_ENGB_CPURAD
#ifndef NRF9230_ENGB_XXAA
#define NRF9230_ENGB_XXAA 1
#endif
#ifndef NRF_RADIOCORE
#define NRF_RADIOCORE 1
#endif
#endif

#ifdef CONFIG_SOC_NRF9230_ENGB_CPUPPR
#ifndef NRF9230_ENGB_XXAA
#define NRF9230_ENGB_XXAA 1
#endif
#ifndef NRF_PPR
#define NRF_PPR 1
#endif
#endif

/*
 * These are mappings of Kconfig options enabling MDK-based features
 * to the corresponding symbols used inside of nrfx-based MDK.
 */

#ifdef CONFIG_NRF_APPROTECT_LOCK
#ifndef ENABLE_APPROTECT
#define ENABLE_APPROTECT 1
#endif
#endif

#ifdef CONFIG_NRF_APPROTECT_USER_HANDLING
#ifndef ENABLE_APPROTECT_USER_HANDLING
#define ENABLE_APPROTECT_USER_HANDLING 1
#endif
#ifndef ENABLE_AUTHENTICATED_APPROTECT
#define ENABLE_AUTHENTICATED_APPROTECT 1
#endif
#endif

#ifdef CONFIG_NRF_SECURE_APPROTECT_LOCK
#ifndef ENABLE_SECURE_APPROTECT
#define ENABLE_SECURE_APPROTECT 1
#endif
#ifndef ENABLE_SECUREAPPROTECT
#define ENABLE_SECUREAPPROTECT 1
#endif
#endif

#ifdef CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING
#ifndef ENABLE_SECURE_APPROTECT_USER_HANDLING
#define ENABLE_SECURE_APPROTECT_USER_HANDLING 1
#endif
#ifndef ENABLE_AUTHENTICATED_SECUREAPPROTECT
#define ENABLE_AUTHENTICATED_SECUREAPPROTECT 1
#endif
#endif

#ifdef CONFIG_NRF_TRACE_PORT
#ifndef ENABLE_TRACE
#define ENABLE_TRACE 1
#endif
#endif

#ifdef CONFIG_LOG_BACKEND_SWO
#ifndef ENABLE_SWO
#define ENABLE_SWO 1
#endif
#endif

#ifdef CONFIG_NRF_SKIP_CLOCK_CONFIG
#ifndef NRF_SKIP_CLOCK_CONFIGURATION
#define NRF_SKIP_CLOCK_CONFIGURATION 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LX_DISABLE_FICR_TRIMCNF
#ifndef NRF_DISABLE_FICR_TRIMCNF
#define NRF_DISABLE_FICR_TRIMCNF 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LX_SKIP_GLITCHDETECTOR_DISABLE
#ifndef NRF_SKIP_GLITCHDETECTOR_DISABLE
#define NRF_SKIP_GLITCHDETECTOR_DISABLE 1
#endif
#endif

#ifdef CONFIG_SOC_NRF54LX_SKIP_TAMPC_SETUP
#ifndef NRF_SKIP_TAMPC_SETUP
#define NRF_SKIP_TAMPC_SETUP 1
#endif
#endif

/*
 * These are mappings of Kconfig options enabling MDK-based anomaly workarounds
 * to the corresponding symbols used inside of nrfx-based MDK.
 */

#ifdef CONFIG_SOC_NRF54L_ANOMALY_56_WORKAROUND
#ifndef NRF54L_CONFIGURATION_56_ENABLE
#define NRF54L_CONFIGURATION_56_ENABLE 0
#endif
#endif

#ifdef CONFIG_NRF91_ANOMALY_36_WORKAROUND
#ifndef NRF91_ERRATA_36_ENABLE_WORKAROUND
#define NRF91_ERRATA_36_ENABLE_WORKAROUND 1
#endif
#endif

/*
 * These are mappings of devicetree properties configuring MDK-based features
 * to the corresponding symbols used inside of nrfx-based MDK.
 */

#include <zephyr/devicetree.h>

/*
 * Inject HAL "NFCT_PINS_AS_GPIOS" definition if user requests to
 * configure the NFCT pins as GPIOS. Do the same with "CONFIG_GPIO_AS_PINRESET"
 * to configure the reset GPIO as nRESET. This way, the HAL will take care of
 * doing the proper configuration sequence during system init.
 */

#if DT_PROP(DT_NODELABEL(uicr), nfct_pins_as_gpios) || \
	DT_PROP(DT_NODELABEL(nfct), nfct_pins_as_gpios)
#ifndef NRF_CONFIG_NFCT_PINS_AS_GPIOS
#define NRF_CONFIG_NFCT_PINS_AS_GPIOS 1
#endif
#endif

#if DT_PROP(DT_NODELABEL(uicr), gpio_as_nreset)
#ifndef NRF_CONFIG_GPIO_AS_PINRESET
#define NRF_CONFIG_GPIO_AS_PINRESET 1
#endif
#endif

#if DT_PROP(DT_NODELABEL(tampc), swd_pins_as_gpios)
#ifndef NRF_CONFIG_SWD_PINS_AS_GPIOS
#define NRF_CONFIG_SWD_PINS_AS_GPIOS 1
#endif
#endif

#if defined(CONFIG_SOC_NRF54L_CPUAPP_COMMON) && DT_PROP(DT_NODELABEL(hfpll), clock_frequency)
#ifndef NRF_CONFIG_CPU_FREQ_MHZ
#define NRF_CONFIG_CPU_FREQ_MHZ (DT_PROP(DT_NODELABEL(hfpll), clock_frequency) / 1000000)
#endif
#endif

#if defined(CONFIG_SOC_NRF7120_ENGA_CPUAPP) && DT_PROP(DT_NODELABEL(cpuapp), clock_frequency)
#ifndef NRF_CONFIG_CPU_FREQ_MHZ
#define NRF_CONFIG_CPU_FREQ_MHZ (DT_PROP(DT_NODELABEL(cpuapp), clock_frequency) / 1000000)
#endif
#endif

#endif /* MDK_CONFIG_H__ */
