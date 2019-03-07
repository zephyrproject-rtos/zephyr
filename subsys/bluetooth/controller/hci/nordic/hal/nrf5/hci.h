/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_SOC_COMPATIBLE_NRF)
#define BT_HCI_VS_HW_PLAT BT_HCI_VS_HW_PLAT_NORDIC
#if defined(CONFIG_SOC_SERIES_NRF51X)
#define BT_HCI_VS_HW_VAR  BT_HCI_VS_HW_VAR_NORDIC_NRF51X
#elif defined(CONFIG_SOC_COMPATIBLE_NRF52X)
#define BT_HCI_VS_HW_VAR  BT_HCI_VS_HW_VAR_NORDIC_NRF52X
#endif
#else
#define BT_HCI_VS_HW_PLAT 0
#define BT_HCI_VS_HW_VAR  0
#endif /* CONFIG_SOC_FAMILY_NRF */

#if defined(CONFIG_BT_HCI_MESH_EXT)
int mesh_cmd_handle(struct net_buf *cmd, struct net_buf **evt);
#endif /* CONFIG_BT_HCI_MESH_EXT */

int vendor_cmd_handle(u16_t ocf, struct net_buf *cmd,
		      struct net_buf **evt);
