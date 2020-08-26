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

#include <devicetree.h>
#include <toolchain.h>

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
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE
 *
 * Define to 1 to enable software ACK timeout logic.
 *
 */
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE       1

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE
 *
 * Define to 1 to enable software retransmission logic.
 *
 */
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE        1

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
#define SETTINGS_CONFIG_PAGE_SIZE \
	DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)

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

/**
 * @def OPENTHREAD_CONFIG_NCP_BUFFER_SIZE
 *
 * The size of the NCP buffers.
 *
 */
#define OPENTHREAD_CONFIG_NCP_BUFFER_SIZE 2048

/**
 * @def OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION
 *
 * The platform logging function for openthread.
 *
 */
#define _OT_CONF_PLAT_LOG_FUN_NARGS__IMPL(		\
		_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,\
		_11, _12, _13, _14, N, ...) N

#define _OT_CONF_PLAT_LOG_FUN_NARGS__GET(...) \
		_OT_CONF_PLAT_LOG_FUN_NARGS__IMPL(__VA_ARGS__,\
		15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, ~)

#define OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION__COUNT_ARGS(aLogLevel, unused, \
							aFormat, ...) \
	do { \
		ARG_UNUSED(unused); \
		otPlatLog( \
		  aLogLevel, \
		  (otLogRegion)_OT_CONF_PLAT_LOG_FUN_NARGS__GET(__VA_ARGS__),\
		  aFormat, ##__VA_ARGS__); \
	} while (false)

#ifdef OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION
#error OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION \
	"OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION mustn't be defined before"
#endif

#define OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION \
	OPENTHREAD_CONFIG_PLAT_LOG_FUNCTION__COUNT_ARGS

#endif  /* OPENTHREAD_CORE_NRF52840_CONFIG_H_ */
