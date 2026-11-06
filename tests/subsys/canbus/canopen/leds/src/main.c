/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define TEST_NODE_ID     0x50U
#define TEST_REMOTE_ID   0x05U
#define TEST_REMOTE_HB   50U
#define TEST_RPDO_TIMER  30U
#define TEST_COB_SYNC    0x080U
#define TEST_COB_RPDO1   (0x200U + TEST_NODE_ID)
#define TEST_COB_SDO_RX  (0x600U + TEST_NODE_ID)
#define TEST_COB_SDO_TX  (0x580U + TEST_NODE_ID)
#define TEST_COB_NMT_ERR (0x700U + TEST_NODE_ID)
#define TEST_COB_REMOTE  (0x700U + TEST_REMOTE_ID)
#define TEST_TIMEOUT     K_MSEC(100)

#define RUN_PIN       DT_GPIO_PIN(DT_NODELABEL(canopen_run_led), gpios)
#define ERR_PIN       DT_GPIO_PIN(DT_NODELABEL(canopen_err_led), gpios)
#define SAMPLE_MS     5
#define TOLERANCE_MS  15
#define MAX_RUNS      32

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct device *const gpio_dev =
	DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(canopen_run_led), gpios));
static const struct led_dt_spec led_run = LED_DT_SPEC_GET(DT_NODELABEL(canopen_run_led));
static const struct led_dt_spec led_err = LED_DT_SPEC_GET(DT_NODELABEL(canopen_err_led));

CAN_MSGQ_DEFINE(sdo_msgq, 4);
CAN_MSGQ_DEFINE(err_msgq, 8);

static uint16_t rx_u16;

CANOPEN_OD_DEFINE(test_od,
	CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_UNSIGNED16, CANOPEN_OD_RW | CANOPEN_OD_ATTR_RPDO,
		       rx_u16));

static const uint32_t rpdo1_map[] = {CANOPEN_PDO_MAP(0x2000, 0, 16)};

static const struct canopen_pdo_config test_rpdo[] = {
	{
		.map = rpdo1_map,
		.map_count = ARRAY_SIZE(rpdo1_map),
		.event_timer = TEST_RPDO_TIMER,
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
	},
};

static const uint32_t test_hb_consumer[] = {
	((uint32_t)TEST_REMOTE_ID << 16) | TEST_REMOTE_HB,
};

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.rpdo = test_rpdo,
	.rpdo_count = ARRAY_SIZE(test_rpdo),
	.hb_consumer = test_hb_consumer,
	.hb_consumer_count = ARRAY_SIZE(test_hb_consumer),
	.led_run = &led_run,
	.led_err = &led_err,
	.led_bicolor = IS_ENABLED(CONFIG_TEST_LED_BICOLOR),
	.node_id = TEST_NODE_ID,
};

static struct canopen_node test_node;

static void send_frame(uint32_t id, const uint8_t *data, uint8_t dlc)
{
	struct can_frame frame = {
		.id = id,
		.dlc = dlc,
	};

	if (dlc > 0U) {
		memcpy(frame.data, data, dlc);
	}
	zassert_ok(can_send(can_dev, &frame, TEST_TIMEOUT, NULL, NULL));
}

static void send_nmt(uint8_t cmd)
{
	const uint8_t data[] = {cmd, TEST_NODE_ID};

	send_frame(0x000U, data, sizeof(data));
	k_msleep(1);
}

static void send_remote_hb(void)
{
	const uint8_t state = CANOPEN_NMT_OPERATIONAL;

	send_frame(TEST_COB_REMOTE, &state, 1U);
}

static void sdo_write(uint16_t index, uint8_t sub, uint32_t value, uint8_t len)
{
	struct can_frame frame;
	uint8_t req[8] = {0x23U | ((4U - len) << 2)};

	sys_put_le16(index, &req[1]);
	req[3] = sub;
	sys_put_le32(value, &req[4]);
	send_frame(TEST_COB_SDO_RX, req, sizeof(req));
	zassert_ok(k_msgq_get(&sdo_msgq, &frame, TEST_TIMEOUT), "no SDO response");
	zassert_equal(frame.data[0], 0x60U, "write %04x:%02x aborted", index, sub);
}

static void expect_bootup(void)
{
	struct can_frame frame;

	while (k_msgq_get(&err_msgq, &frame, TEST_TIMEOUT) == 0) {
		if ((frame.dlc == 1U) && (frame.data[0] == 0x00U)) {
			return;
		}
	}

	zassert_unreachable("no boot-up frame");
}

static bool pin_get(gpio_pin_t pin)
{
	return gpio_emul_output_get(gpio_dev, pin) == 1;
}

static size_t record(gpio_pin_t pin, int32_t window_ms, bool *levels, int32_t *lengths)
{
	bool level = pin_get(pin);
	int64_t start = k_uptime_get();
	int64_t edge = start;
	size_t n = 0U;

	while (((k_uptime_get() - start) < window_ms) && (n < MAX_RUNS)) {
		k_msleep(SAMPLE_MS);
		if (pin_get(pin) != level) {
			levels[n] = level;
			lengths[n] = (int32_t)(k_uptime_get() - edge);
			edge = k_uptime_get();
			level = !level;
			n++;
		}
	}

	return n;
}

static void expect_pattern(gpio_pin_t pin, uint8_t flashes)
{
	int32_t cycle[2U * 4U];
	bool levels[MAX_RUNS];
	int32_t lengths[MAX_RUNS];
	size_t count = (flashes == 0U) ? 2U : (2U * flashes);
	size_t n;
	size_t first = 1U;

	for (size_t i = 0U; i < count; i++) {
		cycle[i] = 200;
	}
	if (flashes != 0U) {
		cycle[count - 1U] = 1000;
	}

	n = record(pin, (flashes == 0U) ? 1200 : (int32_t)(2400U + (800U * flashes)), levels,
		   lengths);

	while ((first < n) &&
	       !(levels[first] && !levels[first - 1U] &&
		 ((flashes == 0U) || (lengths[first - 1U] >= (1000 - TOLERANCE_MS))))) {
		first++;
	}

	zassert_true((n - first) >= count, "pin %u: only %zu runs recorded", pin, n);

	for (size_t i = first; i < (first + count); i++) {
		size_t c = (i - first) % count;

		zassert_equal(levels[i], (c % 2U) == 0U, "pin %u: run %zu wrong level", pin, i);
		zassert_within(lengths[i], cycle[c], TOLERANCE_MS,
			       "pin %u: run %zu lasted %d ms, expected %d", pin, i, lengths[i],
			       cycle[c]);
	}
}

static void expect_steady(gpio_pin_t pin, bool on)
{
	for (int i = 0; i < (600 / SAMPLE_MS); i++) {
		zassert_equal(pin_get(pin), on, "pin %u changed", pin);
		k_msleep(SAMPLE_MS);
	}
}

static void add_filter(struct k_msgq *msgq, uint32_t id)
{
	struct can_filter filter = {
		.id = id,
		.mask = CAN_STD_ID_MASK,
	};

	zassert_true(can_add_rx_filter_msgq(can_dev, msgq, &filter) >= 0);
}

static void *leds_setup(void)
{
	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));

	add_filter(&sdo_msgq, TEST_COB_SDO_TX);
	add_filter(&err_msgq, TEST_COB_NMT_ERR);

	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));
	expect_bootup();

	return NULL;
}

static void leds_before(void *fixture)
{
	ARG_UNUSED(fixture);

	send_nmt(0x81U);
	expect_bootup();
	k_msgq_purge(&sdo_msgq);
}

ZTEST(canopen_leds, test_pre_operational)
{
	Z_TEST_SKIP_IFDEF(CONFIG_TEST_LED_BICOLOR);

	expect_pattern(RUN_PIN, 0U);
	expect_steady(ERR_PIN, false);
}

ZTEST(canopen_leds, test_operational)
{
	Z_TEST_SKIP_IFDEF(CONFIG_TEST_LED_BICOLOR);

	send_nmt(0x01U);
	expect_steady(RUN_PIN, true);
	expect_steady(ERR_PIN, false);
}

ZTEST(canopen_leds, test_stopped)
{
	Z_TEST_SKIP_IFDEF(CONFIG_TEST_LED_BICOLOR);

	send_nmt(0x02U);
	expect_pattern(RUN_PIN, 1U);
}

ZTEST(canopen_leds, test_heartbeat_event)
{
	Z_TEST_SKIP_IFDEF(CONFIG_TEST_LED_BICOLOR);

	sdo_write(0x1029, 1U, 1U, 1U);
	send_nmt(0x01U);
	send_remote_hb();
	k_msleep(TEST_REMOTE_HB * 2);
	expect_pattern(ERR_PIN, 2U);
	expect_steady(RUN_PIN, true);

	send_remote_hb();
	k_msleep(1);
	zassert_false(pin_get(ERR_PIN));
}

ZTEST(canopen_leds, test_sync_error)
{
	Z_TEST_SKIP_IFDEF(CONFIG_TEST_LED_BICOLOR);

	sdo_write(0x1006, 0U, 50000U, 4U);
	send_frame(TEST_COB_SYNC, NULL, 0U);
	k_msleep(100);
	expect_pattern(ERR_PIN, 3U);

	send_frame(TEST_COB_SYNC, NULL, 0U);
	k_msleep(1);
	zassert_false(pin_get(ERR_PIN));
	sdo_write(0x1006, 0U, 0U, 4U);
	expect_steady(ERR_PIN, false);
}

ZTEST(canopen_leds, test_event_timer_error)
{
	const uint8_t data[] = {0x34U, 0x12U};

	Z_TEST_SKIP_IFDEF(CONFIG_TEST_LED_BICOLOR);

	send_nmt(0x01U);
	send_frame(TEST_COB_RPDO1, data, sizeof(data));
	k_msleep(TEST_RPDO_TIMER * 2);
	expect_pattern(ERR_PIN, 4U);
}

ZTEST(canopen_leds, test_error_priority)
{
	const uint8_t data[] = {0x34U, 0x12U};

	Z_TEST_SKIP_IFDEF(CONFIG_TEST_LED_BICOLOR);

	sdo_write(0x1029, 1U, 1U, 1U);
	send_nmt(0x01U);
	send_frame(TEST_COB_RPDO1, data, sizeof(data));
	send_remote_hb();
	k_msleep(TEST_REMOTE_HB * 2);
	expect_pattern(ERR_PIN, 2U);
}

ZTEST(canopen_leds, test_bicolor)
{
	bool run_seen = false;

	Z_TEST_SKIP_IFNDEF(CONFIG_TEST_LED_BICOLOR);

	send_remote_hb();
	k_msleep(TEST_REMOTE_HB * 2);

	for (int i = 0; i < (2400 / SAMPLE_MS); i++) {
		bool run = pin_get(RUN_PIN);

		zassert_false(run && pin_get(ERR_PIN), "green and red on together");
		run_seen = run_seen || run;
		k_msleep(SAMPLE_MS);
	}

	zassert_true(run_seen, "green never on");
	expect_pattern(ERR_PIN, 2U);
}

ZTEST_SUITE(canopen_leds, NULL, leds_setup, leds_before, NULL, NULL);
