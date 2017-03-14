/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LL_H_
#define _LL_H_

void ll_reset(void);
void ll_address_get(uint8_t addr_type, uint8_t *p_bdaddr);
void ll_address_set(uint8_t addr_type, uint8_t const *const p_bdaddr);
void ll_adv_params_set(uint16_t interval, uint8_t adv_type,
		       uint8_t own_addr_type, uint8_t direct_addr_type,
		       uint8_t const *const p_direct_addr, uint8_t chl_map,
		       uint8_t filter_policy);
void ll_adv_data_set(uint8_t len, uint8_t const *const p_data);
void ll_scan_data_set(uint8_t len, uint8_t const *const p_data);
uint32_t ll_adv_enable(uint8_t enable);
void ll_scan_params_set(uint8_t scan_type, uint16_t interval, uint16_t window,
			uint8_t own_addr_type, uint8_t filter_policy);
uint32_t ll_scan_enable(uint8_t enable);
void ll_filter_clear(void);
uint32_t ll_filter_add(uint8_t addr_type, uint8_t *addr);
uint32_t ll_filter_remove(uint8_t addr_type, uint8_t *addr);

void ll_irk_clear(void);
uint32_t ll_irk_add(uint8_t *irk);
uint32_t ll_create_connection(uint16_t scan_interval, uint16_t scan_window,
			      uint8_t filter_policy, uint8_t peer_addr_type,
			      uint8_t *p_peer_addr, uint8_t own_addr_type,
			      uint16_t interval, uint16_t latency,
			      uint16_t timeout);
uint32_t ll_connect_disable(void);
uint32_t ll_conn_update(uint16_t handle, uint8_t cmd, uint8_t status,
			uint16_t interval, uint16_t latency,
			uint16_t timeout);
uint32_t ll_chm_update(uint8_t *chm);
uint32_t ll_chm_get(uint16_t handle, uint8_t *chm);
uint32_t ll_enc_req_send(uint16_t handle, uint8_t *rand, uint8_t *ediv,
			 uint8_t *ltk);
uint32_t ll_start_enc_req_send(uint16_t handle, uint8_t err_code,
			       uint8_t const *const ltk);
uint32_t ll_feature_req_send(uint16_t handle);
uint32_t ll_version_ind_send(uint16_t handle);
uint32_t ll_terminate_ind_send(uint16_t handle, uint8_t reason);

#if defined(CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH)
uint32_t ll_length_req_send(uint16_t handle, uint16_t tx_octets);
void ll_length_default_get(uint16_t *max_tx_octets, uint16_t *max_tx_time);
uint32_t ll_length_default_set(uint16_t max_tx_octets, uint16_t max_tx_time);
void ll_length_max_get(uint16_t *max_tx_octets, uint16_t *max_tx_time,
			  uint16_t *max_rx_octets, uint16_t *max_rx_time);
#endif /* CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH */

#endif /* _LL_H_ */
