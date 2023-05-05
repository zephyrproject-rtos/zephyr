/*
 * Copyright (c) 2019 - 2022, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_CONFIG_H__
#define NRFX_CONFIG_H__

#include <zephyr/devicetree.h>

/*
 * NRFX API version 2.10 flag.
 * When the flag is set NRFX API is compatible with the newest NRFX release.
 */
#define NRFX_CONFIG_API_VER_2_10 1

/*
 * These are mappings of Kconfig options enabling nrfx drivers and particular
 * peripheral instances to the corresponding symbols used inside of nrfx.
 * Please note that only subsets of these entries are used for particular SoCs
 * supported by nrfx (see the corresponding nrfx_config_*.h files).
 */

#ifdef CONFIG_NRFX_ADC
#define NRFX_ADC_ENABLED 1
#endif

#ifdef CONFIG_NRFX_CLOCK
#define NRFX_CLOCK_ENABLED 1
#endif

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
#if defined(CONFIG_SOC_SERIES_NRF91X) || defined(CONFIG_SOC_SERIES_NRF53X)
#define NRFX_CLOCK_CONFIG_LF_SRC 1
#else
#define NRFX_CLOCK_CONFIG_LF_SRC 0
#endif
#endif // CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL
#if defined(CONFIG_SOC_SERIES_NRF91X) || defined(CONFIG_SOC_SERIES_NRF53X)
#define NRFX_CLOCK_CONFIG_LF_SRC 2
#else
#define NRFX_CLOCK_CONFIG_LF_SRC 1
#endif
#endif // CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH
#ifdef CONFIG_SOC_SERIES_NRF53X
#define NRFX_CLOCK_CONFIG_LF_SRC 3
#else
#define NRFX_CLOCK_CONFIG_LF_SRC 2
#endif
#endif // CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING
#define NRFX_CLOCK_CONFIG_LF_SRC 131073
#endif

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING
#define NRFX_CLOCK_CONFIG_LF_SRC 196609
#endif

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION
#define NRFX_CLOCK_CONFIG_LF_CAL_ENABLED 1
#endif

#ifdef CONFIG_NRFX_CLOCK_LFXO_TWO_STAGE_ENABLED
#define NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED 1
#endif

#ifdef CONFIG_NRFX_COMP
#define NRFX_COMP_ENABLED 1
#endif

#ifdef CONFIG_NRFX_DPPI
#define NRFX_DPPI_ENABLED 1
#endif

#ifdef CONFIG_NRFX_EGU
#define NRFX_EGU_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU0
#define NRFX_EGU0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU1
#define NRFX_EGU1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU2
#define NRFX_EGU2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU3
#define NRFX_EGU3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU4
#define NRFX_EGU4_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU5
#define NRFX_EGU5_ENABLED 1
#endif

#ifdef CONFIG_NRFX_GPIOTE
#define NRFX_GPIOTE_ENABLED 1
#endif

#ifdef CONFIG_NRFX_GPIOTE_NUM_OF_EVT_HANDLERS
#define NRFX_GPIOTE_CONFIG_NUM_OF_EVT_HANDLERS CONFIG_NRFX_GPIOTE_NUM_OF_EVT_HANDLERS
#endif

#ifdef CONFIG_NRFX_I2S
#define NRFX_I2S_ENABLED 1
#endif
#ifdef CONFIG_NRFX_I2S0
#define NRFX_I2S0_ENABLED 1
#endif

#ifdef CONFIG_NRFX_IPC
#define NRFX_IPC_ENABLED 1
#endif

#ifdef CONFIG_NRFX_LPCOMP
#define NRFX_LPCOMP_ENABLED 1
#endif

#ifdef CONFIG_NRFX_NFCT
#define NRFX_NFCT_ENABLED 1
#endif

#ifdef CONFIG_NRFX_NVMC
#define NRFX_NVMC_ENABLED 1
#endif

#ifdef CONFIG_NRFX_PDM
#define NRFX_PDM_ENABLED 1
#endif

#ifdef CONFIG_NRFX_POWER
#define NRFX_POWER_ENABLED 1
#endif

#ifdef CONFIG_NRFX_PPI
#define NRFX_PPI_ENABLED 1
#endif

#ifdef CONFIG_NRFX_PRS
#define NRFX_PRS_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_BOX_0
#define NRFX_PRS_BOX_0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_BOX_1
#define NRFX_PRS_BOX_1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_BOX_2
#define NRFX_PRS_BOX_2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_BOX_3
#define NRFX_PRS_BOX_3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_BOX_4
#define NRFX_PRS_BOX_4_ENABLED 1
#endif

#ifdef CONFIG_NRFX_PWM
#define NRFX_PWM_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM0
#define NRFX_PWM0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM1
#define NRFX_PWM1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM2
#define NRFX_PWM2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM3
#define NRFX_PWM3_ENABLED 1
#endif

#ifdef CONFIG_NRFX_QDEC
#define NRFX_QDEC_ENABLED 1
#endif

#ifdef CONFIG_NRFX_QSPI
#define NRFX_QSPI_ENABLED 1
#endif

#ifdef CONFIG_NRFX_RNG
#define NRFX_RNG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_RTC
#define NRFX_RTC_ENABLED 1
#endif
#ifdef CONFIG_NRFX_RTC0
#define NRFX_RTC0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_RTC1
#define NRFX_RTC1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_RTC2
#define NRFX_RTC2_ENABLED 1
#endif

#ifdef CONFIG_NRFX_SAADC
#define NRFX_SAADC_ENABLED 1
#endif

#ifdef CONFIG_NRFX_SPI
#define NRFX_SPI_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPI0
#define NRFX_SPI0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPI1
#define NRFX_SPI1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPI2
#define NRFX_SPI2_ENABLED 1
#endif

#ifdef CONFIG_NRFX_SPIM
#define NRFX_SPIM_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM0
#define NRFX_SPIM0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM1
#define NRFX_SPIM1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM2
#define NRFX_SPIM2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM3
#define NRFX_SPIM3_ENABLED 1
#ifdef CONFIG_NRF52_ANOMALY_198_WORKAROUND
#define NRFX_SPIM3_NRF52840_ANOMALY_198_WORKAROUND_ENABLED 1
#endif
#endif
#ifdef CONFIG_NRFX_SPIM4
#define NRFX_SPIM4_ENABLED 1
#endif
#if (DT_PROP(DT_NODELABEL(spi3), rx_delay_supported) || \
	DT_PROP(DT_NODELABEL(spi4), rx_delay_supported))
#define NRFX_SPIM_EXTENDED_ENABLED 1
#endif

#ifdef CONFIG_NRFX_SPIS
#define NRFX_SPIS_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS0
#define NRFX_SPIS0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS1
#define NRFX_SPIS1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS2
#define NRFX_SPIS2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS3
#define NRFX_SPIS3_ENABLED 1
#endif

#ifdef CONFIG_NRFX_SYSTICK
#define NRFX_SYSTICK_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TEMP
#define NRFX_TEMP_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TIMER
#define NRFX_TIMER_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER0
#define NRFX_TIMER0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER1
#define NRFX_TIMER1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER2
#define NRFX_TIMER2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER3
#define NRFX_TIMER3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER4
#define NRFX_TIMER4_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TWI
#define NRFX_TWI_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWI0
#define NRFX_TWI0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWI1
#define NRFX_TWI1_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TWIM
#define NRFX_TWIM_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM0
#define NRFX_TWIM0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM1
#define NRFX_TWIM1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM2
#define NRFX_TWIM2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM3
#define NRFX_TWIM3_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TWIS
#define NRFX_TWIS_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS0
#define NRFX_TWIS0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS1
#define NRFX_TWIS1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS2
#define NRFX_TWIS2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS3
#define NRFX_TWIS3_ENABLED 1
#endif

#ifdef CONFIG_NRFX_UART
#define NRFX_UART_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UART0
#define NRFX_UART0_ENABLED 1
#endif

#ifdef CONFIG_NRFX_UARTE
#define NRFX_UARTE_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE0
#define NRFX_UARTE0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE1
#define NRFX_UARTE1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE2
#define NRFX_UARTE2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE3
#define NRFX_UARTE3_ENABLED 1
#endif

#ifdef CONFIG_NRFX_USBD
#define NRFX_USBD_ENABLED 1
#endif

#ifdef CONFIG_NRFX_USBREG
#define NRFX_USBREG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_WDT
#define NRFX_WDT_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT0
#define NRFX_WDT0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT1
#define NRFX_WDT1_ENABLED 1
#endif

#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
#include "nrfx_config_bsim.h"
#endif

/*
 * For chips with TrustZone support, MDK provides CMSIS-Core peripheral
 * accessing symbols in two flavors, with secure and non-secure base address
 * mappings. Their names contain the suffix _S or _NS, respectively.
 * Because nrfx HALs and drivers require these peripheral accessing symbols
 * without any suffixes, the following macro is provided that will translate
 * their names according to the kind of the target that is built.
 */
#if defined(NRF_TRUSTZONE_NONSECURE)
#define NRF_PERIPH(P) P##_NS
#else
#define NRF_PERIPH(P) P##_S
#endif

#if defined(NRF51)
    #include <nrfx_config_nrf51.h>
#elif defined(NRF52805_XXAA)
    #include <nrfx_config_nrf52805.h>
#elif defined(NRF52810_XXAA)
    #include <nrfx_config_nrf52810.h>
#elif defined(NRF52811_XXAA)
    #include <nrfx_config_nrf52811.h>
#elif defined(NRF52820_XXAA)
    #include <nrfx_config_nrf52820.h>
#elif defined(NRF52832_XXAA) || defined (NRF52832_XXAB)
    #include <nrfx_config_nrf52832.h>
#elif defined(NRF52833_XXAA)
    #include <nrfx_config_nrf52833.h>
#elif defined(NRF52840_XXAA)
    #include <nrfx_config_nrf52840.h>
#elif defined(NRF5340_XXAA_APPLICATION)
    #include <nrfx_config_nrf5340_application.h>
#elif defined(NRF5340_XXAA_NETWORK)
    #include <nrfx_config_nrf5340_network.h>
#elif defined(NRF9120_XXAA) || defined(NRF9160_XXAA)
    #include <nrfx_config_nrf91.h>
#else
    #error "Unknown device."
#endif

#endif // NRFX_CONFIG_H__
