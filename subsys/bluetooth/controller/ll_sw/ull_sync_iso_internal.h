/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_sync_iso_init(void);
int ull_sync_iso_reset(void);
struct ll_sync_iso *ull_sync_iso_get(uint8_t handle);
uint8_t ull_sync_iso_handle_get(struct ll_sync_iso *sync);
uint8_t ull_sync_iso_lll_handle_get(struct lll_sync_iso *lll);
void ull_sync_iso_release(struct ll_sync_iso *sync_iso);
void ull_sync_iso_setup(struct ll_sync_iso *sync_iso,
			struct node_rx_hdr *node_rx,
			struct pdu_big_info *big_info);
