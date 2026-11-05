/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(canopen_sample, LOG_LEVEL_INF);

#define OD_COUNTER 0x2000U
#define OD_OUTPUT  0x2001U
#define OD_DATA    0x5600U

#define DATA_SIZE 256U

#define MS_PER_DAY      86400000U
#define TIME_PERIOD_MS 1000U

#define RUN_LED_NODE DT_ALIAS(canopen_run_led)
#define ERR_LED_NODE DT_ALIAS(canopen_err_led)

#if DT_NODE_EXISTS(RUN_LED_NODE) && defined(CONFIG_CANOPEN_LEDS)
static const struct led_dt_spec run_led = LED_DT_SPEC_GET(RUN_LED_NODE);
#define SAMPLE_RUN_LED (&run_led)
#else
#define SAMPLE_RUN_LED NULL
#endif

#if DT_NODE_EXISTS(ERR_LED_NODE) && defined(CONFIG_CANOPEN_LEDS)
static const struct led_dt_spec err_led = LED_DT_SPEC_GET(ERR_LED_NODE);
#define SAMPLE_ERR_LED (&err_led)
#else
#define SAMPLE_ERR_LED NULL
#endif

static uint32_t counter;
static uint8_t output;

static uint8_t sample_data[DATA_SIZE];
static size_t sample_data_len;

static uint32_t data_read(const struct canopen_od_entry *entry, size_t offset, uint8_t *buf,
			  size_t *len, void *user_data)
{
	ARG_UNUSED(entry);
	ARG_UNUSED(user_data);

	*len = MIN(*len, sample_data_len - MIN(offset, sample_data_len));
	memcpy(buf, &sample_data[offset], *len);

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t data_write(const struct canopen_od_entry *entry, size_t offset, const uint8_t *buf,
			   size_t len, size_t size, bool last, void *user_data)
{
	ARG_UNUSED(entry);
	ARG_UNUSED(user_data);

	if ((size > sizeof(sample_data)) || ((offset + len) > sizeof(sample_data))) {
		return CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH;
	}

	memcpy(&sample_data[offset], buf, len);
	sample_data_len = offset + len;

	if (last) {
		LOG_INF("5600h: %zu bytes received", sample_data_len);
	}

	return CANOPEN_SDO_ABORT_NONE;
}

static const struct canopen_od_domain data_domain = {
	.read = data_read,
	.write = data_write,
};

CANOPEN_OD_DEFINE(sample_od,
	CANOPEN_OD_VAR(OD_COUNTER, CANOPEN_OD_TYPE_UNSIGNED32,
		       CANOPEN_OD_RO | CANOPEN_OD_ATTR_TPDO, counter),
	CANOPEN_OD_VAR(OD_OUTPUT, CANOPEN_OD_TYPE_UNSIGNED8,
		       CANOPEN_OD_RW | CANOPEN_OD_ATTR_RPDO, output),
	CANOPEN_OD_DOMAIN(OD_DATA, CANOPEN_OD_RW, data_domain, NULL));

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

#ifdef CONFIG_SAMPLE_MASTER
static const uint32_t sample_hb_consumer[] = {((uint32_t)CONFIG_SAMPLE_SLAVE_ID << 16) | 3000U};

static const struct canopen_boot_entry slave_config[] = {
	{.index = 0x1017, .sub = 0U, .len = 2U, .value = 1000U},
	{.index = 0x2000, .sub = 0U, .len = 4U, .value = 0x11223344U},
};

static const struct canopen_boot_slave sample_slaves[] = {
	{
		.config = slave_config,
		.config_count = ARRAY_SIZE(slave_config),
		.node_id = CONFIG_SAMPLE_SLAVE_ID,
	},
};
#define SAMPLE_HB_CONSUMER       sample_hb_consumer
#define SAMPLE_HB_CONSUMER_COUNT ARRAY_SIZE(sample_hb_consumer)
#else
#define SAMPLE_HB_CONSUMER       NULL
#define SAMPLE_HB_CONSUMER_COUNT 0U
#endif

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

static struct k_spinlock time_lock;
static int64_t time_uptime;
static uint32_t time_ms;
static uint16_t time_days;
static bool time_valid;

static void time_cb(struct canopen_node *node, uint32_t ms, uint16_t days, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	K_SPINLOCK(&time_lock) {
		time_uptime = k_uptime_get();
		time_ms = ms;
		time_days = days;
		time_valid = true;
	}

	LOG_INF("TIME: day %u, %u ms", days, ms);
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
	.time_cb = time_cb,
	.hb_consumer = SAMPLE_HB_CONSUMER,
	.hb_consumer_count = SAMPLE_HB_CONSUMER_COUNT,
	.tpdo = sample_tpdo,
	.rpdo = sample_rpdo,
	.tpdo_count = ARRAY_SIZE(sample_tpdo),
	.rpdo_count = ARRAY_SIZE(sample_rpdo),
	.led_run = SAMPLE_RUN_LED,
	.led_err = SAMPLE_ERR_LED,
	.node_id = CONFIG_SAMPLE_NODE_ID,
};

static struct canopen_node sample_node;

static void time_produce(void)
{
	uint64_t ms = 0U;
	uint32_t days = 0U;
	bool valid;

	K_SPINLOCK(&time_lock) {
		valid = time_valid;
		ms = time_ms + (uint64_t)(k_uptime_get() - time_uptime);
		days = time_days;
	}

	if (valid) {
		days += (uint32_t)(ms / MS_PER_DAY);
		(void)canopen_time_send(&sample_node, (uint32_t)(ms % MS_PER_DAY), (uint16_t)days);
	}
}

#ifdef CONFIG_SAMPLE_MASTER
static void sample_master(void)
{
	char name[32];
	size_t len = sizeof(name) - 1U;
	int err;

	err = canopen_master_boot(&sample_node, sample_slaves, ARRAY_SIZE(sample_slaves),
				  K_SECONDS(30));
	if (err != 0) {
		LOG_ERR("boot of node 0x%02x failed (%d)", CONFIG_SAMPLE_SLAVE_ID, err);
		return;
	}

	LOG_INF("node 0x%02x configured and started", CONFIG_SAMPLE_SLAVE_ID);

	err = canopen_sdo_upload(&sample_node, CONFIG_SAMPLE_SLAVE_ID, 0x1008, 0U, name, &len,
				 NULL);
	if (err != 0) {
		LOG_ERR("reading the device name of node 0x%02x failed (%d)",
			CONFIG_SAMPLE_SLAVE_ID, err);
		return;
	}

	name[len] = '\0';
	LOG_INF("node 0x%02x device name: %s", CONFIG_SAMPLE_SLAVE_ID, name);
}
#endif

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

#ifdef CONFIG_SAMPLE_MASTER
	sample_master();
#endif

	for (uint32_t elapsed = 0U; true; elapsed += CONFIG_SAMPLE_COUNTER_PERIOD_MS) {
		k_sleep(K_MSEC(CONFIG_SAMPLE_COUNTER_PERIOD_MS));

		if (IS_ENABLED(CONFIG_CANOPEN_TIME) && (elapsed >= TIME_PERIOD_MS)) {
			elapsed = 0U;
			time_produce();
		}

		if (canopen_od_read(&sample_node, OD_COUNTER, 0U, &value, sizeof(value)) == 0) {
			value++;
			(void)canopen_od_write(&sample_node, OD_COUNTER, 0U, &value, sizeof(value));
		}
	}

	return 0;
}
