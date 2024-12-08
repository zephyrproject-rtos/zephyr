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
void ull_sync_setup_addr_check(struct ll_sync_set *sync, struct ll_scan_set *scan,
			       uint8_t addr_type, uint8_t *addr, uint8_t rl_idx);
bool ull_sync_setup_sid_match(struct ll_sync_set *sync, struct ll_scan_set *scan, uint8_t sid);
void ull_sync_create_from_sync_transfer(uint16_t conn_handle,  uint16_t service_data,
					struct ll_sync_set *sync,
					struct pdu_adv_sync_info *si,
					uint32_t conn_offset_us);
void ull_sync_setup(struct ll_scan_set *scan, uint8_t phy,
		    struct node_rx_pdu *node_rx, struct pdu_adv_sync_info *si);
void ull_sync_setup_reset(struct ll_sync_set *sync);
void ull_sync_established_report(memq_link_t *link, struct node_rx_pdu *rx);
void ull_sync_done(struct node_rx_event_done *done);
void ull_sync_chm_update(uint8_t sync_handle, uint8_t *acad, uint8_t acad_len);
int ull_sync_slot_update(struct ll_sync_set *sync, uint32_t slot_plus_us,
			 uint32_t slot_minus_us);
struct ll_sync_set *ull_sync_is_valid_get(struct ll_sync_set *sync);
void ull_sync_transfer_received(struct ll_conn *conn, uint16_t service_data,
				struct pdu_adv_sync_info *si, uint16_t conn_event_count,
				uint16_t last_pa_event_counter, uint8_t sid,
				uint8_t addr_type, uint8_t sca, uint8_t phy,
				uint8_t *adv_addr, uint16_t sync_conn_event_count,
				uint8_t addr_resolved);
