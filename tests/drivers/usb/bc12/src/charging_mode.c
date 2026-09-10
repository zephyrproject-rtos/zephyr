/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup driver_bc12_tests
 * @ingroup all_tests
 * @{
 * @defgroup t_bc12_charging_mode
 * @brief TestPurpose: Verify BC1.2 devices configured in charging mode.
 * @}
 */

#include <zephyr/drivers/usb/usb_bc12.h>
#include <zephyr/drivers/usb/emul_bc12.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME test_bc12_charging
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

struct bc12_pi3usb9201_charging_mode_fixture {
	const struct device *bc12_dev;
	const struct emul *bc12_emul;
	struct bc12_partner_state partner_state;
};

static void bc12_test_result_cb(const struct device *dev, struct bc12_partner_state *state,
				void *user_data)
{
	struct bc12_pi3usb9201_charging_mode_fixture *fixture = user_data;

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
		fixture->partner_state.type = BC12_TYPE_NONE;
		fixture->partner_state.current_ua = 0;
		fixture->partner_state.voltage_uv = 0;
	}
}

ZTEST_USER_F(bc12_pi3usb9201_charging_mode, test_bc12_charging_mode)
{
	bc12_set_role(fixture->bc12_dev, BC12_CHARGING_PORT);

	bc12_emul_set_pd_partner(fixture->bc12_emul, true);
	k_sleep(K_MSEC(100));

	/* The BC1.2 driver should invoke the callback on plug event */
	zassert_true(fixture->partner_state.pd_partner_connected);

	bc12_emul_set_pd_partner(fixture->bc12_emul, false);
	k_sleep(K_MSEC(100));

	/* The BC1.2 driver should invoke the callback on unplug event */
	zassert_false(fixture->partner_state.pd_partner_connected);
}

static void bc12_before(void *data)
{
	struct bc12_pi3usb9201_charging_mode_fixture *fixture = data;

	memset(&fixture->partner_state, 0, sizeof(struct bc12_partner_state));

	bc12_set_result_cb(fixture->bc12_dev, &bc12_test_result_cb, fixture);
}

static void bc12_after(void *data)
{
	struct bc12_pi3usb9201_charging_mode_fixture *fixture = data;

	bc12_set_result_cb(fixture->bc12_dev, NULL, NULL);
	bc12_set_role(fixture->bc12_dev, BC12_DISCONNECTED);
}

static void *bc12_setup(void)
{
	static struct bc12_pi3usb9201_charging_mode_fixture fixture = {
		.bc12_dev = DEVICE_DT_GET(DT_ALIAS(bc12)),
		.bc12_emul = EMUL_DT_GET(DT_ALIAS(bc12)),
	};

	zassert_not_null(fixture.bc12_dev);
	zassert_not_null(fixture.bc12_emul);
	zassert_true(device_is_ready(fixture.bc12_dev));

	return &fixture;
}

ZTEST_SUITE(bc12_pi3usb9201_charging_mode, NULL, bc12_setup, bc12_before, bc12_after, NULL);
