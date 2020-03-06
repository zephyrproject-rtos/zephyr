/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "addr.h"
#include "ll.h"
#include "memq.h"
#include "lll.h"
#include "pdu.h"
#include "lll_conn.h"
#include "ull_internal.h"

void bt_ctlr_assert_handle(char *file, u32_t line)
{
}

struct device *device_get_binding(char *name)
{
	return NULL;
}

int entropy_get_entropy(struct device *dev, u8_t *buf, u8_t len)
{
	return 0;
}

u8_t util_ones_count_get(u8_t *octets, u8_t octets_len)
{
	u8_t one_count = 0U;

	while (octets_len--) {
		u8_t bite;

		bite = *octets;
		while (bite) {
			bite &= (bite - 1);
			one_count++;
		}
		octets++;
	}

	return one_count;
}

void lll_conn_flush(u16_t handle, struct lll_conn *lll)
{
}

void ull_rx_put(memq_link_t *link, void *rx)
{
}

void ull_rx_sched(void)
{
}

int ll_init(struct k_sem *sem_rx)
{
	return 0;
}

void ll_reset(void)
{
}

u8_t *ll_addr_get(u8_t addr_type, u8_t *p_bdaddr)
{
	return 0;
}

u8_t ll_addr_set(u8_t addr_type, u8_t const *const p_bdaddr)
{
	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
u8_t ll_adv_params_set(u8_t handle, u16_t evt_prop, u32_t interval,
		       u8_t adv_type, u8_t own_addr_type,
		       u8_t direct_addr_type, u8_t const *const direct_addr,
		       u8_t chan_map, u8_t filter_policy, u8_t *tx_pwr,
		       u8_t phy_p, u8_t skip, u8_t phy_s, u8_t sid, u8_t sreq)
{
	return 0;
}

u8_t ll_adv_data_set(u16_t handle, u8_t len, u8_t const *const p_data)
{
	return 0;
}

u8_t ll_adv_scan_rsp_set(u16_t handle, u8_t len, u8_t const *const p_data)
{
	return 0;
}

#else /* !CONFIG_BT_CTLR_ADV_EXT */
u8_t ll_adv_params_set(u16_t interval, u8_t adv_type,
		       u8_t own_addr_type, u8_t direct_addr_type,
		       u8_t const *const direct_addr, u8_t chan_map,
		       u8_t filter_policy)
{
	return 0;
}

u8_t ll_adv_data_set(u8_t len, u8_t const *const p_data)
{
	return 0;
}

u8_t ll_adv_scan_rsp_set(u8_t len, u8_t const *const p_data)
{
	return 0;
}
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_ADV_EXT) || defined(CONFIG_BT_HCI_MESH_EXT)
#if defined(CONFIG_BT_HCI_MESH_EXT)
u8_t ll_adv_enable(u16_t handle, u8_t enable,
		   u8_t at_anchor, u32_t ticks_anchor, u8_t retry,
		   u8_t scan_window, u8_t scan_delay)
{
	return 0;
}

#else /* !CONFIG_BT_HCI_MESH_EXT */
u8_t ll_adv_enable(u16_t handle, u8_t enable)
{
	return 0;
}
#endif /* !CONFIG_BT_HCI_MESH_EXT */
#else /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
u8_t ll_adv_enable(u8_t enable)
{
	return 0;
}
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */

u8_t ll_scan_params_set(u8_t type, u16_t interval, u16_t window,
			u8_t own_addr_type, u8_t filter_policy)
{
	return 0;
}
u8_t ll_scan_enable(u8_t enable)
{
	return 0;
}

u8_t ll_wl_size_get(void)
{
	return 0;
}

u8_t ll_wl_clear(void)
{
	return 0;
}

u8_t ll_wl_add(bt_addr_le_t *addr)
{
	return 0;
}

u8_t ll_wl_remove(bt_addr_le_t *addr)
{
	return 0;
}

void ll_rl_id_addr_get(u8_t rl_idx, u8_t *id_addr_type, u8_t *id_addr)
{
}

u8_t ll_rl_size_get(void)
{
	return 0;
}

u8_t ll_rl_clear(void)
{
	return 0;
}

u8_t ll_rl_add(bt_addr_le_t *id_addr, const u8_t pirk[16],
		const u8_t lirk[16])
{
	return 0;
}

u8_t ll_rl_remove(bt_addr_le_t *id_addr)
{
	return 0;
}

void ll_rl_crpa_set(u8_t id_addr_type, u8_t *id_addr, u8_t rl_idx, u8_t *crpa)
{
}

u8_t ll_rl_crpa_get(bt_addr_le_t *id_addr, bt_addr_t *crpa)
{
	return 0;
}

u8_t ll_rl_lrpa_get(bt_addr_le_t *id_addr, bt_addr_t *lrpa)
{
	return 0;
}

u8_t ll_rl_enable(u8_t enable)
{
	return 0;
}

void ll_rl_timeout_set(u16_t timeout)
{
}

u8_t ll_priv_mode_set(bt_addr_le_t *id_addr, u8_t mode)
{
	return 0;
}

u8_t ll_create_connection(u16_t scan_interval, u16_t scan_window,
			  u8_t filter_policy, u8_t peer_addr_type,
			  u8_t *peer_addr, u8_t own_addr_type,
			  u16_t interval, u16_t latency, u16_t timeout)
{
	return 0;
}

u8_t ll_connect_disable(void **rx)
{
	return 0;
}

u8_t ll_chm_update(u8_t *chm)
{
	return 0;
}

u8_t ll_enc_req_send(u16_t handle, u8_t *rand, u8_t *ediv,
				 u8_t *ltk)
{
	return 0;
}

u8_t ll_start_enc_req_send(u16_t handle, u8_t err_code,
				 u8_t const *const ltk)
{
	return 0;
}

u8_t ll_rssi_get(u16_t handle, u8_t *rssi)
{
	return 0;
}

u8_t ll_tx_pwr_lvl_get(u8_t handle_type,
		       u16_t handle, u8_t type, s8_t *tx_pwr_lvl)
{
	return 0;
}

void ll_tx_pwr_get(s8_t *min, s8_t *max)
{
}

u8_t ll_tx_pwr_lvl_set(u8_t handle_type,
		       u16_t handle, s8_t *tx_pwr_lvl)
{
	return 0;
}

u8_t ll_apto_get(u16_t handle, u16_t *apto)
{
	return 0;
}

u8_t ll_apto_set(u16_t handle, u16_t apto)
{
	return 0;
}
