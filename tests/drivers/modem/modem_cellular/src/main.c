/*
 * Copyright (c) 2026 Ahnaf Shahriar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/modem/modem_cellular.h>

static struct modem_cellular_data test_data;
static uint32_t network_status_events;

static void test_event_dispatch_handler(struct k_work *item)
{
	ARG_UNUSED(item);
}

static void test_event_cb(const struct device *dev, enum cellular_event event,
			  const void *payload, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(payload);
	ARG_UNUSED(user_data);

	if (event == CELLULAR_EVENT_NETWORK_STATUS_CHANGED) {
		network_status_events++;
	}
}

/* Feed one +C*REG read command answer, "+CxREG: <n>,<stat>", to the driver */
static void test_feed_cxreg(char *prefix, char *stat)
{
	char *argv[3];

	argv[0] = prefix;
	argv[1] = "2";
	argv[2] = stat;

	modem_cellular_chat_on_cxreg(NULL, argv, ARRAY_SIZE(argv), &test_data);
}

static void *test_suite_setup(void)
{
	k_work_init(&test_data.event_dispatch_work, test_event_dispatch_handler);
	k_pipe_init(&test_data.event_pipe, test_data.event_buf, sizeof(test_data.event_buf));

	test_data.cb.fn = test_event_cb;
	test_data.cb.mask = CELLULAR_EVENT_NETWORK_STATUS_CHANGED;

	return NULL;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_data.registration_status_gsm = CELLULAR_REGISTRATION_NOT_REGISTERED;
	test_data.registration_status_gprs = CELLULAR_REGISTRATION_NOT_REGISTERED;
	test_data.registration_status_lte = CELLULAR_REGISTRATION_NOT_REGISTERED;
	test_data.registration_status_5g = CELLULAR_REGISTRATION_NOT_REGISTERED;
	network_status_events = 0;
}

/* +C5GREG must not be filed as an LTE registration status */
ZTEST(modem_cellular, test_c5greg_own_status)
{
	test_feed_cxreg("+C5GREG: ", "1");

	zassert_equal(test_data.registration_status_5g,
		      CELLULAR_REGISTRATION_REGISTERED_HOME,
		      "+C5GREG did not update the 5G registration status");
	zassert_equal(test_data.registration_status_lte,
		      CELLULAR_REGISTRATION_NOT_REGISTERED,
		      "+C5GREG overwrote the LTE registration status");
}

/* Positive control: losing the only registration must be reported */
ZTEST(modem_cellular, test_cereg_deregistration_is_reported)
{
	test_feed_cxreg("+CEREG: ", "1");
	zassert_equal(network_status_events, 0, "Registration reported as a status change");

	test_feed_cxreg("+CEREG: ", "0");
	zassert_equal(network_status_events, 1, "Deregistration was not reported");
}

/* A module registered on 5G answers "+CEREG: <n>,0" and "+C5GREG: <n>,1".
 * The periodic +CEREG? read must not deregister such a module.
 */
ZTEST(modem_cellular, test_cereg_does_not_clear_5g_registration)
{
	test_feed_cxreg("+C5GREG: ", "1");
	test_feed_cxreg("+CEREG: ", "0");

	zassert_equal(network_status_events, 0,
		      "Modem deregistered while still registered on 5G");
	zassert_equal(test_data.registration_status_5g,
		      CELLULAR_REGISTRATION_REGISTERED_HOME,
		      "+CEREG cleared the 5G registration status");
}

ZTEST_SUITE(modem_cellular, NULL, test_suite_setup, test_before, NULL, NULL);
