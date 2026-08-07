/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void bt_mesh_sol_reset(void);

void bt_mesh_sol_recv(struct net_buf_simple *buf, uint8_t uuid_list_len);

void bt_mesh_srpl_entry_clear(uint16_t addr);

void bt_mesh_srpl_pending_store(void);

void bt_mesh_sseq_pending_store(void);

int bt_mesh_sol_send(void);
