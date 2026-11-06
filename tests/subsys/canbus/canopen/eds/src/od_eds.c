/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>

#include "test_od.h"

static uint32_t image_write(const struct canopen_od_entry *entry, size_t offset,
			    const uint8_t *buf, size_t len, size_t size, bool last, void *user_data)
{
	ARG_UNUSED(entry);
	ARG_UNUSED(offset);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(size);
	ARG_UNUSED(last);
	ARG_UNUSED(user_data);

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t image_read(const struct canopen_od_entry *entry, size_t offset, uint8_t *buf,
			   size_t *len, void *user_data)
{
	ARG_UNUSED(entry);
	ARG_UNUSED(offset);
	ARG_UNUSED(buf);
	ARG_UNUSED(user_data);

	*len = 0U;

	return CANOPEN_SDO_ABORT_NONE;
}

const struct canopen_od_domain x5600_firmware_image = {
	.read = image_read,
	.write = image_write,
};

const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.tpdo = TEST_OD_TPDO,
	.tpdo_count = TEST_OD_TPDO_COUNT,
	.rpdo = TEST_OD_RPDO,
	.rpdo_count = TEST_OD_RPDO_COUNT,
	.hb_consumer = TEST_OD_HB_CONSUMER,
	.hb_consumer_count = TEST_OD_HB_CONSUMER_COUNT,
	.serial_number = TEST_OD_SERIAL_NUMBER,
	.node_id = 0x22,
};
