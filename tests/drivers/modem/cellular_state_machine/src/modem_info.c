/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <modem_cellular_test.h>
#include <zephyr/ztest.h>

#include <string.h>

static const struct device *const modem_info = DEVICE_DT_GET(DT_NODELABEL(modem_info));

/* Each string as left once the init script completes */
static const struct {
	enum cellular_modem_info_type type;
	const char *value;
} expected_info[] = {
	{CELLULAR_MODEM_INFO_IMEI, TEST_IMEI},
	{CELLULAR_MODEM_INFO_SERIAL_NUMBER, TEST_SERIAL_NUMBER},
	{CELLULAR_MODEM_INFO_MODEL_ID, TEST_MODEL_ID},
	{CELLULAR_MODEM_INFO_MANUFACTURER, TEST_MANUFACTURER},
	{CELLULAR_MODEM_INFO_FW_VERSION, TEST_FW_VERSION},
	{CELLULAR_MODEM_INFO_SIM_IMSI, TEST_IMSI},
	{CELLULAR_MODEM_INFO_SIM_ICCID, TEST_ICCID},
};

K_MSGQ_DEFINE(info_events, sizeof(enum cellular_modem_info_type), 16, 4);

static void info_event_handler(const struct device *dev, enum cellular_event event,
			       const void *payload, void *user_data)
{
	const struct cellular_evt_modem_info *info = payload;

	ARG_UNUSED(dev);
	ARG_UNUSED(event);
	ARG_UNUSED(user_data);

	(void)k_msgq_put(&info_events, &info->field, K_NO_WAIT);
}

/* Every string is reported while the init script runs, with an event each time it is set */
ZTEST(modem_cellular_modem_info, test_reported_during_init)
{
	/* The first response sets both the IMEI and the serial number */
	static const enum cellular_modem_info_type expected_events[] = {
		CELLULAR_MODEM_INFO_IMEI,       CELLULAR_MODEM_INFO_SERIAL_NUMBER,
		CELLULAR_MODEM_INFO_IMEI,       CELLULAR_MODEM_INFO_SERIAL_NUMBER,
		CELLULAR_MODEM_INFO_MODEL_ID,   CELLULAR_MODEM_INFO_MANUFACTURER,
		CELLULAR_MODEM_INFO_FW_VERSION, CELLULAR_MODEM_INFO_SIM_IMSI,
		CELLULAR_MODEM_INFO_SIM_ICCID,
	};
	enum cellular_modem_info_type field;
	char buf[MODEM_CELLULAR_DATA_MODEL_ID_LEN];

	k_msgq_purge(&info_events);
	zassert_ok(cellular_set_callback(modem_info, CELLULAR_EVENT_MODEM_INFO_CHANGED,
					 info_event_handler, NULL));

	modem_restart(modem_info);
	advance_to(modem_info, BOOT_DLCI1_SETUP);

	for (size_t i = 0; i < ARRAY_SIZE(expected_events); i++) {
		zassert_ok(k_msgq_get(&info_events, &field, K_NO_WAIT),
			   "no event %zu, for info type %d", i, expected_events[i]);
		zassert_equal(expected_events[i], field, "event %zu for info type %d, expected %d",
			      i, field, expected_events[i]);
	}
	zassert_equal(0, k_msgq_num_used_get(&info_events), "unexpected info events");

	for (size_t i = 0; i < ARRAY_SIZE(expected_info); i++) {
		zassert_ok(cellular_get_modem_info(modem_info, expected_info[i].type, buf,
						   sizeof(buf)));
		zassert_str_equal(expected_info[i].value, buf);
	}

	zassert_ok(cellular_set_callback(modem_info, 0, NULL, NULL));
}

/* Strings are truncated to fit the caller's buffer, which is always terminated */
ZTEST(modem_cellular_modem_info, test_truncated)
{
	char buf[4];

	modem_restart(modem_info);
	advance_to(modem_info, BOOT_DLCI1_SETUP);

	zassert_ok(cellular_get_modem_info(modem_info, CELLULAR_MODEM_INFO_IMEI, buf, sizeof(buf)));
	zassert_str_equal("352", buf);
	zassert_equal(-EINVAL,
		      cellular_get_modem_info(modem_info, CELLULAR_MODEM_INFO_IMEI, buf, 1));
}

/* A bare AT+CGSN answer is taken as both the IMEI and the serial number */
ZTEST(modem_cellular_modem_info, test_imei_as_serial_number)
{
	char buf[MODEM_CELLULAR_DATA_IMEI_LEN];

	modem_restart(modem_info);
	advance_to(modem_info, BOOT_MODEM_INFO);
	test_backend_respond(TEST_BACKEND_PIPE_UART, TEST_IMEI_SN "\r");
	ok(TEST_BACKEND_PIPE_UART);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_IMEI_CMD);

	zassert_ok(cellular_get_modem_info(modem_info, CELLULAR_MODEM_INFO_IMEI, buf, sizeof(buf)));
	zassert_str_equal(TEST_IMEI_SN, buf);
	zassert_ok(cellular_get_modem_info(modem_info, CELLULAR_MODEM_INFO_SERIAL_NUMBER, buf,
					   sizeof(buf)));
	zassert_str_equal(TEST_IMEI_SN, buf);
}

/* An IMEI that is not a quoted 15 digit string is ignored */
ZTEST(modem_cellular_modem_info, test_invalid_imei)
{
	char buf[MODEM_CELLULAR_DATA_IMEI_LEN];

	modem_restart(modem_info);
	advance_to(modem_info, BOOT_MODEM_INFO);
	test_backend_respond(TEST_BACKEND_PIPE_UART, TEST_IMEI_SN "\r");
	ok(TEST_BACKEND_PIPE_UART);

	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_IMEI_CMD);
	test_backend_respond(TEST_BACKEND_PIPE_UART, "+CGSN: 123\r");
	ok(TEST_BACKEND_PIPE_UART);
	expect_line(TEST_BACKEND_PIPE_UART, MODEM_CELLULAR_TEST_SERIAL_NUMBER_CMD);

	/* Still the IMEI from the previous command */
	zassert_ok(cellular_get_modem_info(modem_info, CELLULAR_MODEM_INFO_IMEI, buf, sizeof(buf)));
	zassert_str_equal(TEST_IMEI_SN, buf);
}

ZTEST_SUITE(modem_cellular_modem_info, NULL, modem_test_setup, modem_test_before, modem_test_after,
	    NULL);
