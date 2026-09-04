/*
 * SPDX-FileCopyrightText: 2026 NXP
 * SPDX-FileCopyrightText: 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ADGM3121 mux backend test. The device is modeled as a single control line
 * (#mux-control-cells = 0) whose state is a 4-bit switch bitmask. The suite
 * drives the mask with mux_control_set(), opens all switches with
 * mux_control_disconnect(), and reads the mask back with mux_state_get().
 *
 * On the GPIO-control fixture (native_sim) it additionally confirms the four
 * switch lines physically follow the mask via gpio_emul_output_get_dt().
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/ztest.h>

#if IS_ENABLED(CONFIG_TEST_ADGM3121_OBSERVE_GPIO)
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

static const struct gpio_dt_spec in_gpios[4] = {
	GPIO_DT_SPEC_GET(DT_NODELABEL(adgm3121), in1_gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(adgm3121), in2_gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(adgm3121), in3_gpios),
	GPIO_DT_SPEC_GET(DT_NODELABEL(adgm3121), in4_gpios),
};

static void assert_switch_lines(uint8_t mask)
{
	for (int i = 0; i < 4; i++) {
		zassert_equal(gpio_emul_output_get_dt(&in_gpios[i]), (mask >> i) & 0x1U,
			      "switch line %d does not match mask bit", i);
	}
}
#else
static void assert_switch_lines(uint8_t mask)
{
	ARG_UNUSED(mask);
}
#endif

#define CONSUMER DT_NODELABEL(mux_consumer)

BUILD_ASSERT(DT_NODE_EXISTS(CONSUMER),
	     "board overlay must define a 'mux_consumer' node for this test");

MUX_CONTROL_DT_SPEC_DEFINE(CONSUMER);

static const struct device *const mux_dev = MUX_CONTROL_DT_DEV_GET(CONSUMER);

ZTEST(adgm3121, test_dev_ready)
{
	zassert_true(device_is_ready(mux_dev), "adgm3121 controller not ready");
}

ZTEST(adgm3121, test_set_disconnect_reconnect)
{
	const struct mux_control *ctrl = MUX_CONTROL_DT_GET(CONSUMER);
	uint32_t state;

	/* Set a mix of switches: SW2 + SW4 closed. */
	zassert_ok(mux_control_set(mux_dev, ctrl, 0x0A), "set 0x0A failed");
	zassert_ok(mux_state_get(mux_dev, ctrl, &state), "get failed");
	zassert_equal(state, 0x0A, "read-back should match the set mask");
	assert_switch_lines(0x0A);

	/* disconnect() opens every switch (the all-open state of the array). */
	zassert_ok(mux_control_disconnect(mux_dev, ctrl), "disconnect failed");
	zassert_ok(mux_state_get(mux_dev, ctrl, &state), "get after disconnect failed");
	zassert_equal(state, 0x00, "disconnect should turn all switches off");
	assert_switch_lines(0x00);

	/* A subsequent set() reconnects with the new mask: SW1 + SW3 closed. */
	zassert_ok(mux_control_set(mux_dev, ctrl, 0x05), "reconnect set 0x05 failed");
	zassert_ok(mux_state_get(mux_dev, ctrl, &state), "get after reconnect failed");
	zassert_equal(state, 0x05, "read-back should track the reconnect mask");
	assert_switch_lines(0x05);
}

ZTEST(adgm3121, test_state_out_of_range)
{
	const struct mux_control *ctrl = MUX_CONTROL_DT_GET(CONSUMER);

	/* Only the low 4 bits are valid switch selectors. */
	zassert_equal(mux_control_set(mux_dev, ctrl, 0x10), -EINVAL,
		      "state beyond the 4-bit mask should be rejected");
}

ZTEST_SUITE(adgm3121, NULL, NULL, NULL, NULL, NULL);
