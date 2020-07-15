/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

uint16_t ull_conn_init(void);
struct ull_cp_conn *ll_conn_acquire(void);
struct ull_cp_conn *ll_conn_get(uint16_t handle);
uint16_t ll_conn_handle_get(struct ull_cp_conn *conn);
struct ull_cp_conn  *ll_connected_get(uint16_t handle);
void ll_conn_release(struct ull_cp_conn *conn);
uint16_t ll_conn_free(void);
void ll_conn_init_connection(struct ull_cp_conn *conn);

u8_t ull_conn_chan_map_cpy(u8_t *chan_map);
void ull_conn_chan_map_set(u8_t *chan_map);
