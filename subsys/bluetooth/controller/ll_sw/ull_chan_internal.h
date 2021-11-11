/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_chan_reset(void);
uint8_t ull_chan_map_get(uint8_t *const chan_map);
#if !defined(CONFIG_BT_LL_SW_LLCP_LEGACY)
void ull_chan_map_set(uint8_t const *const chan_map);
#endif
