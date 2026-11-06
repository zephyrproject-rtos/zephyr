/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/canbus/canopen.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

#include "canopen_internal.h"

extern const struct canopen_node_config test_cfg;

static void scan_entry(struct canopen_node *node, uint16_t index, uint8_t sub,
		       const struct canopen_od_ref *ref)
{
	uint8_t buf[CONFIG_CANOPEN_SDO_BUFFER_SIZE];
	size_t len = 0U;

	if (((ref->attr & CANOPEN_OD_ATTR_READ) != 0U) && (ref->type != CANOPEN_OD_TYPE_DOMAIN)) {
		zassert_equal(canopen_od_read_wire(ref, buf, sizeof(buf), &len),
			      CANOPEN_SDO_ABORT_NONE, "%04x sub %u", index, sub);
	}

	printk("OD %04x %02x %02x %02x ", index, sub, ref->type, ref->attr);
	for (size_t i = 0U; i < len; i++) {
		printk("%02x", buf[i]);
	}
	printk("\n");
}

ZTEST(canopen_eds, test_scan)
{
	static struct canopen_node node;
	struct canopen_od_ref ref;
	uint32_t abort;

	zassert_ok(canopen_node_init(&node, &test_cfg));

	for (uint32_t index = 0x1000U; index <= UINT16_MAX; index++) {
		for (uint32_t sub = 0U; sub <= UINT8_MAX; sub++) {
			abort = canopen_od_find(&node, index, sub, &ref);
			if (abort == CANOPEN_SDO_ABORT_NO_OBJECT) {
				break;
			}

			if (abort == CANOPEN_SDO_ABORT_NO_DATA) {
				printk("OD %04x %02x nodata\n", index, sub);
			} else if (abort == CANOPEN_SDO_ABORT_NONE) {
				scan_entry(&node, index, sub, &ref);
			}
		}
	}

	printk("OD end\n");
}

ZTEST_SUITE(canopen_eds, NULL, NULL, NULL, NULL, NULL);
