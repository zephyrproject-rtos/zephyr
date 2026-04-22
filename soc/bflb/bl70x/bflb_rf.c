/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>

#include <bl702_rf_public.h>

extern int bl_wireless_mac_addr_set(uint8_t mac[8]);

int bflb_rf_init(void)
{
	uint8_t mac[8] = {};

	hwinfo_get_device_id(mac, 6);
	sys_mem_swap(mac, 6);

	bl_wireless_mac_addr_set(mac);
	rf702_set_init_tsen_value(0);

	return 0;
}
