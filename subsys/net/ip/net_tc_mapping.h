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

#include <zephyr/types.h>

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

/* Helper macros used to generate the map to use */
#define PRIORITY2TC_GEN_INNER(TYPE, COUNT) priority2tc_ ## TYPE ## _ ## COUNT
#define PRIORITY2TC_GEN(TYPE, COUNT) PRIORITY2TC_GEN_INNER(TYPE, COUNT)

#if defined(CONFIG_NET_TC_MAPPING_STRICT)

/* This is the recommended priority to traffic class mapping for
 * implementations that do not support the credit-based shaper transmission
 * selection algorithm.
 * Ref: 802.1Q - chapter 8.6.6 - table 8-4
 */

#if NET_TC_TX_COUNT == 1 || NET_TC_RX_COUNT == 1
static const uint8_t priority2tc_strict_1[] = {0, 0, 0, 0, 0, 0, 0, 0};
#endif
#if NET_TC_TX_COUNT == 2 || NET_TC_RX_COUNT == 2
static const uint8_t priority2tc_strict_2[] = {0, 0, 0, 0, 1, 1, 1, 1};
#endif
#if NET_TC_TX_COUNT == 3 || NET_TC_RX_COUNT == 3
static const uint8_t priority2tc_strict_3[] = {0, 0, 0, 0, 1, 1, 2, 2};
#endif
#if NET_TC_TX_COUNT == 4 || NET_TC_RX_COUNT == 4
static const uint8_t priority2tc_strict_4[] = {0, 0, 1, 1, 2, 2, 3, 3};
#endif
#if NET_TC_TX_COUNT == 5 || NET_TC_RX_COUNT == 5
static const uint8_t priority2tc_strict_5[] = {0, 0, 1, 1, 2, 2, 3, 4};
#endif
#if NET_TC_TX_COUNT == 6 || NET_TC_RX_COUNT == 6
static const uint8_t priority2tc_strict_6[] = {1, 0, 2, 2, 3, 3, 4, 5};
#endif
#if NET_TC_TX_COUNT == 7 || NET_TC_RX_COUNT == 7
static const uint8_t priority2tc_strict_7[] = {1, 0, 2, 3, 4, 4, 5, 6};
#endif
#if NET_TC_TX_COUNT == 8 || NET_TC_RX_COUNT == 8
static const uint8_t priority2tc_strict_8[] = {1, 0, 2, 3, 4, 5, 6, 7};
#endif

static const uint8_t *tx_prio2tc_map = PRIORITY2TC_GEN(strict, NET_TC_TX_COUNT);
static const uint8_t *rx_prio2tc_map = PRIORITY2TC_GEN(strict, NET_TC_RX_COUNT);

#elif defined(CONFIG_NET_TC_MAPPING_SR_CLASS_A_AND_B)

/* This is the recommended priority to traffic class mapping for a system that
 * supports SR (Stream Reservation) class A and SR class B.
 * Ref: 802.1Q - chapter 34.5 - table 34-1
 */

#if NET_TC_TX_COUNT == 2 || NET_TC_RX_COUNT == 2
static const uint8_t priority2tc_sr_ab_2[] = {0, 0, 1, 1, 0, 0, 0, 0};
#endif
#if NET_TC_TX_COUNT == 3 || NET_TC_RX_COUNT == 3
static const uint8_t priority2tc_sr_ab_3[] = {0, 0, 1, 2, 0, 0, 0, 0};
#endif
#if NET_TC_TX_COUNT == 4 || NET_TC_RX_COUNT == 4
static const uint8_t priority2tc_sr_ab_4[] = {0, 0, 2, 3, 1, 1, 1, 1};
#endif
#if NET_TC_TX_COUNT == 5 || NET_TC_RX_COUNT == 5
static const uint8_t priority2tc_sr_ab_5[] = {0, 0, 3, 4, 1, 1, 2, 2};
#endif
#if NET_TC_TX_COUNT == 6 || NET_TC_RX_COUNT == 6
static const uint8_t priority2tc_sr_ab_6[] = {0, 0, 4, 5, 1, 1, 2, 3};
#endif
#if NET_TC_TX_COUNT == 7 || NET_TC_RX_COUNT == 7
static const uint8_t priority2tc_sr_ab_7[] = {0, 0, 5, 6, 1, 2, 3, 4};
#endif
#if NET_TC_TX_COUNT == 8 || NET_TC_RX_COUNT == 8
static const uint8_t priority2tc_sr_ab_8[] = {1, 0, 6, 7, 2, 3, 4, 5};
#endif

static const uint8_t *tx_prio2tc_map = PRIORITY2TC_GEN(sr_ab, NET_TC_TX_COUNT);
static const uint8_t *rx_prio2tc_map = PRIORITY2TC_GEN(sr_ab, NET_TC_RX_COUNT);

#elif defined(CONFIG_NET_TC_MAPPING_SR_CLASS_B_ONLY)

/* This is the recommended priority to traffic class mapping for a system that
 * supports SR (Stream Reservation) class B only.
 * Ref: 802.1Q - chapter 34.5 - table 34-2
 */

#if NET_TC_TX_COUNT == 2 || NET_TC_RX_COUNT == 2
static const uint8_t priority2tc_sr_b_2[] = {0, 0, 1, 0, 0, 0, 0, 0};
#endif
#if NET_TC_TX_COUNT == 3 || NET_TC_RX_COUNT == 3
static const uint8_t priority2tc_sr_b_3[] = {0, 0, 2, 0, 1, 1, 1, 1};
#endif
#if NET_TC_TX_COUNT == 4 || NET_TC_RX_COUNT == 4
static const uint8_t priority2tc_sr_b_4[] = {0, 0, 3, 0, 1, 1, 2, 2};
#endif
#if NET_TC_TX_COUNT == 5 || NET_TC_RX_COUNT == 5
static const uint8_t priority2tc_sr_b_5[] = {0, 0, 4, 1, 2, 2, 3, 3};
#endif
#if NET_TC_TX_COUNT == 6 || NET_TC_RX_COUNT == 6
static const uint8_t priority2tc_sr_b_6[] = {0, 0, 5, 1, 2, 2, 3, 4};
#endif
#if NET_TC_TX_COUNT == 7 || NET_TC_RX_COUNT == 7
static const uint8_t priority2tc_sr_b_7[] = {1, 0, 6, 2, 3, 3, 4, 5};
#endif
#if NET_TC_TX_COUNT == 8 || NET_TC_RX_COUNT == 8
static const uint8_t priority2tc_sr_b_8[] = {1, 0, 7, 2, 3, 4, 5, 6};
#endif

static const uint8_t *tx_prio2tc_map = PRIORITY2TC_GEN(sr_b, NET_TC_TX_COUNT);
static const uint8_t *rx_prio2tc_map = PRIORITY2TC_GEN(sr_b, NET_TC_RX_COUNT);

#endif

#endif /* __TC_MAPPING_H */
