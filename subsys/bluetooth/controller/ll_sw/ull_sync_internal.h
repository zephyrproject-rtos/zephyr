/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_sync_init(void);
int ull_sync_reset(void);
uint16_t ull_sync_handle_get(struct ll_sync_set *sync);
struct ll_sync_set *ull_sync_is_enabled_get(uint16_t handle);
void ull_sync_release(struct ll_sync_set *sync);
void ull_sync_setup(struct ll_scan_set *scan, struct ll_scan_aux_set *aux,
		    struct node_rx_hdr *node_rx, struct pdu_adv_sync_info *si);
void ull_sync_done(struct node_rx_event_done *done);
int ull_sync_slot_update(struct ll_sync_set *sync, uint32_t slot_plus_us,
			 uint32_t slot_minus_us);
