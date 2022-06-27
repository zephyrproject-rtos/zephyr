/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/hci_vs.h>

/**
 * @brief This is a stub of a function that provides static address.
 *
 * This function is defined to silent warning printed by Host during BT stack initialization due
 * to lack of static address stored in controller.
 */
uint8_t hci_vendor_read_static_addr(struct bt_hci_vs_static_addr addrs[], uint8_t size)
{
	/* only one supported */
	ARG_UNUSED(size);

	/* Use some fake address, because it does not matter for test purposes */
	(void)memset(addrs[0].ir, 0x01, sizeof(addrs[0].ir));

	return 1;
}
