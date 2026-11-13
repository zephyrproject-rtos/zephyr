/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(canopen_sample, LOG_LEVEL_INF);

#define OD_COUNTER 0x2000U
#define OD_OUTPUT  0x2001U

static uint32_t counter;
static uint8_t output;

CANOPEN_OD_DEFINE(sample_od,
	CANOPEN_OD_VAR(OD_COUNTER, CANOPEN_OD_TYPE_UNSIGNED32,
		       CANOPEN_OD_RO | CANOPEN_OD_ATTR_TPDO, counter),
	CANOPEN_OD_VAR(OD_OUTPUT, CANOPEN_OD_TYPE_UNSIGNED8,
		       CANOPEN_OD_RW | CANOPEN_OD_ATTR_RPDO, output));

static const uint32_t tpdo1_map[] = {CANOPEN_PDO_MAP(OD_COUNTER, 0, 32)};
static const uint32_t rpdo1_map[] = {CANOPEN_PDO_MAP(OD_OUTPUT, 0, 8)};

static const struct canopen_pdo_config sample_tpdo[] = {
	{
		.map = tpdo1_map,
		.map_count = ARRAY_SIZE(tpdo1_map),
		.event_timer = CONFIG_SAMPLE_TPDO_EVENT_TIMER_MS,
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
	},
};

static const struct canopen_pdo_config sample_rpdo[] = {
	{
		.map = rpdo1_map,
		.map_count = ARRAY_SIZE(rpdo1_map),
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
	},
};

static void nmt_state_cb(struct canopen_node *node, enum canopen_nmt_state state,
			 void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	LOG_INF("NMT state 0x%02x", state);
}

static void reset_app_cb(struct canopen_node *node, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	counter = 0U;
	output = 0U;
}

static void hb_cb(struct canopen_node *node, uint8_t remote_id, enum canopen_hb_event event,
		  enum canopen_nmt_state state, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	LOG_INF("node 0x%02x: %s, NMT state 0x%02x", remote_id,
		(event == CANOPEN_HB_EVENT_TIMEOUT) ? "heartbeat lost" : "heartbeat", state);
}

static void rpdo_cb(struct canopen_node *node, uint16_t pdo, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	LOG_INF("RPDO%u: output 0x%02x", pdo, output);
}

static const struct canopen_node_config sample_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &sample_od,
	.nmt_state_cb = nmt_state_cb,
	.reset_app_cb = reset_app_cb,
	.rpdo_cb = rpdo_cb,
	.hb_cb = hb_cb,
	.tpdo = sample_tpdo,
	.rpdo = sample_rpdo,
	.tpdo_count = ARRAY_SIZE(sample_tpdo),
	.rpdo_count = ARRAY_SIZE(sample_rpdo),
	.node_id = CONFIG_SAMPLE_NODE_ID,
};

static struct canopen_node sample_node;

int main(void)
{
	uint32_t value;
	int err;

	err = can_set_bitrate(sample_cfg.can, CONFIG_SAMPLE_BITRATE);
	if (err != 0) {
		LOG_ERR("failed to set bitrate (%d)", err);
		return 0;
	}

	err = canopen_node_init(&sample_node, &sample_cfg);
	if (err != 0) {
		LOG_ERR("failed to initialize node (%d)", err);
		return 0;
	}

	err = canopen_node_start(&sample_node);
	if (err != 0) {
		LOG_ERR("failed to start node (%d)", err);
		return 0;
	}

	LOG_INF("CANopen node 0x%02x started at %u bit/s", CONFIG_SAMPLE_NODE_ID,
		CONFIG_SAMPLE_BITRATE);

	while (true) {
		k_sleep(K_MSEC(CONFIG_SAMPLE_COUNTER_PERIOD_MS));

		if (canopen_od_read(&sample_node, OD_COUNTER, 0U, &value, sizeof(value)) == 0) {
			value++;
			(void)canopen_od_write(&sample_node, OD_COUNTER, 0U, &value, sizeof(value));
		}
	}

	return 0;
}
