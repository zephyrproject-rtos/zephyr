/* Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>

#include <bluetooth/addr.h>
#include <bluetooth/hci_vs.h>

#include <soc.h>

uint8_t hci_vendor_read_static_addr(struct bt_hci_vs_static_addr addrs[],
				 uint8_t size)
{
	/* only one supported */
	ARG_UNUSED(size);

	if (((NRF_FICR->DEVICEADDR[0] != UINT32_MAX) ||
	    ((NRF_FICR->DEVICEADDR[1] & UINT16_MAX) != UINT16_MAX)) &&
	     (NRF_FICR->DEVICEADDRTYPE & 0x01)) {
		sys_put_le32(NRF_FICR->DEVICEADDR[0], &addrs[0].bdaddr.val[0]);
		sys_put_le16(NRF_FICR->DEVICEADDR[1], &addrs[0].bdaddr.val[4]);

		/* The FICR value is a just a random number, with no knowledge
		 * of the Bluetooth Specification requirements for random
		 * static addresses.
		 */
		BT_ADDR_SET_STATIC(&addrs[0].bdaddr);

		/* If no public address is provided and a static address is
		 * available, then it is recommended to return an identity root
		 * key (if available) from this command.
		 */
		if ((NRF_FICR->IR[0] != UINT32_MAX) &&
		    (NRF_FICR->IR[1] != UINT32_MAX) &&
		    (NRF_FICR->IR[2] != UINT32_MAX) &&
		    (NRF_FICR->IR[3] != UINT32_MAX)) {
			sys_put_le32(NRF_FICR->IR[0], &addrs[0].ir[0]);
			sys_put_le32(NRF_FICR->IR[1], &addrs[0].ir[4]);
			sys_put_le32(NRF_FICR->IR[2], &addrs[0].ir[8]);
			sys_put_le32(NRF_FICR->IR[3], &addrs[0].ir[12]);
		} else {
			/* Mark IR as invalid */
			(void)memset(addrs[0].ir, 0x00, sizeof(addrs[0].ir));
		}

		return 1;
	}

	return 0;
}

void hci_vendor_read_key_hierarchy_roots(uint8_t ir[16], uint8_t er[16])
{
	/* Mark IR as invalid.
	 * No public address is available, and static address IR should be read
	 * using Read Static Addresses command.
	 */
	(void)memset(ir, 0x00, 16);

	/* Fill in ER if present */
	if ((NRF_FICR->ER[0] != UINT32_MAX) &&
	    (NRF_FICR->ER[1] != UINT32_MAX) &&
	    (NRF_FICR->ER[2] != UINT32_MAX) &&
	    (NRF_FICR->ER[3] != UINT32_MAX)) {
		sys_put_le32(NRF_FICR->ER[0], &er[0]);
		sys_put_le32(NRF_FICR->ER[1], &er[4]);
		sys_put_le32(NRF_FICR->ER[2], &er[8]);
		sys_put_le32(NRF_FICR->ER[3], &er[12]);
	} else {
		/* Mark ER as invalid */
		(void)memset(er, 0x00, 16);
	}
}
