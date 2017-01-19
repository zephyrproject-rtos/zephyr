/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LL_H_
#define _LL_H_

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
uint32_t ll_create_connection(uint16_t scan_interval, uint16_t scan_window,
			      uint8_t filter_policy, uint8_t peer_addr_type,
			      uint8_t *p_peer_addr, uint8_t own_addr_type,
			      uint16_t interval, uint16_t latency,
			      uint16_t timeout);

#endif /* _LL_H_ */
