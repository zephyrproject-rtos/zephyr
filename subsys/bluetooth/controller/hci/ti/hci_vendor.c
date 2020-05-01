/* Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <sys/byteorder.h>

#include <bluetooth/addr.h>
#include <bluetooth/hci_vs.h>

#include "hci_vendor.h"

#if defined(CONFIG_SOC_FAMILY_TISIMPLELINK)
#include <inc/hw_ccfg.h>
#include <inc/hw_fcfg1.h>
#include <inc/hw_memmap.h>
#endif

u8_t hci_vendor_read_static_addr(struct bt_hci_vs_static_addr addrs[],
				 u8_t size)
{

#if defined(CONFIG_SOC_FAMILY_TISIMPLELINK)
	/* only one supported */
	ARG_UNUSED(size);

	/* Read address from cc13xx_cc26xx-specific storage */
	u32_t *mac;

	if (sys_read32(CCFG_BASE + CCFG_O_IEEE_BLE_0) != 0xFFFFFFFF &&
	    sys_read32(CCFG_BASE + CCFG_O_IEEE_BLE_1) != 0xFFFFFFFF) {
		mac = (u32_t *)(CCFG_BASE + CCFG_O_IEEE_BLE_0);
	} else {
		mac = (u32_t *)(FCFG1_BASE + FCFG1_O_MAC_BLE_0);
	}

	sys_put_le32(mac[0], &addrs[0].bdaddr.val[0]);
	sys_put_le16((u16_t)mac[1], &addrs[0].bdaddr.val[4]);

	BT_ADDR_SET_STATIC(&addrs[0].bdaddr);

	/* Mark IR as invalid */
	(void)memset(addrs[0].ir, 0x00, sizeof(addrs[0].ir));

	return 1;
#endif /* CONFIG_SOC_FAMILY_TISIMPLELINK */

	return 0;
}
