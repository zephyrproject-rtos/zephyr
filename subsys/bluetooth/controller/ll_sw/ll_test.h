/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

uint8_t ll_test_tx(uint8_t chan, uint8_t len, uint8_t type, uint8_t phy,
		   uint8_t cte_len, uint8_t cte_type, uint8_t switch_pattern_len,
		   const uint8_t *ant_id, int8_t tx_power);
uint8_t ll_test_rx(uint8_t chan, uint8_t phy, uint8_t mod_idx, uint8_t expected_cte_len,
		   uint8_t expected_cte_type, uint8_t slot_duration, uint8_t switch_pattern_len,
		   const uint8_t *ant_ids);
uint8_t ll_test_end(uint16_t *num_rx);
