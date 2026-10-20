/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"

#include <zephyr/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/ztest.h>

#include <stdio.h>
#include <string.h>

static const struct device *const modem = DEVICE_DT_GET(DT_ALIAS(modem));

/* Out of range for each enumeration */
#define INVALID_SIGNAL_TYPE ((enum cellular_signal_type)99)
#define INVALID_INFO_TYPE   ((enum cellular_modem_info_type)99)
#define INVALID_ACCESS_TECH ((enum cellular_access_technology)99)

/* An unknown signal type is rejected without querying the modem */
ZTEST(modem_cellular_api, test_get_signal)
{
	int16_t value = 0;

	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	zassert_equal(-ENOTSUP, cellular_get_signal(modem, INVALID_SIGNAL_TYPE, &value));
	zassert_equal(0, value, "value written");
	expect_no_line(K_MSEC(100));
}

ZTEST(modem_cellular_api, test_get_modem_info)
{
	char buf[MODEM_CELLULAR_DATA_IMEI_LEN];

	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	zassert_equal(-ENODATA, cellular_get_modem_info(modem, INVALID_INFO_TYPE, buf,
							 sizeof(buf)));
	zassert_equal(-EINVAL, cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_IMEI, buf, 0));
	zassert_equal(-EINVAL, cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_IMEI, buf, 1));
}

ZTEST(modem_cellular_api, test_get_registration_status)
{
	enum cellular_registration_status status;

	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	zassert_equal(-ENODATA, cellular_get_registration_status(
					modem, CELLULAR_ACCESS_TECHNOLOGY_UNKNOWN, &status));
	zassert_equal(-ENODATA, cellular_get_registration_status(modem, INVALID_ACCESS_TECH,
								  &status));
}

/* Missing, empty and oversized APNs are rejected and leave the driver waiting for one */
ZTEST(modem_cellular_api, test_set_apn_invalid)
{
	/* One character more than the driver stores, with its terminator */
	char apn[MODEM_CELLULAR_DATA_APN_LEN + 1];
	char cmd[sizeof(apn) + 32];

	modem_restart(modem);
	advance_to(modem, BOOT_AWAIT_APN);

	memset(apn, 'a', MODEM_CELLULAR_DATA_APN_LEN);
	apn[MODEM_CELLULAR_DATA_APN_LEN] = '\0';
	zassert_equal(-EINVAL, cellular_set_apn(modem, NULL));
	zassert_equal(-EINVAL, cellular_set_apn(modem, ""));
	zassert_equal(-EINVAL, cellular_set_apn(modem, apn), "APN of %zu accepted", strlen(apn));
	expect_no_line(K_MSEC(100));

	/* The longest that fits is configured */
	apn[MODEM_CELLULAR_DATA_APN_LEN - 1] = '\0';
	zassert_ok(cellular_set_apn(modem, apn));
	snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", apn);
	expect_line(TEST_BACKEND_PIPE_DLCI1, cmd);
}

/* Once configured, the APN cannot be set again or changed while connected */
ZTEST(modem_cellular_api, test_set_apn_configured)
{
	modem_restart(modem);
	advance_to(modem, BOOT_LINK_UP);

	zassert_equal(-EALREADY, cellular_set_apn(modem, TEST_APN));
	zassert_equal(-EBUSY, cellular_set_apn(modem, "other"));
	expect_no_line(K_MSEC(100));
}

ZTEST_SUITE(modem_cellular_api, NULL, modem_test_setup, modem_test_before, modem_test_after, NULL);
