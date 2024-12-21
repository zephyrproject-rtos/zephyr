/*
 * Copyright (c) 2018 - 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_CONFIG_NRF91_H__
#define NRFX_CONFIG_NRF91_H__

#ifndef NRFX_CONFIG_H__
#error "This file should not be included directly. Include nrfx_config.h instead."
#endif

#define NRF_CLOCK        NRF_PERIPH(NRF_CLOCK)
#define NRF_DPPIC        NRF_PERIPH(NRF_DPPIC)
#define NRF_EGU0         NRF_PERIPH(NRF_EGU0)
#define NRF_EGU1         NRF_PERIPH(NRF_EGU1)
#define NRF_EGU2         NRF_PERIPH(NRF_EGU2)
#define NRF_EGU3         NRF_PERIPH(NRF_EGU3)
#define NRF_EGU4         NRF_PERIPH(NRF_EGU4)
#define NRF_EGU5         NRF_PERIPH(NRF_EGU5)
#define NRF_FPU          NRF_PERIPH(NRF_FPU)
#define NRF_I2S          NRF_PERIPH(NRF_I2S)
#define NRF_IPC          NRF_PERIPH(NRF_IPC)
#define NRF_KMU          NRF_PERIPH(NRF_KMU)
#define NRF_NVMC         NRF_PERIPH(NRF_NVMC)
#define NRF_P0           NRF_PERIPH(NRF_P0)
#define NRF_PDM          NRF_PERIPH(NRF_PDM)
#define NRF_POWER        NRF_PERIPH(NRF_POWER)
#define NRF_PWM0         NRF_PERIPH(NRF_PWM0)
#define NRF_PWM1         NRF_PERIPH(NRF_PWM1)
#define NRF_PWM2         NRF_PERIPH(NRF_PWM2)
#define NRF_PWM3         NRF_PERIPH(NRF_PWM3)
#define NRF_REGULATORS   NRF_PERIPH(NRF_REGULATORS)
#define NRF_RTC0         NRF_PERIPH(NRF_RTC0)
#define NRF_RTC1         NRF_PERIPH(NRF_RTC1)
#define NRF_SAADC        NRF_PERIPH(NRF_SAADC)
#define NRF_SPIM0        NRF_PERIPH(NRF_SPIM0)
#define NRF_SPIM1        NRF_PERIPH(NRF_SPIM1)
#define NRF_SPIM2        NRF_PERIPH(NRF_SPIM2)
#define NRF_SPIM3        NRF_PERIPH(NRF_SPIM3)
#define NRF_SPIS0        NRF_PERIPH(NRF_SPIS0)
#define NRF_SPIS1        NRF_PERIPH(NRF_SPIS1)
#define NRF_SPIS2        NRF_PERIPH(NRF_SPIS2)
#define NRF_SPIS3        NRF_PERIPH(NRF_SPIS3)
#define NRF_TIMER0       NRF_PERIPH(NRF_TIMER0)
#define NRF_TIMER1       NRF_PERIPH(NRF_TIMER1)
#define NRF_TIMER2       NRF_PERIPH(NRF_TIMER2)
#define NRF_TWIM0        NRF_PERIPH(NRF_TWIM0)
#define NRF_TWIM1        NRF_PERIPH(NRF_TWIM1)
#define NRF_TWIM2        NRF_PERIPH(NRF_TWIM2)
#define NRF_TWIM3        NRF_PERIPH(NRF_TWIM3)
#define NRF_TWIS0        NRF_PERIPH(NRF_TWIS0)
#define NRF_TWIS1        NRF_PERIPH(NRF_TWIS1)
#define NRF_TWIS2        NRF_PERIPH(NRF_TWIS2)
#define NRF_TWIS3        NRF_PERIPH(NRF_TWIS3)
#define NRF_UARTE0       NRF_PERIPH(NRF_UARTE0)
#define NRF_UARTE1       NRF_PERIPH(NRF_UARTE1)
#define NRF_UARTE2       NRF_PERIPH(NRF_UARTE2)
#define NRF_UARTE3       NRF_PERIPH(NRF_UARTE3)
#define NRF_VMC          NRF_PERIPH(NRF_VMC)
#define NRF_WDT          NRF_PERIPH(NRF_WDT)

/*
 * The following section provides the name translation for peripherals with
 * only one type of access available. For these peripherals, you cannot choose
 * between secure and non-secure mapping.
 */
#if defined(NRF_TRUSTZONE_NONSECURE)
#define NRF_GPIOTE       NRF_GPIOTE1
#define NRF_GPIOTE1      NRF_GPIOTE1_NS
#else
#define NRF_CC_HOST_RGF  NRF_CC_HOST_RGF_S
#define NRF_CRYPTOCELL   NRF_CRYPTOCELL_S
#define NRF_CTRL_AP_PERI NRF_CTRL_AP_PERI_S
#define NRF_FICR         NRF_FICR_S
#define NRF_GPIOTE       NRF_GPIOTE0
#define NRF_GPIOTE0      NRF_GPIOTE0_S
#define NRF_GPIOTE1      NRF_GPIOTE1_NS
#define NRF_SPU          NRF_SPU_S
#define NRF_TAD          NRF_TAD_S
#define NRF_UICR         NRF_UICR_S
#endif

/**
 * @brief NRFX_DEFAULT_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_DEFAULT_IRQ_PRIORITY
#define NRFX_DEFAULT_IRQ_PRIORITY 7
#endif

/**
 * @brief NRFX_CLOCK_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_CLOCK_ENABLED
#define NRFX_CLOCK_ENABLED 0
#endif

/**
 * @brief NRFX_CLOCK_CONFIG_LF_SRC
 *
 * Integer value.
 * Supported values:
 * - RC    = 1
 * - XTAL  = 2
 */
#ifndef NRFX_CLOCK_CONFIG_LF_SRC
#define NRFX_CLOCK_CONFIG_LF_SRC 2
#endif

/**
 * @brief NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED
#define NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED 0
#endif

/**
 * @brief NRFX_CLOCK_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_CLOCK_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_CLOCK_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_CLOCK_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_CLOCK_CONFIG_LOG_ENABLED
#define NRFX_CLOCK_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_CLOCK_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_CLOCK_CONFIG_LOG_LEVEL
#define NRFX_CLOCK_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_DPPI_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_DPPI_ENABLED
#define NRFX_DPPI_ENABLED 0
#endif

/**
 * @brief NRFX_DPPI_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_DPPI_CONFIG_LOG_ENABLED
#define NRFX_DPPI_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_DPPI_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_DPPI_CONFIG_LOG_LEVEL
#define NRFX_DPPI_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_EGU_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_EGU_ENABLED
#define NRFX_EGU_ENABLED 0
#endif

/**
 * @brief NRFX_EGU_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_EGU_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_EGU_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_EGU0_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_EGU0_ENABLED
#define NRFX_EGU0_ENABLED 0
#endif

/**
 * @brief NRFX_EGU1_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_EGU1_ENABLED
#define NRFX_EGU1_ENABLED 0
#endif

/**
 * @brief NRFX_EGU2_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_EGU2_ENABLED
#define NRFX_EGU2_ENABLED 0
#endif

/**
 * @brief NRFX_EGU3_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_EGU3_ENABLED
#define NRFX_EGU3_ENABLED 0
#endif

/**
 * @brief NRFX_EGU4_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_EGU4_ENABLED
#define NRFX_EGU4_ENABLED 0
#endif

/**
 * @brief NRFX_EGU5_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_EGU5_ENABLED
#define NRFX_EGU5_ENABLED 0
#endif

/**
 * @brief NRFX_GPIOTE_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_GPIOTE_ENABLED
#define NRFX_GPIOTE_ENABLED 0
#endif

/**
 * @brief NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_GPIOTE_CONFIG_NUM_OF_EVT_HANDLERS
 *
 * Integer value. Minimum: 0 Maximum: 15
 */
#ifndef NRFX_GPIOTE_CONFIG_NUM_OF_EVT_HANDLERS
#define NRFX_GPIOTE_CONFIG_NUM_OF_EVT_HANDLERS 1
#endif

/**
 * @brief NRFX_GPIOTE_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_GPIOTE_CONFIG_LOG_ENABLED
#define NRFX_GPIOTE_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_GPIOTE_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_GPIOTE_CONFIG_LOG_LEVEL
#define NRFX_GPIOTE_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_I2S_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_I2S_ENABLED
#define NRFX_I2S_ENABLED 0
#endif

/**
 * @brief NRFX_I2S_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_I2S_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_I2S_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_I2S_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_I2S_CONFIG_LOG_ENABLED
#define NRFX_I2S_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_I2S_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_I2S_CONFIG_LOG_LEVEL
#define NRFX_I2S_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_IPC_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_IPC_ENABLED
#define NRFX_IPC_ENABLED 0
#endif

/**
 * @brief NRFX_NVMC_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_NVMC_ENABLED
#define NRFX_NVMC_ENABLED 0
#endif

/**
 * @brief NRFX_PDM_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PDM_ENABLED
#define NRFX_PDM_ENABLED 0
#endif

/**
 * @brief NRFX_PDM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_PDM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_PDM_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_PDM_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PDM_CONFIG_LOG_ENABLED
#define NRFX_PDM_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_PDM_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_PDM_CONFIG_LOG_LEVEL
#define NRFX_PDM_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_POWER_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_POWER_ENABLED
#define NRFX_POWER_ENABLED 0
#endif

/**
 * @brief NRFX_POWER_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_POWER_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_POWER_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_PRS_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PRS_ENABLED
#define NRFX_PRS_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PRS_CONFIG_LOG_ENABLED
#define NRFX_PRS_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_PRS_CONFIG_LOG_LEVEL
#define NRFX_PRS_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_PRS_BOX_0_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PRS_BOX_0_ENABLED
#define NRFX_PRS_BOX_0_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_1_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PRS_BOX_1_ENABLED
#define NRFX_PRS_BOX_1_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_2_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PRS_BOX_2_ENABLED
#define NRFX_PRS_BOX_2_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_3_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PRS_BOX_3_ENABLED
#define NRFX_PRS_BOX_3_ENABLED 0
#endif

/**
 * @brief NRFX_PWM_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PWM_ENABLED
#define NRFX_PWM_ENABLED 0
#endif

/**
 * @brief NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_PWM_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PWM_CONFIG_LOG_ENABLED
#define NRFX_PWM_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_PWM_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_PWM_CONFIG_LOG_LEVEL
#define NRFX_PWM_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_PWM0_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PWM0_ENABLED
#define NRFX_PWM0_ENABLED 0
#endif

/**
 * @brief NRFX_PWM1_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PWM1_ENABLED
#define NRFX_PWM1_ENABLED 0
#endif

/**
 * @brief NRFX_PWM2_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PWM2_ENABLED
#define NRFX_PWM2_ENABLED 0
#endif

/**
 * @brief NRFX_PWM3_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PWM3_ENABLED
#define NRFX_PWM3_ENABLED 0
#endif

/**
 * @brief NRFX_RTC_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_RTC_ENABLED
#define NRFX_RTC_ENABLED 0
#endif

/**
 * @brief NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_RTC_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_RTC_CONFIG_LOG_ENABLED
#define NRFX_RTC_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_RTC_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_RTC_CONFIG_LOG_LEVEL
#define NRFX_RTC_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_RTC0_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_RTC0_ENABLED
#define NRFX_RTC0_ENABLED 0
#endif

/**
 * @brief NRFX_RTC1_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_RTC1_ENABLED
#define NRFX_RTC1_ENABLED 0
#endif

/**
 * @brief NRFX_SAADC_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SAADC_ENABLED
#define NRFX_SAADC_ENABLED 0
#endif

/**
 * @brief NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_SAADC_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SAADC_CONFIG_LOG_ENABLED
#define NRFX_SAADC_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_SAADC_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_SAADC_CONFIG_LOG_LEVEL
#define NRFX_SAADC_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_SPIM_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM_ENABLED
#define NRFX_SPIM_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_SPIM_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM_CONFIG_LOG_ENABLED
#define NRFX_SPIM_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_SPIM_CONFIG_LOG_LEVEL
#define NRFX_SPIM_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_SPIM0_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM0_ENABLED
#define NRFX_SPIM0_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM1_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM1_ENABLED
#define NRFX_SPIM1_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM2_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM2_ENABLED
#define NRFX_SPIM2_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM3_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM3_ENABLED
#define NRFX_SPIM3_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS_ENABLED
#define NRFX_SPIS_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_SPIS_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS_CONFIG_LOG_ENABLED
#define NRFX_SPIS_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_SPIS_CONFIG_LOG_LEVEL
#define NRFX_SPIS_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_SPIS0_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS0_ENABLED
#define NRFX_SPIS0_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS1_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS1_ENABLED
#define NRFX_SPIS1_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS2_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS2_ENABLED
#define NRFX_SPIS2_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS3_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS3_ENABLED
#define NRFX_SPIS3_ENABLED 0
#endif

/**
 * @brief NRFX_SYSTICK_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SYSTICK_ENABLED
#define NRFX_SYSTICK_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER_ENABLED
#define NRFX_TIMER_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_TIMER_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER_CONFIG_LOG_ENABLED
#define NRFX_TIMER_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_TIMER_CONFIG_LOG_LEVEL
#define NRFX_TIMER_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_TIMER0_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER0_ENABLED
#define NRFX_TIMER0_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER1_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER1_ENABLED
#define NRFX_TIMER1_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER2_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER2_ENABLED
#define NRFX_TIMER2_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIM_ENABLED
#define NRFX_TWIM_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_TWIM_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIM_CONFIG_LOG_ENABLED
#define NRFX_TWIM_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_TWIM_CONFIG_LOG_LEVEL
#define NRFX_TWIM_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_TWIM0_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIM0_ENABLED
#define NRFX_TWIM0_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM1_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIM1_ENABLED
#define NRFX_TWIM1_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM2_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIM2_ENABLED
#define NRFX_TWIM2_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM3_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIM3_ENABLED
#define NRFX_TWIM3_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS_ENABLED
#define NRFX_TWIS_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_TWIS_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS_CONFIG_LOG_ENABLED
#define NRFX_TWIS_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY - Assume that any instance would be initialized
 * only once.
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY
#define NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY 0
#endif

/**
 * @brief NRFX_TWIS_NO_SYNC_MODE - Remove support for synchronous mode.
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS_NO_SYNC_MODE
#define NRFX_TWIS_NO_SYNC_MODE 0
#endif

/**
 * @brief NRFX_TWIS_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_TWIS_CONFIG_LOG_LEVEL
#define NRFX_TWIS_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_TWIS0_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS0_ENABLED
#define NRFX_TWIS0_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS1_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS1_ENABLED
#define NRFX_TWIS1_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS2_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS2_ENABLED
#define NRFX_TWIS2_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS3_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS3_ENABLED
#define NRFX_TWIS3_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE_ENABLED
#define NRFX_UARTE_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE_RX_FIFO_FLUSH_WORKAROUND_MAGIC_BYTE
 *
 * Integer value. Minimum: 0. Maximum: 255.
 */
#ifndef NRFX_UARTE_RX_FIFO_FLUSH_WORKAROUND_MAGIC_BYTE
#define NRFX_UARTE_RX_FIFO_FLUSH_WORKAROUND_MAGIC_BYTE 171
#endif

/**
 * @brief NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_UARTE_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE_CONFIG_LOG_ENABLED
#define NRFX_UARTE_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_UARTE_CONFIG_LOG_LEVEL
#define NRFX_UARTE_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_UARTE0_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE0_ENABLED
#define NRFX_UARTE0_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE1_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE1_ENABLED
#define NRFX_UARTE1_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE2_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE2_ENABLED
#define NRFX_UARTE2_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE3_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE3_ENABLED
#define NRFX_UARTE3_ENABLED 0
#endif

/**
 * @brief NRFX_WDT_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_WDT_ENABLED
#define NRFX_WDT_ENABLED 0
#endif

/**
 * @brief NRFX_WDT_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_WDT_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_WDT_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_WDT_CONFIG_NO_IRQ - Remove WDT IRQ handling from WDT driver
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_WDT_CONFIG_NO_IRQ
#define NRFX_WDT_CONFIG_NO_IRQ 0
#endif

/**
 * @brief NRFX_WDT_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_WDT_CONFIG_LOG_ENABLED
#define NRFX_WDT_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_WDT_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_WDT_CONFIG_LOG_LEVEL
#define NRFX_WDT_CONFIG_LOG_LEVEL 3
#endif

#endif /* NRFX_CONFIG_NRF91_H__ */
