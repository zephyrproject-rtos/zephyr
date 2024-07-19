/*
 * Copyright 2020 - 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/sdp.h>

#define APP_INQUIRY_LENGTH		(10) /* 10 * 1.28 Sec */
#define APP_INQUIRY_NUM_RESPONSES (20)

static uint32_t br_discover_result_count;
static struct bt_br_discovery_result br_discovery_results[APP_INQUIRY_NUM_RESPONSES];

static void br_device_found(size_t index, const bt_addr_t *addr, int8_t rssi,
				const uint8_t cod[3], const uint8_t eir[240])
{
	char br_addr[BT_ADDR_STR_LEN];
	char name[239];
	int len = 240;

	(void)memset(name, 0, sizeof(name));

	while (len) {
		if (len < 2) {
			break;
		}

		/* Look for early termination */
		if (!eir[0]) {
			break;
		}

		/* check if field length is correct */
		if (eir[0] > len - 1) {
			break;
		}

		switch (eir[1]) {
		case BT_DATA_NAME_SHORTENED:
		case BT_DATA_NAME_COMPLETE:
			memcpy(name, &eir[2], (eir[0] - 1) > (sizeof(name) - 1) ?
				(sizeof(name) - 1) : (eir[0] - 1));
			break;
		default:
			break;
		}

		/* Parse next AD Structure */
		len -= (eir[0] + 1);
		eir += (eir[0] + 1);
	}

	bt_addr_to_str(addr, br_addr, sizeof(br_addr));
	printk("[%d]: %s, RSSI %i %s\r\n", index + 1, br_addr, rssi, name);
}

static void br_discovery_complete(struct bt_br_discovery_result *results,
				  size_t count)
{
	size_t index;

	br_discover_result_count = count;
	printk("BR/EDR discovery complete\r\n");
	for (index = 0; index < count; ++index) {
		br_device_found(index, &results[index].addr, results[index].rssi,
			results[index].cod, results[index].eir);
	}
}

void app_discover(void)
{
	int err;
	struct bt_br_discovery_param param;

	param.length = APP_INQUIRY_LENGTH;
	param.limited = 0U;
	err = bt_br_discovery_start(&param, br_discovery_results,
								APP_INQUIRY_NUM_RESPONSES,
								br_discovery_complete);
	if (err != 0) {
		printk("Failed to start discovery\r\n");
	} else {
		printk("Discovery started. Please wait ...\r\n");
	}
}

uint8_t *app_get_addr(uint8_t select)
{
	if (select < br_discover_result_count) {
		return &br_discovery_results[select].addr.val[0];
	}

	return NULL;
}
