/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_CONFIG_NRF54H20_FLPR_H__
#define NRFX_CONFIG_NRF54H20_FLPR_H__

#ifndef NRFX_CONFIG_H__
#error "This file should not be included directly. Include nrfx_config.h instead."
#endif


/**
 * @brief NRFX_DEFAULT_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_DEFAULT_IRQ_PRIORITY
#define NRFX_DEFAULT_IRQ_PRIORITY 0
#endif

/**
 * @brief NRFX_COMP_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_COMP_ENABLED
#define NRFX_COMP_ENABLED 0
#endif

/**
 * @brief NRFX_COMP_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_COMP_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_COMP_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_COMP_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
#define NRFX_COMP_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_COREDEP_VPR_LEGACY
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_COREDEP_VPR_LEGACY
#define NRFX_COREDEP_VPR_LEGACY 0
#endif

/**
 * @brief NRFX_DPPI_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_DPPI_ENABLED
#define NRFX_DPPI_ENABLED 0
#endif

/**
 * @brief NRFX_DPPI_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * @brief NRFX_DPPI120_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI120_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI120_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0x00000030
#endif

/**
 * @brief NRFX_DPPI130_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI130_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI130_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0x000000ff
#endif

/**
 * @brief NRFX_DPPI131_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI131_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI131_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0
#endif

/**
 * @brief NRFX_DPPI132_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI132_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI132_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0
#endif

/**
 * @brief NRFX_DPPI133_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI133_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI133_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0x0000001e
#endif

/**
 * @brief NRFX_DPPI134_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI134_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI134_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0x00000020
#endif

/**
 * @brief NRFX_DPPI135_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI135_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI135_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0x00000040
#endif

/**
 * @brief NRFX_DPPI136_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI136_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI136_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0x00000081
#endif

/**
 * @brief NRFX_DPPI120_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI120_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI120_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0x0000000c
#endif

/**
 * @brief NRFX_DPPI130_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI130_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI130_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0x000000ff
#endif

/**
 * @brief NRFX_DPPI131_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI131_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI131_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0x000000ff
#endif

/**
 * @brief NRFX_DPPI132_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI132_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI132_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0
#endif

/**
 * @brief NRFX_DPPI133_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI133_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI133_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0x000000e1
#endif

/**
 * @brief NRFX_DPPI134_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI134_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI134_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0x000000df
#endif

/**
 * @brief NRFX_DPPI135_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI135_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI135_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0x000000bf
#endif

/**
 * @brief NRFX_DPPI136_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_DPPI136_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_DPPI136_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0x0000007e
#endif

/**
 * @brief NRFX_EGU_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_EGU_ENABLED
#define NRFX_EGU_ENABLED 0
#endif

/**
 * @brief NRFX_EGU_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_EGU_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_EGU_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_EGU130_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_EGU130_ENABLED
#define NRFX_EGU130_ENABLED 0
#endif

/**
 * @brief NRFX_GRTC_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_GRTC_ENABLED
#define NRFX_GRTC_ENABLED 0
#endif

/**
 * @brief NRFX_GRTC_CONFIG_AUTOEN
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_GRTC_CONFIG_AUTOEN
#define NRFX_GRTC_CONFIG_AUTOEN 1
#endif

/**
 * @brief NRFX_GRTC_CONFIG_AUTOSTART
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_GRTC_CONFIG_AUTOSTART
#define NRFX_GRTC_CONFIG_AUTOSTART 0
#endif

/**
 * @brief NRFX_GRTC_CONFIG_CLEAR_AT_INIT
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_GRTC_CONFIG_CLEAR_AT_INIT
#define NRFX_GRTC_CONFIG_CLEAR_AT_INIT 0
#endif

/**
 * @brief NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS
 *
 * Integer value.
 */
#ifndef NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS
#define NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS 4
#endif

/**
 * @brief NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK
 */
#ifndef NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK
#define NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK 0
#endif

/**
 * @brief NRFX_GRTC_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_GRTC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_GRTC_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_GRTC_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_GRTC_CONFIG_LOG_ENABLED
#define NRFX_GRTC_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_GRTC_CONFIG_LOG_LEVEL
 *
 * Integer value.
 * Supported values:
 * - Off     = 0
 * - Error   = 1
 * - Warning = 2
 * - Info    = 3
 * - Debug   = 4
 */
#ifndef NRFX_GRTC_CONFIG_LOG_LEVEL
#define NRFX_GRTC_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_IPCT_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_IPCT_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_IPCT_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0x00000030
#endif

/**
 * @brief NRFX_IPCT120_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_IPCT120_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_IPCT120_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0
#endif

/**
 * @brief NRFX_IPCT130_PUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_IPCT130_PUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_IPCT130_PUB_CONFIG_ALLOWED_CHANNELS_MASK 0x0000000c
#endif

/**
 * @brief NRFX_IPCT_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_IPCT_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_IPCT_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0x000000c0
#endif

/**
 * @brief NRFX_IPCT120_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_IPCT120_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_IPCT120_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0
#endif

/**
 * @brief NRFX_IPCT130_SUB_CONFIG_ALLOWED_CHANNELS_MASK
 */
#ifndef NRFX_IPCT130_SUB_CONFIG_ALLOWED_CHANNELS_MASK
#define NRFX_IPCT130_SUB_CONFIG_ALLOWED_CHANNELS_MASK 0x00000003
#endif

/**
 * @brief NRFX_LPCOMP_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_LPCOMP_ENABLED
#define NRFX_LPCOMP_ENABLED 0
#endif

/**
 * @brief NRFX_LPCOMP_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_LPCOMP_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_LPCOMP_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_LPCOMP_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
#define NRFX_LPCOMP_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_MVDMA_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_MVDMA_ENABLED
#define NRFX_MVDMA_ENABLED 0
#endif

/**
 * @brief NRFX_MVDMA120_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_MVDMA120_ENABLED
#define NRFX_MVDMA120_ENABLED 0
#endif

/**
 * @brief NRFX_NFCT_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_NFCT_ENABLED
#define NRFX_NFCT_ENABLED 0
#endif

/**
 * @brief NRFX_NFCT_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_NFCT_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_NFCT_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID - Timer instance used for workarounds in the driver.
 *
 * Integer value. Minimum: 0. Maximum: 5.
 */
#ifndef NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID
#define NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID 0
#endif

/**
 * @brief NRFX_NFCT_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
#define NRFX_NFCT_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_PDM_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PDM_ENABLED
#define NRFX_PDM_ENABLED 0
#endif

/**
 * @brief NRFX_PDM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_PDM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_PDM_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_PDM_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * @brief NRFX_PRS_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_ENABLED
#define NRFX_PRS_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_BOX_0_ENABLED
#define NRFX_PRS_BOX_0_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_1_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_BOX_1_ENABLED
#define NRFX_PRS_BOX_1_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_2_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_BOX_2_ENABLED
#define NRFX_PRS_BOX_2_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_3_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_BOX_3_ENABLED
#define NRFX_PRS_BOX_3_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_4_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_BOX_4_ENABLED
#define NRFX_PRS_BOX_4_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_5_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_BOX_5_ENABLED
#define NRFX_PRS_BOX_5_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_6_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_BOX_6_ENABLED
#define NRFX_PRS_BOX_6_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_7_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_BOX_7_ENABLED
#define NRFX_PRS_BOX_7_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_8_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_BOX_8_ENABLED
#define NRFX_PRS_BOX_8_ENABLED 0
#endif

/**
 * @brief NRFX_PRS_BOX_9_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PRS_BOX_9_ENABLED
#define NRFX_PRS_BOX_9_ENABLED 0
#endif

/**
 * @brief NRFX_PWM_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PWM_ENABLED
#define NRFX_PWM_ENABLED 0
#endif

/**
 * @brief NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_PWM_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * @brief NRFX_PWM120_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PWM120_ENABLED
#define NRFX_PWM120_ENABLED 0
#endif

/**
 * @brief NRFX_PWM130_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PWM130_ENABLED
#define NRFX_PWM130_ENABLED 0
#endif

/**
 * @brief NRFX_PWM131_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PWM131_ENABLED
#define NRFX_PWM131_ENABLED 0
#endif

/**
 * @brief NRFX_PWM132_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PWM132_ENABLED
#define NRFX_PWM132_ENABLED 0
#endif

/**
 * @brief NRFX_PWM133_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_PWM133_ENABLED
#define NRFX_PWM133_ENABLED 0
#endif

/**
 * @brief NRFX_QDEC_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_QDEC_ENABLED
#define NRFX_QDEC_ENABLED 0
#endif

/**
 * @brief NRFX_QDEC_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_QDEC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_QDEC_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_QDEC_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
#define NRFX_QDEC_CONFIG_LOG_LEVEL 3
#endif

/**
 * @brief NRFX_QDEC130_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_QDEC130_ENABLED
#define NRFX_QDEC130_ENABLED 0
#endif

/**
 * @brief NRFX_QDEC131_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_QDEC131_ENABLED
#define NRFX_QDEC131_ENABLED 0
#endif

/**
 * @brief NRFX_RTC_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_RTC_ENABLED
#define NRFX_RTC_ENABLED 0
#endif

/**
 * @brief NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_RTC_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * @brief NRFX_RTC130_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_RTC130_ENABLED
#define NRFX_RTC130_ENABLED 0
#endif

/**
 * @brief NRFX_RTC131_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_RTC131_ENABLED
#define NRFX_RTC131_ENABLED 0
#endif

/**
 * @brief NRFX_SAADC_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SAADC_ENABLED
#define NRFX_SAADC_ENABLED 0
#endif

/**
 * @brief NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_SAADC_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM_ENABLED
#define NRFX_SPIM_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_SPIM_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * @brief NRFX_SPIM120_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM120_ENABLED
#define NRFX_SPIM120_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM121_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM121_ENABLED
#define NRFX_SPIM121_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM130_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM130_ENABLED
#define NRFX_SPIM130_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM131_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM131_ENABLED
#define NRFX_SPIM131_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM132_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM132_ENABLED
#define NRFX_SPIM132_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM133_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM133_ENABLED
#define NRFX_SPIM133_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM134_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM134_ENABLED
#define NRFX_SPIM134_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM135_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM135_ENABLED
#define NRFX_SPIM135_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM136_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM136_ENABLED
#define NRFX_SPIM136_ENABLED 0
#endif

/**
 * @brief NRFX_SPIM137_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIM137_ENABLED
#define NRFX_SPIM137_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIS_ENABLED
#define NRFX_SPIS_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_SPIS_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * @brief NRFX_SPIS120_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIS120_ENABLED
#define NRFX_SPIS120_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS130_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIS130_ENABLED
#define NRFX_SPIS130_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS131_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIS131_ENABLED
#define NRFX_SPIS131_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS132_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIS132_ENABLED
#define NRFX_SPIS132_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS133_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIS133_ENABLED
#define NRFX_SPIS133_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS134_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIS134_ENABLED
#define NRFX_SPIS134_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS135_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIS135_ENABLED
#define NRFX_SPIS135_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS136_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIS136_ENABLED
#define NRFX_SPIS136_ENABLED 0
#endif

/**
 * @brief NRFX_SPIS137_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_SPIS137_ENABLED
#define NRFX_SPIS137_ENABLED 0
#endif

/**
 * @brief NRFX_TEMP_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TEMP_ENABLED
#define NRFX_TEMP_ENABLED 0
#endif

/**
 * @brief NRFX_TEMP_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_TEMP_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TEMP_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_TEMP_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER_ENABLED
#define NRFX_TIMER_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_TIMER_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * @brief NRFX_TIMER120_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER120_ENABLED
#define NRFX_TIMER120_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER121_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER121_ENABLED
#define NRFX_TIMER121_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER130_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER130_ENABLED
#define NRFX_TIMER130_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER131_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER131_ENABLED
#define NRFX_TIMER131_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER132_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER132_ENABLED
#define NRFX_TIMER132_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER133_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER133_ENABLED
#define NRFX_TIMER133_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER134_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER134_ENABLED
#define NRFX_TIMER134_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER135_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER135_ENABLED
#define NRFX_TIMER135_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER136_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER136_ENABLED
#define NRFX_TIMER136_ENABLED 0
#endif

/**
 * @brief NRFX_TIMER137_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TIMER137_ENABLED
#define NRFX_TIMER137_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIM_ENABLED
#define NRFX_TWIM_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_TWIM_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * @brief NRFX_TWIM130_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIM130_ENABLED
#define NRFX_TWIM130_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM131_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIM131_ENABLED
#define NRFX_TWIM131_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM132_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIM132_ENABLED
#define NRFX_TWIM132_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM133_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIM133_ENABLED
#define NRFX_TWIM133_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM134_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIM134_ENABLED
#define NRFX_TWIM134_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM135_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIM135_ENABLED
#define NRFX_TWIM135_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM136_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIM136_ENABLED
#define NRFX_TWIM136_ENABLED 0
#endif

/**
 * @brief NRFX_TWIM137_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIM137_ENABLED
#define NRFX_TWIM137_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS_ENABLED
#define NRFX_TWIS_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_TWIS_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS_CONFIG_LOG_ENABLED
#define NRFX_TWIS_CONFIG_LOG_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY - Assume that any instance would be initialized
 *        only once.
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY
#define NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY 0
#endif

/**
 * @brief NRFX_TWIS_NO_SYNC_MODE - Remove support for synchronous mode.
 *
 * Boolean. Accepted values: 0 and 1.
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
 * @brief NRFX_TWIS130_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS130_ENABLED
#define NRFX_TWIS130_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS131_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS131_ENABLED
#define NRFX_TWIS131_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS132_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS132_ENABLED
#define NRFX_TWIS132_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS133_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS133_ENABLED
#define NRFX_TWIS133_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS134_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS134_ENABLED
#define NRFX_TWIS134_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS135_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS135_ENABLED
#define NRFX_TWIS135_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS136_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS136_ENABLED
#define NRFX_TWIS136_ENABLED 0
#endif

/**
 * @brief NRFX_TWIS137_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_TWIS137_ENABLED
#define NRFX_TWIS137_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE_ENABLED
#define NRFX_UARTE_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE_CONFIG_SKIP_GPIO_CONFIG - If enabled, support for configuring GPIO pins is
 *        removed from the driver
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE_CONFIG_SKIP_GPIO_CONFIG
#define NRFX_UARTE_CONFIG_SKIP_GPIO_CONFIG 0
#endif

/**
 * @brief NRFX_UARTE_CONFIG_SKIP_PSEL_CONFIG - If enabled, support for configuring PSEL registers
 *        is removed from the driver
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE_CONFIG_SKIP_PSEL_CONFIG
#define NRFX_UARTE_CONFIG_SKIP_PSEL_CONFIG 0
#endif

/**
 * @brief NRFX_UARTE_CONFIG_TX_LINK - If enabled, driver supports linking of TX transfers.
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE_CONFIG_TX_LINK
#define NRFX_UARTE_CONFIG_TX_LINK 1
#endif

/**
 * @brief NRFX_UARTE_CONFIG_RX_CACHE_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE_CONFIG_RX_CACHE_ENABLED
#define NRFX_UARTE_CONFIG_RX_CACHE_ENABLED 1
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
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_UARTE_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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
 * @brief NRFX_UARTE120_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE120_ENABLED
#define NRFX_UARTE120_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE130_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE130_ENABLED
#define NRFX_UARTE130_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE131_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE131_ENABLED
#define NRFX_UARTE131_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE132_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE132_ENABLED
#define NRFX_UARTE132_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE133_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE133_ENABLED
#define NRFX_UARTE133_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE134_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE134_ENABLED
#define NRFX_UARTE134_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE135_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE135_ENABLED
#define NRFX_UARTE135_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE136_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE136_ENABLED
#define NRFX_UARTE136_ENABLED 0
#endif

/**
 * @brief NRFX_UARTE137_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_UARTE137_ENABLED
#define NRFX_UARTE137_ENABLED 0
#endif

/**
 * @brief NRFX_VEVIF_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_VEVIF_ENABLED
#define NRFX_VEVIF_ENABLED 0
#endif

/**
 * @brief NRFX_WDT_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_WDT_ENABLED
#define NRFX_WDT_ENABLED 0
#endif

/**
 * @brief NRFX_WDT_DEFAULT_CONFIG_IRQ_PRIORITY
 *
 * Integer value. Minimum: 0. Maximum: 3.
 */
#ifndef NRFX_WDT_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_WDT_DEFAULT_CONFIG_IRQ_PRIORITY NRFX_DEFAULT_IRQ_PRIORITY
#endif

/**
 * @brief NRFX_WDT_CONFIG_NO_IRQ - Remove WDT IRQ handling from WDT driver
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_WDT_CONFIG_NO_IRQ
#define NRFX_WDT_CONFIG_NO_IRQ 0
#endif

/**
 * @brief NRFX_WDT_CONFIG_LOG_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
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

/**
 * @brief NRFX_WDT131_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_WDT131_ENABLED
#define NRFX_WDT131_ENABLED 0
#endif

/**
 * @brief NRFX_WDT132_ENABLED
 *
 * Boolean. Accepted values: 0 and 1.
 */
#ifndef NRFX_WDT132_ENABLED
#define NRFX_WDT132_ENABLED 0
#endif

#endif /* NRFX_CONFIG_NRF54H20_FLPR_H__ */
