/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup driver_bc12_tests
 * @ingroup all_tests
 * @{
 * @defgroup t_bc12_portable_device
 * @brief TestPurpose: Verify BC1.2 devices in portable device mode.
 * @}
 */

#include <zephyr/drivers/usb/usb_bc12.h>
#include <zephyr/drivers/usb/emul_bc12.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME test_bc12_pd_mode
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

struct bc12_pd_mode_fixture {
	const struct device *bc12_dev;
	const struct emul *bc12_emul;
	int callback_count;
	bool disconnect_detected;
	struct bc12_partner_state partner_state;
};

static void bc12_test_result_cb(const struct device *dev, struct bc12_partner_state *state,
				void *user_data)
{
	struct bc12_pd_mode_fixture *fixture = user_data;

	fixture->callback_count++;

	if (state) {
		if (state->bc12_role == BC12_PORTABLE_DEVICE) {
			LOG_INF("charging partner: type %d, voltage %d, current %d", state->type,
				state->voltage_uv, state->current_ua);
		} else if (state->bc12_role == BC12_CHARGING_PORT) {
			LOG_INF("portable device partner: connected %d",
				state->pd_partner_connected);
		}
		fixture->partner_state = *state;
	} else {
		LOG_INF("callback: partner disconnect");
		fixture->disconnect_detected = true;
		fixture->partner_state.type = BC12_TYPE_NONE;
		fixture->partner_state.current_ua = 0;
		fixture->partner_state.voltage_uv = 0;
	}
}

ZTEST_USER_F(bc12_pd_mode, test_bc12_no_charging_partner)
{
	bc12_emul_set_charging_partner(fixture->bc12_emul, BC12_TYPE_NONE);

	bc12_set_role(fixture->bc12_dev, BC12_PORTABLE_DEVICE);

	k_sleep(K_MSEC(100));

	/* Without any device connected, our callback should not execute */
	zassert_equal(fixture->callback_count, 0);
}

ZTEST_USER_F(bc12_pd_mode, test_bc12_sdp_charging_partner)
{
	/* Connect a SDP charging partner to the emulator */
	bc12_emul_set_charging_partner(fixture->bc12_emul, BC12_TYPE_SDP);

	/* Report to the BC1.2 driver that VBUS is present */
	bc12_set_role(fixture->bc12_dev, BC12_PORTABLE_DEVICE);

	k_sleep(K_MSEC(100));

	/*
	 * Note that in SDP mode, the USB device is limited to 2.5 mA until
	 * the USB bus is not suspended or the USB device configured.
	 *
	 * The BC1.2 driver contract specifies to set the current to Isusp
	 * for SDP ports or when BC1.2 detection fails.
	 */
	zassert_equal(fixture->callback_count, 1);
	zassert_equal(fixture->partner_state.bc12_role, BC12_PORTABLE_DEVICE);
	zassert_equal(fixture->partner_state.type, BC12_TYPE_SDP);
	zassert_equal(fixture->partner_state.current_ua, 2500);
	zassert_equal(fixture->partner_state.voltage_uv, 5000 * 1000);

	/* Remove the charging partner */
	fixture->callback_count = 0;
	bc12_emul_set_charging_partner(fixture->bc12_emul, BC12_TYPE_NONE);

	/* Report to the BC1.2 driver that VBUS is no longer present */
	bc12_set_role(fixture->bc12_dev, BC12_DISCONNECTED);

	k_sleep(K_MSEC(100));

	/* The BC1.2 driver should invoke the callback on disconnects */
	zassert_equal(fixture->callback_count, 1);
	zassert_equal(fixture->partner_state.type, BC12_TYPE_NONE);
	zassert_equal(fixture->partner_state.current_ua, 0);
	zassert_equal(fixture->partner_state.voltage_uv, 0);
}

ZTEST_USER_F(bc12_pd_mode, test_bc12_cdp_charging_partner)
{
	bc12_emul_set_charging_partner(fixture->bc12_emul, BC12_TYPE_CDP);

	bc12_set_role(fixture->bc12_dev, BC12_PORTABLE_DEVICE);

	k_sleep(K_MSEC(100));

	zassert_equal(fixture->callback_count, 1);
	zassert_equal(fixture->partner_state.bc12_role, BC12_PORTABLE_DEVICE);
	zassert_equal(fixture->partner_state.type, BC12_TYPE_CDP);
	zassert_equal(fixture->partner_state.current_ua, 1500 * 1000);
	zassert_equal(fixture->partner_state.voltage_uv, 5000 * 1000);

	/* Remove the charging partner */
	fixture->callback_count = 0;
	bc12_emul_set_charging_partner(fixture->bc12_emul, BC12_TYPE_NONE);

	/* Report to the BC1.2 driver that VBUS is no longer present */
	bc12_set_role(fixture->bc12_dev, BC12_DISCONNECTED);

	k_sleep(K_MSEC(100));

	/* The BC1.2 driver should invoke the callback on disconnects */
	zassert_equal(fixture->callback_count, 1);
	zassert_true(fixture->disconnect_detected);
	zassert_equal(fixture->partner_state.type, BC12_TYPE_NONE);
	zassert_equal(fixture->partner_state.current_ua, 0);
	zassert_equal(fixture->partner_state.voltage_uv, 0);
}

ZTEST_USER_F(bc12_pd_mode, test_bc12_sdp_to_dcp_charging_partner)
{
	bc12_emul_set_charging_partner(fixture->bc12_emul, BC12_TYPE_SDP);

	bc12_set_role(fixture->bc12_dev, BC12_PORTABLE_DEVICE);

	k_sleep(K_MSEC(100));

	zassert_equal(fixture->callback_count, 1);
	zassert_equal(fixture->partner_state.bc12_role, BC12_PORTABLE_DEVICE);
	zassert_equal(fixture->partner_state.type, BC12_TYPE_SDP);
	zassert_equal(fixture->partner_state.current_ua, 2500);
	zassert_equal(fixture->partner_state.voltage_uv, 5000 * 1000);

	/* Change the partner type to DCP */
	fixture->callback_count = 0;
	bc12_emul_set_charging_partner(fixture->bc12_emul, BC12_TYPE_DCP);

	/* Trigger a new detection */
	bc12_set_role(fixture->bc12_dev, BC12_PORTABLE_DEVICE);

	k_sleep(K_MSEC(100));

	/* The BC1.2 driver should invoke the callback once to report the new state. */
	zassert_equal(fixture->callback_count, 1);
	zassert_equal(fixture->partner_state.bc12_role, BC12_PORTABLE_DEVICE);
	zassert_equal(fixture->partner_state.type, BC12_TYPE_DCP);
	zassert_equal(fixture->partner_state.current_ua, 1500 * 1000);
	zassert_equal(fixture->partner_state.voltage_uv, 5000 * 1000);
}

static void bc12_before(void *data)
{
	struct bc12_pd_mode_fixture *fixture = data;

	fixture->callback_count = 0;
	fixture->disconnect_detected = 0;
	memset(&fixture->partner_state, 0, sizeof(struct bc12_partner_state));

	bc12_set_result_cb(fixture->bc12_dev, &bc12_test_result_cb, fixture);
}

static void bc12_after(void *data)
{
	struct bc12_pd_mode_fixture *fixture = data;

	bc12_set_result_cb(fixture->bc12_dev, NULL, NULL);
	bc12_set_role(fixture->bc12_dev, BC12_DISCONNECTED);
}

static void *bc12_setup(void)
{
	static struct bc12_pd_mode_fixture fixture = {
		.bc12_dev = DEVICE_DT_GET(DT_ALIAS(bc12)),
		.bc12_emul = EMUL_DT_GET(DT_ALIAS(bc12)),
	};

	zassert_not_null(fixture.bc12_dev);
	zassert_not_null(fixture.bc12_emul);
	zassert_true(device_is_ready(fixture.bc12_dev));

	return &fixture;
}

ZTEST_SUITE(bc12_pd_mode, NULL, bc12_setup, bc12_before, bc12_after, NULL);
