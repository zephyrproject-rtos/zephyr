/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LL_H_
#define _LL_H_

int ll_init(struct k_sem *sem_rx);
void ll_reset(void);
u8_t *ll_addr_get(u8_t addr_type, u8_t *p_bdaddr);
void ll_addr_set(u8_t addr_type, u8_t const *const p_bdaddr);
u32_t ll_adv_params_set(u16_t interval, u8_t adv_type,
			u8_t own_addr_type, u8_t direct_addr_type,
			u8_t const *const p_direct_addr, u8_t chl_map,
			u8_t filter_policy);
void ll_adv_data_set(u8_t len, u8_t const *const p_data);
void ll_scan_data_set(u8_t len, u8_t const *const p_data);
u32_t ll_adv_enable(u8_t enable);
u32_t ll_scan_params_set(u8_t type, u16_t interval, u16_t window,
			 u8_t own_addr_type, u8_t filter_policy);
u32_t ll_scan_enable(u8_t enable);

u32_t ll_wl_size_get(void);
u32_t ll_wl_clear(void);
u32_t ll_wl_add(bt_addr_le_t *addr);
u32_t ll_wl_remove(bt_addr_le_t *addr);

void ll_irk_clear(void);
u32_t ll_irk_add(u8_t *irk);
u32_t ll_create_connection(u16_t scan_interval, u16_t scan_window,
			   u8_t filter_policy, u8_t peer_addr_type,
			   u8_t *p_peer_addr, u8_t own_addr_type,
			   u16_t interval, u16_t latency,
			   u16_t timeout);
u32_t ll_connect_disable(void);
u32_t ll_conn_update(u16_t handle, u8_t cmd, u8_t status,
		     u16_t interval, u16_t latency,
		     u16_t timeout);
u32_t ll_chm_update(u8_t *chm);
u32_t ll_chm_get(u16_t handle, u8_t *chm);
u32_t ll_enc_req_send(u16_t handle, u8_t *rand, u8_t *ediv,
		      u8_t *ltk);
u32_t ll_start_enc_req_send(u16_t handle, u8_t err_code,
			    u8_t const *const ltk);
u32_t ll_feature_req_send(u16_t handle);
u32_t ll_version_ind_send(u16_t handle);
u32_t ll_terminate_ind_send(u16_t handle, u8_t reason);

#if defined(CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH)
u32_t ll_length_req_send(u16_t handle, u16_t tx_octets);
void ll_length_default_get(u16_t *max_tx_octets, u16_t *max_tx_time);
u32_t ll_length_default_set(u16_t max_tx_octets, u16_t max_tx_time);
void ll_length_max_get(u16_t *max_tx_octets, u16_t *max_tx_time,
		       u16_t *max_rx_octets, u16_t *max_rx_time);
#endif /* CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PHY)
u32_t ll_phy_get(u16_t handle, u8_t *tx, u8_t *rx);
u32_t ll_phy_default_set(u8_t tx, u8_t rx);
u32_t ll_phy_req_send(u16_t handle, u8_t tx, u8_t flags, u8_t rx);
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PHY */

#endif /* _LL_H_ */
