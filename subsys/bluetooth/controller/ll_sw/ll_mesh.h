/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

u8_t ll_mesh_advertise(u8_t handle, u8_t own_addr_type,
		       u8_t const *const rand_addr,
		       u8_t chan_map, u8_t tx_pwr,
		       u8_t min_tx_delay, u8_t max_tx_delay,
		       u8_t retry, u8_t interval,
		       u8_t scan_window, u8_t scan_delay, u8_t scan_filter,
		       u8_t data_len, u8_t const *const data);
u8_t ll_mesh_advertise_cancel(u8_t handle);
