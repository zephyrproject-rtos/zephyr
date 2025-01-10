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

#include <zephyr/devicetree.h>
#include <zephyr/psa/key_ids.h>
#include <zephyr/toolchain.h>

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
 *
 * The assert is managed by platform defined logic when this flag is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
#define OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT 1
#endif

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
 * @def CONFIG_OPENTHREAD_TMF_ADDRESS_CACHE_MAX_SNOOP_ENTRIES
 *
 * The maximum number of EID-to-RLOC cache entries that can be used for
 * "snoop optimization" where an entry is created by inspecting a received message.
 *
 */
#ifdef CONFIG_OPENTHREAD_TMF_ADDRESS_CACHE_MAX_SNOOP_ENTRIES
#define OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_MAX_SNOOP_ENTRIES                  \
	CONFIG_OPENTHREAD_TMF_ADDRESS_CACHE_MAX_SNOOP_ENTRIES
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
 * @def OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
 *
 * Define as 1 for a child to inform its previous parent when it attaches to a new parent.
 *
 */
#ifdef CONFIG_OPENTHREAD_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH
#define OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH 1
#endif

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE
 *
 * Define as 1 to enable periodic parent search feature.
 *
 */
#ifdef CONFIG_OPENTHREAD_PARENT_SEARCH
#define OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE 1

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL
 *
 * Specifies the interval in seconds for a child to check the trigger condition
 * to perform a parent search.
 *
 */
#define OPENTHREAD_CONFIG_PARENT_SEARCH_CHECK_INTERVAL                         \
	CONFIG_OPENTHREAD_PARENT_SEARCH_CHECK_INTERVAL

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL
 *
 * Specifies the backoff interval in seconds for a child to not perform a parent
 * search after triggering one.
 *
 */
#define OPENTHREAD_CONFIG_PARENT_SEARCH_BACKOFF_INTERVAL                       \
	CONFIG_OPENTHREAD_PARENT_SEARCH_BACKOFF_INTERVAL

/**
 * @def OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD
 *
 * Specifies the RSS threshold used to trigger a parent search.
 *
 */
#define OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD                          \
	CONFIG_OPENTHREAD_PARENT_SEARCH_RSS_THRESHOLD
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE
 *
 * Define to 1 to enable software transmission target time logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE                        \
	(OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE
 *
 * Define to 1 to enable software reception target time logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RX_TIMING_ENABLE                        \
	(OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
 *
 * Define as 1 to support IEEE 802.15.4-2015 Header IE (Information Element) generation and parsing,
 * it must be set to support following features:
 *    1. Time synchronization service feature (i.e., OPENTHREAD_CONFIG_TIME_SYNC_ENABLE is set).
 *    2. Thread 1.2.
 *
 * @note If it's enabled, platform must support interrupt context and concurrent access AES.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT
#if OPENTHREAD_CONFIG_TIME_SYNC_ENABLE ||                                      \
	(OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#define OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT 1
#else
#define OPENTHREAD_CONFIG_MAC_HEADER_IE_SUPPORT 0
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
 *
 * Define to 1 if you want to enable microsecond backoff timer implemented
 * in platform.
 *
 */
#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE                                               \
	((CONFIG_OPENTHREAD_CSL_RECEIVER &&                                                        \
	  (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)) ||                          \
	 CONFIG_OPENTHREAD_WAKEUP_END_DEVICE)

/* Zephyr does not use OpenThread's heap. mbedTLS will use heap memory allocated
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

/* Disable software source address matching. */

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
 * @def OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US
 *
 * Define how many microseconds ahead should MAC deliver CSL frame to SubMac.
 *
 */
#ifdef CONFIG_OPENTHREAD_CSL_REQUEST_TIME_AHEAD
#define OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US CONFIG_OPENTHREAD_CSL_REQUEST_TIME_AHEAD
#endif /* CONFIG_OPENTHREAD_CSL_REQUEST_TIME_AHEAD */

/**
 * @def OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD
 *
 * For some reasons, CSL receivers wake up a little later than expected. This
 * variable specifies how much time that CSL receiver would wake up earlier
 * than the expected sample window. The time is in unit of microseconds.
 *
 */
#ifdef CONFIG_OPENTHREAD_CSL_RECEIVE_TIME_AHEAD
#define OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD \
	CONFIG_OPENTHREAD_CSL_RECEIVE_TIME_AHEAD
#endif /* CONFIG_OPENTHREAD_CSL_RECEIVE_TIME_AHEAD */

/**
 * @def OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD
 *
 * The minimum time (microseconds) that radio has to be in receive mode before the start of the MHR.
 *
 */
#ifdef CONFIG_OPENTHREAD_MIN_RECEIVE_ON_AHEAD
#define OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD CONFIG_OPENTHREAD_MIN_RECEIVE_ON_AHEAD
#endif /* CONFIG_OPENTHREAD_MIN_RECEIVE_ON_AHEAD */

/**
 * @def OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER
 *
 * The minimum time (microseconds) that radio has to be in receive mode after the start of the MHR .
 *
 */
#ifdef CONFIG_OPENTHREAD_MIN_RECEIVE_ON_AFTER
#define OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER CONFIG_OPENTHREAD_MIN_RECEIVE_ON_AFTER
#endif /* CONFIG_OPENTHREAD_MIN_RECEIVE_ON_AFTER */

/**
 * @def OPENTHREAD_CONFIG_CSL_TIMEOUT
 *
 * The default CSL timeout in seconds.
 *
 */
#ifdef CONFIG_OPENTHREAD_CSL_TIMEOUT
#define OPENTHREAD_CONFIG_CSL_TIMEOUT CONFIG_OPENTHREAD_CSL_TIMEOUT
#endif /* CONFIG_OPENTHREAD_CSL_TIMEOUT */

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
 *
 * Set to 1 to enable software transmission security logic.
 *
 */
#ifdef CONFIG_OPENTHREAD_MAC_SOFTWARE_TX_SECURITY_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE                                          \
	CONFIG_OPENTHREAD_MAC_SOFTWARE_TX_SECURITY_ENABLE
#endif /* CONFIG_OPENTHREAD_MAC_SOFTWARE_TX_SECURITY_ENABLE */

/**
 * @def OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
 *
 * The maximum size of the CLI line in bytes.
 *
 */
#ifdef CONFIG_OPENTHREAD_CLI_MAX_LINE_LENGTH
#define OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH CONFIG_OPENTHREAD_CLI_MAX_LINE_LENGTH
#endif /* CONFIG_OPENTHREAD_CLI_MAX_LINE_LENGTH */

/**
 * @def OPENTHREAD_CONFIG_CLI_PROMPT_ENABLE
 *
 * Enable CLI prompt.
 *
 * When enabled, the CLI will print prompt on the output after processing a command.
 * Otherwise, no prompt is added to the output.
 *
 */
#define OPENTHREAD_CONFIG_CLI_PROMPT_ENABLE 0

/**
 * @def OPENTHREAD_CONFIG_IP6_MAX_EXT_UCAST_ADDRS
 *
 * The maximum number of supported IPv6 addresses allows to be externally added.
 *
 */
#ifdef CONFIG_OPENTHREAD_IP6_MAX_EXT_UCAST_ADDRS
#define OPENTHREAD_CONFIG_IP6_MAX_EXT_UCAST_ADDRS CONFIG_OPENTHREAD_IP6_MAX_EXT_UCAST_ADDRS
#endif /* CONFIG_OPENTHREAD_IP6_MAX_EXT_UCAST_ADDRS */

/**
 * @def OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS
 *
 * The maximum number of supported IPv6 multicast addresses allows to be externally added.
 *
 */
#ifdef CONFIG_OPENTHREAD_IP6_MAX_EXT_MCAST_ADDRS
#define OPENTHREAD_CONFIG_IP6_MAX_EXT_MCAST_ADDRS CONFIG_OPENTHREAD_IP6_MAX_EXT_MCAST_ADDRS
#endif /* CONFIG_OPENTHREAD_IP6_MAX_EXT_MCAST_ADDRS */

/**
 * @def OPENTHREAD_CONFIG_CLI_TCP_ENABLE
 *
 * Enable TCP in the CLI tool.
 *
 */
#define OPENTHREAD_CONFIG_CLI_TCP_ENABLE IS_ENABLED(CONFIG_OPENTHREAD_CLI_TCP_ENABLE)

/**
 * @def OPENTHREAD_CONFIG_CRYPTO_LIB
 *
 * Selects crypto backend library for OpenThread.
 *
 */
#ifdef CONFIG_OPENTHREAD_CRYPTO_PSA
#define OPENTHREAD_CONFIG_CRYPTO_LIB OPENTHREAD_CONFIG_CRYPTO_LIB_PSA
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_MAC_KEYS_EXPORTABLE_ENABLE
 *
 * Set to 1 if you want to make MAC keys exportable.
 *
 */
#ifdef CONFIG_OPENTHREAD_PLATFORM_KEYS_EXPORTABLE_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_MAC_KEYS_EXPORTABLE_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE
 *
 * The size of a message buffer in bytes.
 *
 */
#ifdef CONFIG_OPENTHREAD_MESSAGE_BUFFER_SIZE
#define OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE CONFIG_OPENTHREAD_MESSAGE_BUFFER_SIZE
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT
 *
 * The message pool is managed by platform defined logic.
 *
 */
#ifdef CONFIG_OPENTHREAD_PLATFORM_MESSAGE_MANAGEMENT
#define OPENTHREAD_CONFIG_PLATFORM_MESSAGE_MANAGEMENT CONFIG_OPENTHREAD_PLATFORM_MESSAGE_MANAGEMENT
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
 *
 * Enable to stay awake between fragments while transmitting a large packet,
 * and to stay awake after receiving a packet with frame pending set to true.
 *
 */
#ifdef CONFIG_OPENTHREAD_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
#define OPENTHREAD_CONFIG_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS                                         \
	CONFIG_OPENTHREAD_MAC_STAY_AWAKE_BETWEEN_FRAGMENTS
#endif

/**
 * @def OPENTHREAD_CONFIG_POWER_CALIBRATION_ENABLE
 *
 * In Zephyr, power calibration is handled by Radio Driver, so it can't be handled on OT level.
 *
 */
#ifndef OPENTHREAD_CONFIG_POWER_CALIBRATION_ENABLE
#define OPENTHREAD_CONFIG_POWER_CALIBRATION_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_RADIO_STATS
 *
 * Enable support for Radio Statistics.
 *
 */
#ifdef CONFIG_OPENTHREAD_RADIO_STATS
#define OPENTHREAD_CONFIG_RADIO_STATS_ENABLE CONFIG_OPENTHREAD_RADIO_STATS
#endif

/**
 * @def OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD
 *
 * The value ahead of the current frame counter for persistent storage.
 *
 */
#ifdef CONFIG_OPENTHREAD_STORE_FRAME_COUNTER_AHEAD
#define OPENTHREAD_CONFIG_STORE_FRAME_COUNTER_AHEAD CONFIG_OPENTHREAD_STORE_FRAME_COUNTER_AHEAD
#endif

/**
 * @def OPENTHREAD_CONFIG_CHILD_SUPERVISION_CHECK_TIMEOUT
 *
 * The value of the child supervision check timeout in seconds.
 *
 */
#ifdef CONFIG_OPENTHREAD_CHILD_SUPERVISION_CHECK_TIMEOUT
#define OPENTHREAD_CONFIG_CHILD_SUPERVISION_CHECK_TIMEOUT                                          \
	CONFIG_OPENTHREAD_CHILD_SUPERVISION_CHECK_TIMEOUT
#endif

/**
 * @def OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL
 *
 * The value of the child supervision interval in seconds.
 *
 */
#ifdef CONFIG_OPENTHREAD_CHILD_SUPERVISION_INTERVAL
#define OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL CONFIG_OPENTHREAD_CHILD_SUPERVISION_INTERVAL
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT
 *
 * The value of the MLE child timeout in seconds.
 *
 */
#ifdef CONFIG_OPENTHREAD_MLE_CHILD_TIMEOUT
#define OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT CONFIG_OPENTHREAD_MLE_CHILD_TIMEOUT
#endif

/**
 * @def OPENTHREAD_CONFIG_PSA_ITS_NVM_OFFSET
 *
 * NVM offset while using key refs.
 *
 */
#define OPENTHREAD_CONFIG_PSA_ITS_NVM_OFFSET ZEPHYR_PSA_OPENTHREAD_KEY_ID_RANGE_BEGIN

#endif  /* OPENTHREAD_CORE_ZEPHYR_CONFIG_H_ */
