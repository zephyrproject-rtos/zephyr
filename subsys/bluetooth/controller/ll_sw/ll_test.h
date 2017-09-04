/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

u32_t ll_test_tx(u8_t chan, u8_t len, u8_t type, u8_t phy);
u32_t ll_test_rx(u8_t chan, u8_t phy, u8_t mod_idx);
u32_t ll_test_end(u16_t *num_rx);
