/*
 * Copyright (c) 2024 Adrien Leravat
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_STATUS(DT_INST(0, hc_sr04), okay)
#define HC_SR04 DT_NODELABEL(hc_sr04)
#define HC_SR04_GPIO_OUT DT_GPIO_CTLR(DT_INST(0, hc_sr04), trigger_gpios)
#define HC_SR04_PIN_OUT DT_GPIO_PIN(DT_INST(0, hc_sr04), trigger_gpios)
#define HC_SR04_GPIO_IN DT_GPIO_CTLR(DT_INST(0, hc_sr04), echo_gpios)
#define HC_SR04_PIN_IN DT_GPIO_PIN(DT_INST(0, hc_sr04), echo_gpios)
#else
#error "HC-SR04 not enabled"
#endif

#define TEST_MEASURED_VALUE(fixture, value, duration_us, value1, value2)                   \
	fixture->emul.echo_duration_us = duration_us;                                      \
	zassert_false(sensor_sample_fetch(fixture->dev), "sensor_sample_fetch failed");    \
	zassert_false(sensor_channel_get(fixture->dev, SENSOR_CHAN_DISTANCE, &value),      \
			"sensor_channel_get failed");                                      \
	zassert_equal(value.val1, value1, "incorrect measurement for value.val1");         \
	zassert_within(value.val2, value2, 10000, "incorrect measurement for value.val2"); \

struct hcsr04_emul {
	bool fail_echo;
	uint32_t echo_duration_us;
	struct gpio_callback cb;
};

struct hcsr04_fixture {
	const struct device *dev;
	struct hcsr04_emul emul;
};

static void gpio_emul_callback_handler(const struct device *port,
					struct gpio_callback *cb,
					gpio_port_pins_t pins);

static void *hcsr04_setup(void)
{
	static struct hcsr04_fixture fixture = {
		.dev = DEVICE_DT_GET(HC_SR04),
		.emul = {
			.fail_echo = false,
			.echo_duration_us = 0
		}
	};
	const struct device *gpio_dev = DEVICE_DT_GET(HC_SR04_GPIO_IN);

	zassert_not_null(fixture.dev);
	zassert_not_null(gpio_dev);
	zassert_true(device_is_ready(fixture.dev));
	zassert_equal(DEVICE_DT_GET(HC_SR04_GPIO_IN), DEVICE_DT_GET(HC_SR04_GPIO_OUT),
			"Input and output GPIO devices must the same");

	zassert_true(device_is_ready(gpio_dev), "GPIO dev is not ready");

	gpio_init_callback(&fixture.emul.cb, &gpio_emul_callback_handler, BIT(HC_SR04_PIN_OUT));
	zassert_false(gpio_add_callback(gpio_dev, &fixture.emul.cb),
			"Failed to add emulation callback");

	return &fixture;
}

static void hcsr04_before(void *f)
{
	struct hcsr04_fixture *fixture = f;

	fixture->emul.fail_echo = false;
}

static void gpio_emul_callback_handler(const struct device *port,
					struct gpio_callback *cb,
					gpio_port_pins_t pins)
{
	const struct hcsr04_emul *emul = CONTAINER_OF(cb, struct hcsr04_emul, cb);

	if (emul->fail_echo) {
		return;
	}
	if (gpio_emul_output_get(port, HC_SR04_PIN_OUT) == 1) {
		/* Ignore rising edge */
		return;
	}

	/* Output high-level on echo pin */
	gpio_emul_input_set(port, HC_SR04_PIN_IN, 1);
	k_busy_wait(emul->echo_duration_us);
	gpio_emul_input_set(port, HC_SR04_PIN_IN, 0);
}

ZTEST_SUITE(hcsr04, NULL, hcsr04_setup, hcsr04_before, NULL, NULL);

ZTEST_USER_F(hcsr04, test_sample_fetch_fail_no_echo)
{
	int ret;

	fixture->emul.fail_echo = true;

	ret = sensor_sample_fetch(fixture->dev);
	zassert_equal(-EIO, ret, "sensor_sample_fetch unexpected return code %d", ret);
}

ZTEST_USER_F(hcsr04, test_sample_fetch)
{
	int ret;

	ret = sensor_sample_fetch(fixture->dev);
	zassert_equal(0, ret, "sensor_sample_fetch unexpected return code %d", ret);
}

ZTEST_USER_F(hcsr04, test_channel_get_fails_with_wrong_channel)
{
	int ret;
	struct sensor_value value;

	ret = sensor_channel_get(fixture->dev, SENSOR_CHAN_ACCEL_X, &value);
	zassert_equal(-ENOTSUP, ret, "sensor_channel_get returned unexpected code with %d", ret);
}

ZTEST_USER_F(hcsr04, test_channel_get_at_10cm)
{
	struct sensor_value value;

	TEST_MEASURED_VALUE(fixture, value, 583, 0, 100000);
}

ZTEST_USER_F(hcsr04, test_channel_get_at_150cm)
{
	struct sensor_value value;

	TEST_MEASURED_VALUE(fixture, value, 8745, 1, 500000);
}
