/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

uint32_t ll_test_tx(uint8_t chan, uint8_t len, uint8_t type, uint8_t phy);
uint32_t ll_test_rx(uint8_t chan, uint8_t phy, uint8_t mod_idx);
uint32_t ll_test_end(uint16_t *num_rx);
