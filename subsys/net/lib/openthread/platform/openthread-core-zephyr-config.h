/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file includes Zephyr compile-time configuration constants
 *   for OpenThread.
 */

#ifndef OPENTHREAD_CORE_ZEPHYR_CONFIG_H_
#define OPENTHREAD_CORE_ZEPHYR_CONFIG_H_

#include <generated_dts_board.h>

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 */
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS                   128

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers
 * (set using `otSetStateChangedCallback()`).
 *
 */
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS              2

/**
 * @def OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#define OPENTHREAD_CONFIG_ADDRESS_CACHE_ENTRIES                 20

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPREND_LEVEL
 *
 * Define to prepend the log level to all log messages
 *
 */
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL                     0

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT
 *
 * Define to 1 if you want to enable software ACK timeout logic.
 *
 */
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_ACK_TIMEOUT           1

/**
 * @def OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT
 *
 * Define to 1 if you want to enable software retransmission logic.
 *
 */
#define OPENTHREAD_CONFIG_ENABLE_SOFTWARE_RETRANSMIT            1

/**
 * @def SETTINGS_CONFIG_BASE_ADDRESS
 *
 * The base address of settings.
 *
 */
#define SETTINGS_CONFIG_BASE_ADDRESS                            0

/**
 * @def SETTINGS_CONFIG_PAGE_SIZE
 *
 * The page size of settings. Ensure that 'erase-block-size'
 * is set in your SOC dts file.
 *
 */
#define SETTINGS_CONFIG_PAGE_SIZE          FLASH_ERASE_BLOCK_SIZE

/**
 * @def SETTINGS_CONFIG_PAGE_NUM
 *
 * The page number of settings.
 *
 */
#define SETTINGS_CONFIG_PAGE_NUM                                4

/**
 * @def OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER
 *
 * Define to 1 if you want to enable microsecond backoff timer
 * implemented in platform.
 *
 */
#define OPENTHREAD_CONFIG_ENABLE_PLATFORM_USEC_TIMER            0

#endif  /* OPENTHREAD_CORE_NRF52840_CONFIG_H_ */
