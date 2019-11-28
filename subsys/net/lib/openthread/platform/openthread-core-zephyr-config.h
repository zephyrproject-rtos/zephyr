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
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#define OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES             20

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
 *
 * Define to prepend the log level to all log messages.
 *
 */
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL                     0

/**
 * @def OPENTHREAD_CONFIG_SOFTWARE_ACK_TIMEOUT_ENABLE
 *
 * Define to 1 to enable software ACK timeout logic.
 *
 * Applicable only if raw link layer API is enabled
 * (i.e., `OPENTHREAD_CONFIG_LINK_RAW_ENABLE` is set).
 *
 */
#define OPENTHREAD_CONFIG_SOFTWARE_ACK_TIMEOUT_ENABLE           1

/**
 * @def OPENTHREAD_CONFIG_SOFTWARE_RETRANSMIT_ENABLE
 *
 * Define to 1 to enable software retransmission logic.
 *
 * Applicable only if raw link layer API is enabled
 * (i.e., `OPENTHREAD_CONFIG_LINK_RAW_ENABLE` is set).
 *
 */
#define OPENTHREAD_CONFIG_SOFTWARE_RETRANSMIT_ENABLE            1

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
#define SETTINGS_CONFIG_PAGE_SIZE          DT_FLASH_ERASE_BLOCK_SIZE

/**
 * @def SETTINGS_CONFIG_PAGE_NUM
 *
 * The page number of settings.
 *
 */
#define SETTINGS_CONFIG_PAGE_NUM                                4

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
 *
 * Define to 1 if you want to enable microsecond backoff timer implemented
 * in platform.
 *
 */
#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE            0

/* Zephyr does not use OpenThreads heap. mbedTLS will use heap memory allocated
 * by Zephyr. Here, we use some dummy values to prevent OpenThread warnings.
 */

/**
 * @def OPENTHREAD_CONFIG_HEAP_SIZE
 *
 * The size of heap buffer when DTLS is enabled.
 *
 */
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE (4 * sizeof(void *))

/**
 * @def OPENTHREAD_CONFIG_HEAP_SIZE_NO_DTLS
 *
 * The size of heap buffer when DTLS is disabled.
 *
 */
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE_NO_DTLS (4 * sizeof(void *))

/* Disable software srouce address matching. */

/**
 * @def RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM
 *
 * The number of short source address table entries.
 *
 */
#define RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM 0

/**
 * @def RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM
 *
 * The number of extended source address table entries.
 *
 */
#define RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM 0

#endif  /* OPENTHREAD_CORE_NRF52840_CONFIG_H_ */
