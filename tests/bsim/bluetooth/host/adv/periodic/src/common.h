/**
 * Common functions and helpers
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_
#define ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_

#include <stdint.h>

#include <zephyr/bluetooth/gap.h>

extern uint8_t mfg_data[254];

/* per_adv_sync_term_leak: a terminate while the sync's chain is still being received strands the
 * auxiliary context. On nrf52_bsim the chain (two AUX_CHAIN_IND on 2M) ends 1.7 ms after the
 * synced callback, and terminates from the callback to then strand it. The scanner terminates
 * 0 to TERM_LEAK_SPAN_US after the callback, one step later on each sync, and syncs once per
 * periodic interval (BT_LE_PER_ADV_DEFAULT: at most BT_GAP_PER_ADV_SLOW_INT_MAX).
 */
#define TERM_LEAK_SPAN_US     2000U
#define TERM_LEAK_STEP_US     100U
#define TERM_LEAK_ITERATIONS  (TERM_LEAK_SPAN_US / TERM_LEAK_STEP_US + 1U)
#define TERM_LEAK_INTERVAL_MS BT_GAP_PER_ADV_INTERVAL_TO_MS(BT_GAP_PER_ADV_SLOW_INT_MAX)
/* A sync establishes at the next AUX_SYNC_IND; two intervals allow one missed */
#define TERM_LEAK_TIMEOUT_MS  (2U * TERM_LEAK_INTERVAL_MS)
/* The advertiser outlives the sweep: one interval for the scan that finds it, one per iteration */
#define TERM_LEAK_ADV_MS      ((TERM_LEAK_ITERATIONS + 1U) * TERM_LEAK_INTERVAL_MS)

#endif /* ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_ */
