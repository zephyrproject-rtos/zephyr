/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_CONFIG_NRF54L15_APPLICATION_H__
#define NRFX_CONFIG_NRF54L15_APPLICATION_H__

#ifndef NRFX_CONFIG_H__
#error "This file should not be included directly. Include nrfx_config.h instead."
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
 * - RC    = 0
 * - XTAL  = 1
 * - Synth = 2
 */
#ifndef NRFX_CLOCK_CONFIG_LF_SRC
#define NRFX_CLOCK_CONFIG_LF_SRC 0
#endif

/**
 * @brief NRFX_CLOCK_CONFIG_LF_CAL_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_CLOCK_CONFIG_LF_CAL_ENABLED
#define NRFX_CLOCK_CONFIG_LF_CAL_ENABLED 0
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
#define NRFX_CLOCK_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_CLOCK_CONFIG_LOG_LEVEL 0
#endif

/**
 * @brief NRFX_COMP_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_COMP_ENABLED
#define NRFX_COMP_ENABLED 0
#endif

/**
 * @brief NRFX_COMP_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_COMP_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_COMP_DEFAULT_CONFIG_IRQ_PRIORITY 0
#endif

/**
 * @brief NRFX_COMP_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_COMP_CONFIG_LOG_ENABLED
#define NRFX_COMP_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_COMP_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_COMP_CONFIG_LOG_LEVEL
#define NRFX_COMP_CONFIG_LOG_LEVEL 0
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
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_DPPI_CONFIG_LOG_LEVEL
#define NRFX_DPPI_CONFIG_LOG_LEVEL 0
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
 * @brief NRFX_EGU10_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_EGU10_ENABLED
#define NRFX_EGU10_ENABLED 0
#endif

/**
 * @brief NRFX_EGU20_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_EGU20_ENABLED
#define NRFX_EGU20_ENABLED 0
#endif

/**
 * @brief NRFX_EGU_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_EGU_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_EGU_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
 * @brief NRFX_GPIOTE_CONFIG_NUM_OF_EVT_HANDLERS
 *
 * Integer value. Minimum: 0 Maximum: 15
 */
#ifndef NRFX_GPIOTE_CONFIG_NUM_OF_EVT_HANDLERS
#define NRFX_GPIOTE_CONFIG_NUM_OF_EVT_HANDLERS 0
#endif

/**
 * @brief NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_GPIOTE_CONFIG_LOG_LEVEL 0
#endif

/**
 * @brief NRFX_GRTC_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_GRTC_ENABLED
#define NRFX_GRTC_ENABLED 0
#endif

/**
 * @brief NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS
 *
 * Integer value.
 */
#ifndef NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS
#define NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS 11
#endif

/**
 * @brief GRTC CC channels ownership mask.
 */
#ifndef NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK
#define NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK 0x000007ff
#endif

/**
 * @brief NRFX_GRTC_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_GRTC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_GRTC_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_I2S_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_I2S_CONFIG_LOG_LEVEL 0
#endif

/**
 * @brief NRFX_LPCOMP_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_LPCOMP_ENABLED
#define NRFX_LPCOMP_ENABLED 0
#endif

/**
 * @brief NRFX_LPCOMP_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_LPCOMP_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_LPCOMP_DEFAULT_CONFIG_IRQ_PRIORITY 0
#endif

/**
 * @brief NRFX_LPCOMP_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_LPCOMP_CONFIG_LOG_ENABLED
#define NRFX_LPCOMP_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_LPCOMP_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_LPCOMP_CONFIG_LOG_LEVEL
#define NRFX_LPCOMP_CONFIG_LOG_LEVEL 0
#endif

/**
 * @brief NRFX_NFCT_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_NFCT_ENABLED
#define NRFX_NFCT_ENABLED 0
#endif

/**
 * @brief NRFX_NFCT_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_NFCT_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_NFCT_DEFAULT_CONFIG_IRQ_PRIORITY 0
#endif

/**
 * @brief NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID
#define NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID 0
#endif

/**
 * @brief NRFX_NFCT_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_NFCT_CONFIG_LOG_ENABLED
#define NRFX_NFCT_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_NFCT_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_NFCT_CONFIG_LOG_LEVEL
#define NRFX_NFCT_CONFIG_LOG_LEVEL 0
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
#define NRFX_PDM_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_PDM_CONFIG_LOG_LEVEL 0
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
#define NRFX_POWER_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
 * @brief NRFX_PRS_BOX_4_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PRS_BOX_4_ENABLED
#define NRFX_PRS_BOX_4_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_5_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PRS_BOX_5_ENABLED
#define NRFX_PRS_BOX_5_ENABLED 0
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
#define NRFX_PRS_CONFIG_LOG_LEVEL 0
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
 * @brief NRFX_PWM20_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PWM20_ENABLED
#define NRFX_PWM20_ENABLED 0
#endif

/**
 * @brief NRFX_PWM21_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PWM21_ENABLED
#define NRFX_PWM21_ENABLED 0
#endif

/**
 * @brief NRFX_PWM22_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_PWM22_ENABLED
#define NRFX_PWM22_ENABLED 0
#endif

/**
 * @brief NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_PWM_CONFIG_LOG_LEVEL 0
#endif

/**
 * @brief NRFX_QDEC_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_QDEC_ENABLED
#define NRFX_QDEC_ENABLED 0
#endif

/**
 * @brief NRFX_QDEC20_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_QDEC20_ENABLED
#define NRFX_QDEC20_ENABLED 0
#endif

/**
 * @brief NRFX_QDEC21_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_QDEC21_ENABLED
#define NRFX_QDEC21_ENABLED 0
#endif

/**
 * @brief NRFX_QDEC_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_QDEC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_QDEC_DEFAULT_CONFIG_IRQ_PRIORITY 0
#endif

/**
 * @brief NRFX_QDEC_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_QDEC_CONFIG_LOG_ENABLED
#define NRFX_QDEC_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_QDEC_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_QDEC_CONFIG_LOG_LEVEL
#define NRFX_QDEC_CONFIG_LOG_LEVEL 0
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
 * @brief NRFX_RTC10_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_RTC10_ENABLED
#define NRFX_RTC10_ENABLED 0
#endif

/**
 * @brief NRFX_RTC30_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_RTC30_ENABLED
#define NRFX_RTC30_ENABLED 0
#endif

/**
 * @brief NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_RTC_CONFIG_LOG_LEVEL 0
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
#define NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_SAADC_CONFIG_LOG_LEVEL 0
#endif

/**
 * @brief NRFX_SPI_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPI_ENABLED
#define NRFX_SPI_ENABLED 0
#endif

/**
 * @brief NRFX_SPI00_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPI00_ENABLED
#define NRFX_SPI00_ENABLED 0
#endif

/**
 * @brief NRFX_SPI20_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPI20_ENABLED
#define NRFX_SPI20_ENABLED 0
#endif

/**
 * @brief NRFX_SPI21_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPI21_ENABLED
#define NRFX_SPI21_ENABLED 0
#endif

/**
 * @brief NRFX_SPI22_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPI22_ENABLED
#define NRFX_SPI22_ENABLED 0
#endif

/**
 * @brief NRFX_SPI30_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPI30_ENABLED
#define NRFX_SPI30_ENABLED 0
#endif

/**
 * @brief NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY 0
#endif

/**
 * @brief NRFX_SPI_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPI_CONFIG_LOG_ENABLED
#define NRFX_SPI_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_SPI_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_SPI_CONFIG_LOG_LEVEL
#define NRFX_SPI_CONFIG_LOG_LEVEL 0
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
 * @brief NRFX_SPIM00_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM00_ENABLED
#define NRFX_SPIM00_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM20_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM20_ENABLED
#define NRFX_SPIM20_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM21_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM21_ENABLED
#define NRFX_SPIM21_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM22_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM22_ENABLED
#define NRFX_SPIM22_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM30_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM30_ENABLED
#define NRFX_SPIM30_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM_EXTENDED_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIM_EXTENDED_ENABLED
#define NRFX_SPIM_EXTENDED_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_SPIM_CONFIG_LOG_LEVEL 0
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
 * @brief NRFX_SPIS00_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS00_ENABLED
#define NRFX_SPIS00_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS20_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS20_ENABLED
#define NRFX_SPIS20_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS21_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS21_ENABLED
#define NRFX_SPIS21_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS22_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS22_ENABLED
#define NRFX_SPIS22_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS30_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_SPIS30_ENABLED
#define NRFX_SPIS30_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_SPIS_CONFIG_LOG_LEVEL 0
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
 * @brief NRFX_TEMP_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TEMP_ENABLED
#define NRFX_TEMP_ENABLED 0
#endif

/**
 * @brief NRFX_TEMP_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_TEMP_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TEMP_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
 * @brief NRFX_TIMER10_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER10_ENABLED
#define NRFX_TIMER10_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER20_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER20_ENABLED
#define NRFX_TIMER20_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER21_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER21_ENABLED
#define NRFX_TIMER21_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER22_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER22_ENABLED
#define NRFX_TIMER22_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER23_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER23_ENABLED
#define NRFX_TIMER23_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER24_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TIMER24_ENABLED
#define NRFX_TIMER24_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_TIMER_CONFIG_LOG_LEVEL 0
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
 * @brief NRFX_TWIM20_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIM20_ENABLED
#define NRFX_TWIM20_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM21_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIM21_ENABLED
#define NRFX_TWIM21_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM22_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIM22_ENABLED
#define NRFX_TWIM22_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM30_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIM30_ENABLED
#define NRFX_TWIM30_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_TWIM_CONFIG_LOG_LEVEL 0
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
 * @brief NRFX_TWIS00_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS00_ENABLED
#define NRFX_TWIS00_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS20_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS20_ENABLED
#define NRFX_TWIS20_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS21_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS21_ENABLED
#define NRFX_TWIS21_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS22_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS22_ENABLED
#define NRFX_TWIS22_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS30_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS30_ENABLED
#define NRFX_TWIS30_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY
#define NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY 0
#endif

/**
 * @brief NRFX_TWIS_NO_SYNC_MODE
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TWIS_NO_SYNC_MODE
#define NRFX_TWIS_NO_SYNC_MODE 0
#endif

/**
 * @brief NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_TWIS_CONFIG_LOG_LEVEL 0
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
 * @brief NRFX_UARTE00_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE00_ENABLED
#define NRFX_UARTE00_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE20_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE20_ENABLED
#define NRFX_UARTE20_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE21_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE21_ENABLED
#define NRFX_UARTE21_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE22_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE22_ENABLED
#define NRFX_UARTE22_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE30_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE30_ENABLED
#define NRFX_UARTE30_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_UARTE_CONFIG_LOG_LEVEL 0
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
 * @brief NRFX_WDT30_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_WDT30_ENABLED
#define NRFX_WDT30_ENABLED 0
#endif

/**
 * @brief NRFX_WDT31_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_WDT31_ENABLED
#define NRFX_WDT31_ENABLED 0
#endif

/**
 * @brief NRFX_WDT_CONFIG_NO_IRQ
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_WDT_CONFIG_NO_IRQ
#define NRFX_WDT_CONFIG_NO_IRQ 0
#endif

/**
 * @brief NRFX_WDT_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_WDT_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_WDT_DEFAULT_CONFIG_IRQ_PRIORITY 0
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
#define NRFX_WDT_CONFIG_LOG_LEVEL 0
#endif

#endif /* NRFX_CONFIG_NRF54L15_APPLICATION_H__ */
