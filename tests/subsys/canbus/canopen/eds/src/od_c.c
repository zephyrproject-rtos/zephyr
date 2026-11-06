/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>

static bool flag = true;
static int16_t temperature = -273;
static uint64_t energy = 0x0123456789ABCDEFULL;
static float gain = 1.5f;
static char label[12] = "zephyr";
static uint8_t key[4] = {0xDEU, 0xADU, 0xBEU, 0xEFU};
static uint32_t limits[3] = {10U, 20U, 30U};
static uint8_t mode = 3U;
static uint32_t position = 0xFFFFFFFFU;
static struct {
	uint16_t speed;
	int8_t offset;
} motor = {
	.speed = 1500U,
	.offset = -5,
};

static uint32_t log_read(const struct canopen_od_entry *entry, size_t offset, uint8_t *buf,
			 size_t *len, void *user_data)
{
	ARG_UNUSED(entry);
	ARG_UNUSED(offset);
	ARG_UNUSED(buf);
	ARG_UNUSED(user_data);

	*len = 0U;

	return CANOPEN_SDO_ABORT_NONE;
}

static const struct canopen_od_domain event_log = {
	.read = log_read,
};

CANOPEN_OD_DEFINE(test_od,
	CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_BOOLEAN, CANOPEN_OD_RW | CANOPEN_OD_ATTR_PERSIST,
		       flag),
	CANOPEN_OD_VAR(0x2001, CANOPEN_OD_TYPE_INTEGER16, CANOPEN_OD_RO | CANOPEN_OD_ATTR_TPDO,
		       temperature),
	CANOPEN_OD_VAR(0x2002, CANOPEN_OD_TYPE_UNSIGNED64, CANOPEN_OD_CONST, energy),
	CANOPEN_OD_VAR(0x2003, CANOPEN_OD_TYPE_REAL32, CANOPEN_OD_WO | CANOPEN_OD_ATTR_RPDO, gain),
	CANOPEN_OD_VAR(0x2004, CANOPEN_OD_TYPE_VISIBLE_STRING, CANOPEN_OD_RW, label),
	CANOPEN_OD_VAR(0x2005, CANOPEN_OD_TYPE_OCTET_STRING, CANOPEN_OD_RO, key),
	CANOPEN_OD_ARRAY(0x2100, CANOPEN_OD_TYPE_UNSIGNED32,
			 CANOPEN_OD_RW | CANOPEN_OD_ATTR_TPDO | CANOPEN_OD_ATTR_RPDO, limits),
	CANOPEN_OD_RECORD(0x2200,
		CANOPEN_OD_SUB(1U, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW, mode),
		CANOPEN_OD_SUB(4U, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RO | CANOPEN_OD_ATTR_TPDO,
			       position)),
	CANOPEN_OD_RECORD(0x2300,
		CANOPEN_OD_SUB(1U, CANOPEN_OD_TYPE_UNSIGNED16, CANOPEN_OD_RW, motor.speed),
		CANOPEN_OD_SUB(2U, CANOPEN_OD_TYPE_INTEGER8, CANOPEN_OD_RW, motor.offset)),
	CANOPEN_OD_DOMAIN(0x5600, CANOPEN_OD_RO, event_log, NULL),
	CANOPEN_OD_VAR(0x6000, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RO | CANOPEN_OD_ATTR_TPDO,
		       mode));

static const uint32_t tpdo1_map[] = {
	CANOPEN_PDO_MAP(0x2001, 0, 16),
	CANOPEN_PDO_MAP(0x2200, 4, 32),
};

static const uint32_t rpdo2_map[] = {
	CANOPEN_PDO_MAP(0x0005, 0, 8),
	CANOPEN_PDO_MAP(0x2003, 0, 32),
};

static const struct canopen_pdo_config test_tpdo[] = {
	{
		.map = tpdo1_map,
		.map_count = ARRAY_SIZE(tpdo1_map),
		.transmission_type = 1U,
		.sync_start = 2U,
	},
	{
		.cob_id = CANOPEN_PDO_COB_ID_INVALID,
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
		.inhibit_time = 100U,
		.event_timer = 250U,
	},
	{
		.cob_id = 0x40000000U | 0x3A2U,
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
	},
	{
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
	},
	{
		.cob_id = 0x40000000U | 0x4A2U,
		.transmission_type = 0xFFU,
	},
};

static const struct canopen_pdo_config test_rpdo[] = {
	{
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
	},
	{
		.map = rpdo2_map,
		.map_count = ARRAY_SIZE(rpdo2_map),
		.transmission_type = 0U,
		.event_timer = 50U,
	},
};

static const uint32_t test_hb_consumer[] = {
	(0x05U << 16) | 1000U,
	0U,
	(0x07U << 16) | 2000U,
};

const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.tpdo = test_tpdo,
	.tpdo_count = ARRAY_SIZE(test_tpdo),
	.rpdo = test_rpdo,
	.rpdo_count = ARRAY_SIZE(test_rpdo),
	.hb_consumer = test_hb_consumer,
	.hb_consumer_count = ARRAY_SIZE(test_hb_consumer),
	.serial_number = 0x12345678U,
	.node_id = 0x22,
};
