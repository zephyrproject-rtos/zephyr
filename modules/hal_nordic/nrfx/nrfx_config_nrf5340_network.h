/*
 * Copyright (c) 2019 - 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_CONFIG_NRF5340_NETWORK_H__
#define NRFX_CONFIG_NRF5340_NETWORK_H__

#ifndef NRFX_CONFIG_H__
#error "This file should not be included directly. Include nrfx_config.h instead."
#endif

/*
 * The MDK provides macros for accessing the peripheral register structures
 * by using their secure and non-secure address mappings (with the names
 * containing the suffix _S or _NS, respectively). Because the nrfx drivers
 * use the macros without any suffixes, you must translate the names.
 * The following section provides configuration for the name translation.
 */
#define NRF_AAR        NRF_AAR_NS
#define NRF_ACL        NRF_ACL_NS
#define NRF_CCM        NRF_CCM_NS
#define NRF_CLOCK      NRF_CLOCK_NS
#define NRF_CTI        NRF_CTI_NS
#define NRF_CTRLAP     NRF_CTRLAP_NS
#define NRF_DCNF       NRF_DCNF_NS
#define NRF_DPPIC      NRF_DPPIC_NS
#define NRF_ECB        NRF_ECB_NS
#define NRF_EGU0       NRF_EGU0_NS
#define NRF_FICR       NRF_FICR_NS
#define NRF_GPIOTE     NRF_GPIOTE_NS
#define NRF_IPC        NRF_IPC_NS
#define NRF_NVMC       NRF_NVMC_NS
#define NRF_P0         NRF_P0_NS
#define NRF_P1         NRF_P1_NS
#define NRF_POWER      NRF_POWER_NS
#define NRF_RADIO      NRF_RADIO_NS
#define NRF_RESET      NRF_RESET_NS
#define NRF_RNG        NRF_RNG_NS
#define NRF_RTC0       NRF_RTC0_NS
#define NRF_RTC1       NRF_RTC1_NS
#define NRF_SPIM0      NRF_SPIM0_NS
#define NRF_SPIS0      NRF_SPIS0_NS
#define NRF_SWI0       NRF_SWI0_NS
#define NRF_SWI1       NRF_SWI1_NS
#define NRF_SWI2       NRF_SWI2_NS
#define NRF_SWI3       NRF_SWI3_NS
#define NRF_TEMP       NRF_TEMP_NS
#define NRF_TIMER0     NRF_TIMER0_NS
#define NRF_TIMER1     NRF_TIMER1_NS
#define NRF_TIMER2     NRF_TIMER2_NS
#define NRF_TWIM0      NRF_TWIM0_NS
#define NRF_TWIS0      NRF_TWIS0_NS
#define NRF_UARTE0     NRF_UARTE0_NS
#define NRF_UICR       NRF_UICR_NS
#define NRF_VMC        NRF_VMC_NS
#define NRF_VREQCTRL   NRF_VREQCTRL_NS
#define NRF_WDT        NRF_WDT_NS

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
 * - Synth = 3
 */
#ifndef NRFX_CLOCK_CONFIG_LF_SRC
#define NRFX_CLOCK_CONFIG_LF_SRC 2
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
 * @brief NRFX_RNG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_RNG_ENABLED
#define NRFX_RNG_ENABLED 0
#endif

/**
 * @brief NRFX_RNG_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0 Maximum: 7
 */
#ifndef NRFX_RNG_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_RNG_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_RNG_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_RNG_CONFIG_LOG_ENABLED
#define NRFX_RNG_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_RNG_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_RNG_CONFIG_LOG_LEVEL
#define NRFX_RNG_CONFIG_LOG_LEVEL 3
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
#define NRFX_TEMP_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_TEMP_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_TEMP_CONFIG_LOG_ENABLED
#define NRFX_TEMP_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_TEMP_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_TEMP_CONFIG_LOG_LEVEL
#define NRFX_TEMP_CONFIG_LOG_LEVEL 3
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
 * @brief NRFX_UARTE_ENABLED
 *
 * Boolean. Accepted values 0 and 1.
 */
#ifndef NRFX_UARTE_ENABLED
#define NRFX_UARTE_ENABLED 0
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

#endif // NRFX_CONFIG_NRF5340_NETWORK_H__
