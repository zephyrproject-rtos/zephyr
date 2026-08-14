/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

/* Number of write iterations to attempt while BLE is advertising. */
#define WRITE_ITERATIONS 8U

/* Size of the buffer written on each iteration. */
#define WRITE_BUF_SIZE   64U

/* off_t is used for describing file sizes.  It is a signed integer type. Ensure offset passed to
 * flash_area_write() do not overflow.
 */
BUILD_ASSERT(((uint64_t)(WRITE_ITERATIONS) * (WRITE_BUF_SIZE)) <= INT_MAX);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *buf)
{
	printk("Device found: %s (RSSI %d), type %u, AD data len %u\n",	bt_addr_le_str(addr), rssi,
	       type, buf->len);
}

int main(void)
{
	const struct flash_area *fa;
	uint8_t buf[WRITE_BUF_SIZE] = {0};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth enable failed (err %d)\n", err);
		return 0;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		printk("Advertising start failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
		err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
		if (err != 0) {
			printk("Scanning start failed (err %d)\n", err);
			return 0;
		}
	}

	err = flash_area_open(PARTITION_ID(storage_partition), &fa);
	if (err != 0) {
		printk("Flash area open failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth LE advertising, writing to flash while radio is active\n");

	/* Erase the whole partition once up front. Using the full area size
	 * keeps the range aligned to the flash erase-block-size.
	 */
	err = flash_area_erase(fa, 0, fa->fa_size);
	if (err != 0) {
		printk("Flash area erase failed (err %d)\n", err);
		return 0;
	}

	/* Let the advertising state be active for sometime before the first flash write so that the
	 * flash operation contends with Bluetooth LE radio use.
	 */
	k_msleep(500);

	for (uint32_t i = 0U; i < WRITE_ITERATIONS; i++) {
		int64_t t0 = k_uptime_get();

		err = flash_area_write(fa, (off_t)i * WRITE_BUF_SIZE, buf, sizeof(buf));

		int64_t delta = k_uptime_delta(&t0);

		printk("iter %u err=%d (%lld ms)\n", i, err, delta);

		if (err != 0) {
			printk("flash write FAILED at iter %u (err %d)\n", i, err);
			break;
		}

		k_msleep(100);
	}

	flash_area_close(fa);

	if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
		int scan_err = bt_le_scan_stop();

		if (scan_err != 0) {
			printk("Scanning stop failed (err %d)\n", scan_err);
		}
	}

	int adv_err = bt_le_adv_stop();

	if (adv_err != 0) {
		printk("Advertising stop failed (err %d)\n", adv_err);
	}

	if (err != 0) {
		return 0;
	}

	printk("Successfully completed flash write while radio was active\n");

	return 0;
}
