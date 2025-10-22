/** @file
 * @brief Various priority to traffic class mappings
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TC_MAPPING_H
#define __TC_MAPPING_H

#include "zephyr/net/net_core.h"

/* All the maps below use priorities and indexes, below is the list of them
 * according to 802.1Q - table I-2.
 *
 *  Priority         Acronym   Traffic types
 *  1 (lowest)       BK        Background
 *  0 (default)      BE        Best effort
 *  2                EE        Excellent effort
 *  3                CA        Critical applications
 *  4                VI        Video, < 100 ms latency and jitter
 *  5                VO        Voice, < 10 ms latency and jitter
 *  6                IC        Internetwork control
 *  7 (highest)      NC        Network control
 */


/* This is the recommended priority to traffic class mapping for
 * implementations that do not support the credit-based shaper transmission
 * selection algorithm.
 * Ref: 802.1Q - chapter 8.6.6 - table 8-4
 */
#if defined(CONFIG_NET_TC_MAPPING_STRICT)
#define PRIORITY2TC_1 {0, 0, 0, 0, 0, 0, 0, 0}
#define PRIORITY2TC_2 {0, 0, 0, 0, 1, 1, 1, 1}
#define PRIORITY2TC_3 {0, 0, 0, 0, 1, 1, 2, 2}
#define PRIORITY2TC_4 {0, 0, 1, 1, 2, 2, 3, 3}
#define PRIORITY2TC_5 {0, 0, 1, 1, 2, 2, 3, 4}
#define PRIORITY2TC_6 {1, 0, 2, 2, 3, 3, 4, 5}
#define PRIORITY2TC_7 {1, 0, 2, 3, 4, 4, 5, 6}
#define PRIORITY2TC_8 {1, 0, 2, 3, 4, 5, 6, 7}


/* This is the recommended priority to traffic class mapping for a system that
 * supports SR (Stream Reservation) class A and SR class B.
 * Ref: 802.1Q - chapter 34.5 - table 34-1
 */
#elif defined(CONFIG_NET_TC_MAPPING_SR_CLASS_A_AND_B)
#define PRIORITY2TC_1 {0, 0, 0, 0, 0, 0, 0, 0}
#define PRIORITY2TC_2 {0, 0, 1, 1, 0, 0, 0, 0}
#define PRIORITY2TC_3 {0, 0, 1, 2, 0, 0, 0, 0}
#define PRIORITY2TC_4 {0, 0, 2, 3, 1, 1, 1, 1}
#define PRIORITY2TC_5 {0, 0, 3, 4, 1, 1, 2, 2}
#define PRIORITY2TC_6 {0, 0, 4, 5, 1, 1, 2, 3}
#define PRIORITY2TC_7 {0, 0, 5, 6, 1, 2, 3, 4}
#define PRIORITY2TC_8 {1, 0, 6, 7, 2, 3, 4, 5}


/* This is the recommended priority to traffic class mapping for a system that
 * supports SR (Stream Reservation) class B only.
 * Ref: 802.1Q - chapter 34.5 - table 34-2
 */
#elif defined(CONFIG_NET_TC_MAPPING_SR_CLASS_B_ONLY)
#define PRIORITY2TC_1 {0, 0, 0, 0, 0, 0, 0, 0}
#define PRIORITY2TC_2 {0, 0, 1, 0, 0, 0, 0, 0}
#define PRIORITY2TC_3 {0, 0, 2, 0, 1, 1, 1, 1}
#define PRIORITY2TC_4 {0, 0, 3, 0, 1, 1, 2, 2}
#define PRIORITY2TC_5 {0, 0, 4, 1, 2, 2, 3, 3}
#define PRIORITY2TC_6 {0, 0, 5, 1, 2, 2, 3, 4}
#define PRIORITY2TC_7 {1, 0, 6, 2, 3, 3, 4, 5}
#define PRIORITY2TC_8 {1, 0, 7, 2, 3, 4, 5, 6}
#endif


#if NET_TC_TX_EFFECTIVE_COUNT == 0
#elif NET_TC_TX_EFFECTIVE_COUNT == 1
#define PRIORITY2TC_TX PRIORITY2TC_1
#elif NET_TC_TX_EFFECTIVE_COUNT == 2
#define PRIORITY2TC_TX PRIORITY2TC_2
#elif NET_TC_TX_EFFECTIVE_COUNT == 3
#define PRIORITY2TC_TX PRIORITY2TC_3
#elif NET_TC_TX_EFFECTIVE_COUNT == 4
#define PRIORITY2TC_TX PRIORITY2TC_4
#elif NET_TC_TX_EFFECTIVE_COUNT == 5
#define PRIORITY2TC_TX PRIORITY2TC_5
#elif NET_TC_TX_EFFECTIVE_COUNT == 6
#define PRIORITY2TC_TX PRIORITY2TC_6
#elif NET_TC_TX_EFFECTIVE_COUNT == 7
#define PRIORITY2TC_TX PRIORITY2TC_7
#elif NET_TC_TX_EFFECTIVE_COUNT == 8
#define PRIORITY2TC_TX PRIORITY2TC_8
#else
BUILD_ASSERT(false, "Too many effective tx traffic class queues, either reduce "
		     "CONFIG_NET_TC_TX_COUNT or disable "
		     "CONFIG_NET_TC_TX_SKIP_FOR_HIGH_PRIO");
#endif


#if NET_TC_RX_EFFECTIVE_COUNT == 0
#elif NET_TC_RX_EFFECTIVE_COUNT == 1
#define PRIORITY2TC_RX PRIORITY2TC_1
#elif NET_TC_RX_EFFECTIVE_COUNT == 2
#define PRIORITY2TC_RX PRIORITY2TC_2
#elif NET_TC_RX_EFFECTIVE_COUNT == 3
#define PRIORITY2TC_RX PRIORITY2TC_3
#elif NET_TC_RX_EFFECTIVE_COUNT == 4
#define PRIORITY2TC_RX PRIORITY2TC_4
#elif NET_TC_RX_EFFECTIVE_COUNT == 5
#define PRIORITY2TC_RX PRIORITY2TC_5
#elif NET_TC_RX_EFFECTIVE_COUNT == 6
#define PRIORITY2TC_RX PRIORITY2TC_6
#elif NET_TC_RX_EFFECTIVE_COUNT == 7
#define PRIORITY2TC_RX PRIORITY2TC_7
#elif NET_TC_RX_EFFECTIVE_COUNT == 8
#define PRIORITY2TC_RX PRIORITY2TC_8
#else
BUILD_ASSERT(false, "Too many effective rx traffic class queues, either reduce "
		     "CONFIG_NET_TC_RX_COUNT or disable "
		     "CONFIG_NET_TC_RX_SKIP_FOR_HIGH_PRIO");
#endif

#endif /* __TC_MAPPING_H */
