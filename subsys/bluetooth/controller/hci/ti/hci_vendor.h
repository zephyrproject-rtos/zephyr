/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_SOC_FAMILY_TISIMPLELINK)
#define BT_HCI_VS_HW_PLAT BT_HCI_VS_HW_PLAT_TI
#if defined(CONFIG_BLE_CC13XX_CC26XX)
#define BT_HCI_VS_HW_VAR  BT_HCI_VS_HW_VAR_TI_CC13XX_CC26XX
#endif
#else
#define BT_HCI_VS_HW_PLAT 0
#define BT_HCI_VS_HW_VAR  0
#endif /* CONFIG_SOC_FAMILY_TISIMPLELINK */

/* Map vendor command handler directly to common implementation */
inline int hci_vendor_cmd_handle(u16_t ocf, struct net_buf *cmd,
				 struct net_buf **evt)
{
	return hci_vendor_cmd_handle_common(ocf, cmd, evt);
}
