/*
 * Copyright (c) 2024 THE ARC GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/shell/shell.h>
#include <zephyr/device.h>
#include <stdlib.h>
#include "dummy_sensor.h"

/* Longest name is currently 'gauge_desired_charging_current' = 30 */
#define MAX_CHANNEL_NAME_LENGTH 30

static const struct shell *sh;
static const struct device *dummy_sensor = DEVICE_DT_GET(DT_NODELABEL(dummy));

struct parsed_q31_data {
	int idx;
	char name[MAX_CHANNEL_NAME_LENGTH + 1];
	int shift;
	int num_samples;
	long long value;
	int int_value;
	int frac_value;
};

struct parsed_prox_data {
	int idx;
	char name[MAX_CHANNEL_NAME_LENGTH + 1];
	int num_samples;
	long long value;
	int is_near;
};

static int parse_q31_value_output(const char *str, struct parsed_q31_data *data)
{
	if (sscanf(str, "%*[^c]channel idx=%d "
			"%" STRINGIFY(MAX_CHANNEL_NAME_LENGTH) "s"
							       " shift=%d num_samples=%d "
							       "value=%lldns (%d.%d)%*s",
				      &data->idx, data->name, &data->shift, &data->num_samples,
				      &data->value, &data->int_value, &data->frac_value) == 7) {
	} else {
		return 1;
	}
	return 0;
}

static int parse_prox_value_output(const char *str, struct parsed_prox_data *data)
{
	if (sscanf(str, "%*[^c]channel idx=%d "
			"%" STRINGIFY(MAX_CHANNEL_NAME_LENGTH) "s"
							       " num_samples=%d value=%lldns "
							       "(is_near = %d)%*s",
				      &data->idx, data->name, &data->num_samples, &data->value,
				      &data->is_near) == 5) {
	} else {
		return 1;
	}
	return 0;
}

void before(void *data)
{
	ARG_UNUSED(data);
	int timeout = 100;

	ztest_simple_1cpu_before(NULL);
	sh = shell_backend_dummy_get_ptr();
	while (!shell_ready(sh) && timeout-- > 0) {
		k_sleep(K_MSEC(10));
	}
	zassert_not_equal(timeout, 0, "Timeout while waiting for shell.");
}

static const char *wait_for_next_shell_output(size_t minlen)
{
	size_t output_length;
	const char *shell_output;
	int timeout = 100;

	shell_output = shell_backend_dummy_get_output(sh, &output_length);
	while (output_length < minlen && timeout-- > 0) {
		k_sleep(K_MSEC(10));
		shell_output = shell_backend_dummy_get_output(sh, &output_length);
	}
	zassert_not_equal(timeout, 0, "Timeout while waiting for output.");
	return shell_output;
}

ZTEST(sensor_shell, test_sensor_get_light)
{
	const char *shell_output;
	struct parsed_q31_data parsed_data;
	struct sensor_value value;

	/* set light value to 1.000001 */
	value.val1 = 1;
	value.val2 = 1;
	dummy_sensor_set_value(dummy_sensor, SENSOR_CHAN_LIGHT, &value);
	shell_backend_dummy_clear_output(sh);
	zassert_ok(shell_execute_cmd(NULL, "sensor get dummy light"), "Failed cmd execution.");

	shell_output = wait_for_next_shell_output(50);
	zassert_ok(parse_q31_value_output(shell_output, &parsed_data),
		   "Parsing failed, expected output not found.");

	zassert_equal(parsed_data.int_value, 1);
	zassert_equal(parsed_data.frac_value, 0);
	zassert_equal(strcmp(parsed_data.name, "light"), 0, "Unexpected channel name");

	/* set light value to 2.000002 */
	value.val1 = 2;
	value.val2 = 2;
	dummy_sensor_set_value(dummy_sensor, SENSOR_CHAN_LIGHT, &value);
	shell_backend_dummy_clear_output(sh);
	zassert_ok(shell_execute_cmd(NULL, "sensor get dummy light"), "Failed cmd execution.");

	shell_output = wait_for_next_shell_output(50);
	zassert_ok(parse_q31_value_output(shell_output, &parsed_data),
		   "Parsing failed, expected output not found.");

	zassert_equal(parsed_data.int_value, 2);
	zassert_equal(parsed_data.frac_value, 1);
	zassert_equal(strcmp(parsed_data.name, "light"), 0, "Unexpected channel name");
}

ZTEST(sensor_shell, test_sensor_get_prox)
{
	const char *shell_output;
	struct parsed_prox_data parsed_data;
	struct sensor_value value;

	/* set to is_near=1 */
	value.val1 = 1;
	value.val2 = 0;
	dummy_sensor_set_value(dummy_sensor, SENSOR_CHAN_PROX, &value);
	shell_backend_dummy_clear_output(sh);
	zassert_ok(shell_execute_cmd(NULL, "sensor get dummy prox"), "Failed cmd execution.");

	shell_output = wait_for_next_shell_output(50);
	zassert_ok(parse_prox_value_output(shell_output, &parsed_data),
		   "Parsing failed, expected output not found.");

	zassert_equal(parsed_data.is_near, 1);
	zassert_equal(strcmp(parsed_data.name, "prox"), 0, "Unexpected channel name");

	/* set to is_near=0 */
	value.val1 = 0;
	value.val2 = 0;
	dummy_sensor_set_value(dummy_sensor, SENSOR_CHAN_PROX, &value);
	shell_backend_dummy_clear_output(sh);
	zassert_ok(shell_execute_cmd(NULL, "sensor get dummy prox"), "Failed cmd execution.");

	shell_output = wait_for_next_shell_output(50);
	zassert_ok(parse_prox_value_output(shell_output, &parsed_data),
		   "Parsing failed, expected output not found.");

	zassert_equal(parsed_data.is_near, 0);
	zassert_equal(strcmp(parsed_data.name, "prox"), 0, "Unexpected channel name");
}

ZTEST_SUITE(sensor_shell, NULL, NULL, before, ztest_simple_1cpu_after, NULL);
