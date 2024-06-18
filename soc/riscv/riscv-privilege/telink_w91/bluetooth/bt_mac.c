/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bt_mac.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/bluetooth/addr.h>

#ifdef CONFIG_FLASH
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#define FLASH_PARTITION             vendor_partition
#define FLASH_PARTITION_DEVICE      FIXED_PARTITION_DEVICE(FLASH_PARTITION)
#define FLASH_PARTITION_OFFSET      FIXED_PARTITION_OFFSET(FLASH_PARTITION)

/* SOC BT MAC Address Offset */
#define W91_BT_MAC_ADDR_OFFSET      0x1000
#endif /* CONFIG_FLASH */

#ifdef CONFIG_ENTROPY_GENERATOR
#include <zephyr/drivers/entropy.h>
#define ENTROPY_DEVICE              DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy))
#endif /* CONFIG_ENTROPY_GENERATOR */

__attribute__((noinline)) void telink_bt_blc_mac_init(uint8_t *bt_mac)
{
	uint8_t temp_mac[BT_ADDR_SIZE];

	memset(bt_mac, 0xFF, BT_ADDR_SIZE);

#ifdef CONFIG_FLASH
	if (flash_read(FLASH_PARTITION_DEVICE,
			FLASH_PARTITION_OFFSET + W91_BT_MAC_ADDR_OFFSET,
			temp_mac, BT_ADDR_SIZE)) {
		return;
	}
#else
	memset(temp_mac, 0xFF, BT_ADDR_SIZE);
#endif

	if (!memcmp(bt_mac, temp_mac, BT_ADDR_SIZE)) {
#ifdef CONFIG_ENTROPY_GENERATOR
		if (entropy_get_entropy(ENTROPY_DEVICE, temp_mac, BT_ADDR_SIZE)) {
			return;
		}
#endif

#if CONFIG_TELINK_W91_BLE_MAC_TYPE_PUBLIC && CONFIG_TELINK_W91_BLE_MAC_PUBLIC_DEBUG
/* Debug purpose only. Public address must be set by vendor only */
		temp_mac[3] = 0x38; /* company id: 0xA4C138 */
		temp_mac[4] = 0xC1;
		temp_mac[5] = 0xA4;
#elif CONFIG_TELINK_W91_BLE_MAC_TYPE_RANDOM_STATIC || CONFIG_TELINK_W91_BLE_MAC_TYPE_PUBLIC
/* Enters this condition, when configured public address is empty */
/* Generally we are not allowed to generate public address in mass production device */
/* The random static address will be generated, if stored public MAC is empty */
		temp_mac[5] |= 0xC0; /* random static by default */
#else
	#error "Other address types are not supported or need to be set via HCI"
#endif

#ifdef CONFIG_FLASH
		if (flash_write(FLASH_PARTITION_DEVICE,
				FLASH_PARTITION_OFFSET + W91_BT_MAC_ADDR_OFFSET,
				temp_mac, BT_ADDR_SIZE)) {
			return;
		}
#endif
	}

	memcpy(bt_mac, temp_mac, BT_ADDR_SIZE);
}
