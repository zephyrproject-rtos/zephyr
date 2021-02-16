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

#include <autoconf.h>
#include <devicetree.h>
#include <toolchain.h>

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 */
#ifdef CONFIG_OPENTHREAD_NUM_MESSAGE_BUFFERS
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS                                  \
	CONFIG_OPENTHREAD_NUM_MESSAGE_BUFFERS
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers
 * (set using `otSetStateChangedCallback()`).
 *
 */
#ifdef CONFIG_OPENTHREAD_MAX_STATECHANGE_HANDLERS
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS                             \
	CONFIG_OPENTHREAD_MAX_STATECHANGE_HANDLERS
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#ifdef CONFIG_OPENTHREAD_TMF_ADDRESS_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES                            \
	CONFIG_OPENTHREAD_TMF_ADDRESS_CACHE_ENTRIES
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
 *
 * Define to prepend the log level to all log messages.
 *
 */
#ifdef CONFIG_OPENTHREAD_LOG_PREPEND_LEVEL_ENABLE
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE
 *
 * Define to 1 to enable software ACK timeout logic.
 *
 */
#ifdef CONFIG_OPENTHREAD_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE
 *
 * Define to 1 to enable software retransmission logic.
 *
 */
#ifdef CONFIG_OPENTHREAD_MAC_SOFTWARE_RETRANSMIT_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE
 *
 * Define to 1 if you want to enable software CSMA-CA backoff logic.
 *
 */
#ifdef CONFIG_OPENTHREAD_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
 *
 * Define to 1 if you want to enable microsecond backoff timer implemented
 * in platform.
 *
 */
#ifdef CONFIG_OPENTHREAD_PLATFORM_USEC_TIMER_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE 1
#endif

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
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#ifdef CONFIG_OPENTHREAD_CONFIG_PLATFORM_INFO
#define OPENTHREAD_CONFIG_PLATFORM_INFO CONFIG_OPENTHREAD_CONFIG_PLATFORM_INFO
#endif /* CONFIG_OPENTHREAD_CONFIG_PLATFORM_INFO */

/**
 * @def OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#ifdef CONFIG_OPENTHREAD_MAX_CHILDREN
#define OPENTHREAD_CONFIG_MLE_MAX_CHILDREN CONFIG_OPENTHREAD_MAX_CHILDREN
#endif /* CONFIG_OPENTHREAD_MAX_CHILDREN */

/**
 * @def OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD
 *
 * The maximum number of supported IPv6 address registrations per child.
 *
 */
#ifdef CONFIG_OPENTHREAD_MAX_IP_ADDR_PER_CHILD
#define OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD \
	CONFIG_OPENTHREAD_MAX_IP_ADDR_PER_CHILD
#endif /* CONFIG_OPENTHREAD_MAX_IP_ADDR_PER_CHILD */

/**
 * @def RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM
 *
 * The number of extended source address table entries.
 *
 */
#define RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM 0

/**
 * @def OPENTHREAD_CONFIG_PLAT_LOG_MACRO_NAME
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

#define OPENTHREAD_CONFIG_PLAT_LOG_MACRO_NAME__COUNT_ARGS(aLogLevel, unused, \
							aFormat, ...) \
	do { \
		ARG_UNUSED(unused); \
		otPlatLog( \
		  aLogLevel, \
		  (otLogRegion)_OT_CONF_PLAT_LOG_FUN_NARGS__GET(__VA_ARGS__),\
		  aFormat, ##__VA_ARGS__); \
	} while (false)

#ifdef OPENTHREAD_CONFIG_PLAT_LOG_MACRO_NAME
#error OPENTHREAD_CONFIG_PLAT_LOG_MACRO_NAME \
	"OPENTHREAD_CONFIG_PLAT_LOG_MACRO_NAME mustn't be defined before"
#endif

#define OPENTHREAD_CONFIG_PLAT_LOG_MACRO_NAME \
	OPENTHREAD_CONFIG_PLAT_LOG_MACRO_NAME__COUNT_ARGS

/**
 * @def OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE
 *
 * Set to 1 to enable support for IEEE802.15.4 radio link.
 *
 */
#ifdef CONFIG_OPENTHREAD_RADIO_LINK_IEEE_802_15_4_ENABLE
#define OPENTHREAD_CONFIG_RADIO_LINK_IEEE_802_15_4_ENABLE \
	CONFIG_OPENTHREAD_RADIO_LINK_IEEE_802_15_4_ENABLE
#endif /* CONFIG_OPENTHREAD_RADIO_LINK_IEEE_802_15_4_ENABLE */

/**
 * @def OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE
 *
 * Set to 1 to enable support for Thread Radio Encapsulation Link (TREL).
 *
 */
#ifdef CONFIG_OPENTHREAD_RADIO_LINK_TREL_ENABLE
#define OPENTHREAD_CONFIG_RADIO_LINK_TREL_ENABLE \
	CONFIG_OPENTHREAD_RADIO_LINK_TREL_ENABLE
#endif /* CONFIG_OPENTHREAD_RADIO_LINK_TREL_ENABLE */

/**
 * @def OPENTHREAD_CONFIG_CSL_SAMPLE_WINDOW
 *
 * CSL sample window in units of 10 symbols.
 *
 */
#ifdef CONFIG_OPENTHREAD_CSL_SAMPLE_WINDOW
#define OPENTHREAD_CONFIG_CSL_SAMPLE_WINDOW \
	CONFIG_OPENTHREAD_CSL_SAMPLE_WINDOW
#endif /* CONFIG_OPENTHREAD_CSL_SAMPLE_WINDOW */

/**
 * @def OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD
 *
 * For some reasons, CSL receivers wake up a little later than expected. This
 * variable specifies how much time that CSL receiver would wake up earlier
 * than the expected sample window. The time is in unit of 10 symbols.
 *
 */
#ifdef CONFIG_OPENTHREAD_CSL_RECEIVE_TIME_AHEAD
#define OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD \
	CONFIG_OPENTHREAD_CSL_RECEIVE_TIME_AHEAD
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVE_TIME_AHEAD */

#endif  /* OPENTHREAD_CORE_ZEPHYR_CONFIG_H_ */
