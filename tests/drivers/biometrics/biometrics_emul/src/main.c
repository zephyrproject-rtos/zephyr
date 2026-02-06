/*
 * Copyright (c) 2025 Siratul Islam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/biometrics.h>
#include <zephyr/drivers/biometrics/emul.h>

static const struct device *get_biometrics_device(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(biometrics));

	zassert_not_null(dev, "Cannot get biometrics device");
	zassert_true(device_is_ready(dev), "Biometrics device not ready");

	return dev;
}

ZTEST(biometrics_emul, test_get_capabilities)
{
	const struct device *dev = get_biometrics_device();
	struct biometric_capabilities caps;
	int ret;

	ret = biometric_get_capabilities(dev, &caps);
	zassert_equal(ret, 0, "Failed to get capabilities: %d", ret);

	zassert_equal(caps.type, BIOMETRIC_TYPE_FINGERPRINT, "Expected fingerprint sensor type");
	zassert_true(caps.max_templates > 0, "max_templates should be > 0");
	zassert_true(caps.template_size > 0, "template_size should be > 0");
	zassert_true(caps.storage_modes & BIOMETRIC_STORAGE_DEVICE,
		     "Should support device storage");
	zassert_true(caps.enrollment_samples_required > 0,
		     "enrollment_samples_required should be > 0");
}

ZTEST(biometrics_emul, test_attr_set_get)
{
	const struct device *dev = get_biometrics_device();
	int32_t val;
	int ret;

	ret = biometric_attr_set(dev, BIOMETRIC_ATTR_MATCH_THRESHOLD, 75);
	zassert_equal(ret, 0, "Failed to set match threshold: %d", ret);

	ret = biometric_attr_get(dev, BIOMETRIC_ATTR_MATCH_THRESHOLD, &val);
	zassert_equal(ret, 0, "Failed to get match threshold: %d", ret);
	zassert_equal(val, 75, "Match threshold mismatch");

	ret = biometric_attr_set(dev, BIOMETRIC_ATTR_SECURITY_LEVEL, 5);
	zassert_equal(ret, 0, "Failed to set security level");

	ret = biometric_attr_set(dev, BIOMETRIC_ATTR_SECURITY_LEVEL, 15);
	zassert_equal(ret, -EINVAL, "Should reject invalid security level");

	ret = biometric_attr_set(dev, BIOMETRIC_ATTR_SECURITY_LEVEL, 0);
	zassert_equal(ret, -EINVAL, "Should reject invalid security level");

	ret = biometric_attr_set(dev, BIOMETRIC_ATTR_IMAGE_QUALITY, 100);
	zassert_equal(ret, -EACCES, "IMAGE_QUALITY should be read-only");
}

ZTEST(biometrics_emul, test_enrollment_flow)
{
	const struct device *dev = get_biometrics_device();
	struct biometric_capabilities caps;
	struct biometric_capture_result capture;
	int ret;

	ret = biometric_get_capabilities(dev, &caps);
	zassert_equal(ret, 0, "Failed to get capabilities");

	ret = biometric_enroll_start(dev, 1);
	zassert_equal(ret, 0, "Failed to start enrollment: %d", ret);

	for (int i = 0; i < caps.enrollment_samples_required; i++) {
		ret = biometric_enroll_capture(dev, K_SECONDS(5), &capture);
		zassert_equal(ret, 0, "Failed to capture sample %d: %d", i, ret);
		zassert_equal(capture.samples_captured, i + 1, "samples_captured mismatch");
		zassert_equal(capture.samples_required, caps.enrollment_samples_required,
			      "samples_required mismatch");
	}

	ret = biometric_enroll_finalize(dev);
	zassert_equal(ret, 0, "Failed to finalize enrollment: %d", ret);

	uint16_t ids[10];
	size_t count;

	ret = biometric_template_list(dev, ids, ARRAY_SIZE(ids), &count);
	zassert_equal(ret, 0, "Failed to list templates: %d", ret);
	zassert_equal(count, 1, "Expected 1 template, got %zu", count);
	zassert_equal(ids[0], 1, "Template ID mismatch");

	biometric_template_delete(dev, 1);
}

ZTEST(biometrics_emul, test_enrollment_abort)
{
	const struct device *dev = get_biometrics_device();
	int ret;

	ret = biometric_enroll_start(dev, 10);
	zassert_equal(ret, 0, "Failed to start enrollment");

	ret = biometric_enroll_capture(dev, K_SECONDS(5), NULL);
	zassert_equal(ret, 0, "Failed to capture sample");

	ret = biometric_enroll_abort(dev);
	zassert_equal(ret, 0, "Failed to abort enrollment: %d", ret);

	uint16_t ids[10];
	size_t count;

	ret = biometric_template_list(dev, ids, ARRAY_SIZE(ids), &count);
	zassert_equal(ret, 0, "Failed to list templates");
	zassert_equal(count, 0, "No templates should exist after abort");
}

ZTEST(biometrics_emul, test_enrollment_errors)
{
	const struct device *dev = get_biometrics_device();
	int ret;

	ret = biometric_enroll_abort(dev);
	zassert_equal(ret, -EALREADY, "Abort without enrollment should fail");

	ret = biometric_enroll_finalize(dev);
	zassert_equal(ret, -EINVAL, "Finalize without enrollment should fail");

	ret = biometric_enroll_start(dev, 20);
	zassert_equal(ret, 0, "Failed to start enrollment");

	ret = biometric_enroll_start(dev, 21);
	zassert_equal(ret, -EBUSY, "Should not allow concurrent enrollment");

	biometric_enroll_abort(dev);
}

ZTEST(biometrics_emul, test_template_operations)
{
	const struct device *dev = get_biometrics_device();
	uint8_t template_data[512];
	uint8_t read_buf[512];
	int ret;

	memset(template_data, 0xAB, sizeof(template_data));

	ret = biometric_template_store(dev, 100, template_data, sizeof(template_data));
	zassert_equal(ret, 0, "Failed to store template: %d", ret);

	ret = biometric_template_read(dev, 100, read_buf, sizeof(read_buf));
	zassert_true(ret > 0, "Failed to read template: %d", ret);
	zassert_mem_equal(template_data, read_buf, sizeof(template_data), "Template data mismatch");

	ret = biometric_template_delete(dev, 100);
	zassert_equal(ret, 0, "Failed to delete template: %d", ret);

	ret = biometric_template_read(dev, 100, read_buf, sizeof(read_buf));
	zassert_equal(ret, -ENOENT, "Template should not exist after deletion");
}

ZTEST(biometrics_emul, test_template_delete_all)
{
	const struct device *dev = get_biometrics_device();
	uint8_t template_data[64];
	uint16_t ids[10];
	size_t count;
	int ret;

	memset(template_data, 0x55, sizeof(template_data));

	for (int i = 0; i < 5; i++) {
		ret = biometric_template_store(dev, i + 200, template_data, sizeof(template_data));
		zassert_equal(ret, 0, "Failed to store template %d", i);
	}

	ret = biometric_template_list(dev, ids, ARRAY_SIZE(ids), &count);
	zassert_equal(ret, 0, "Failed to list templates");
	zassert_equal(count, 5, "Expected 5 templates");

	ret = biometric_template_delete_all(dev);
	zassert_equal(ret, 0, "Failed to delete all templates: %d", ret);

	ret = biometric_template_list(dev, ids, ARRAY_SIZE(ids), &count);
	zassert_equal(ret, 0, "Failed to list templates");
	zassert_equal(count, 0, "All templates should be deleted");
}

ZTEST(biometrics_emul, test_match_verify)
{
	const struct device *dev = get_biometrics_device();
	struct biometric_capabilities caps;
	struct biometric_match_result result;
	int ret;

	ret = biometric_get_capabilities(dev, &caps);
	zassert_equal(ret, 0, "Failed to get capabilities");

	ret = biometric_enroll_start(dev, 300);
	zassert_equal(ret, 0, "Failed to start enrollment");

	for (int i = 0; i < caps.enrollment_samples_required; i++) {
		ret = biometric_enroll_capture(dev, K_SECONDS(5), NULL);
		zassert_equal(ret, 0, "Failed to capture");
	}

	ret = biometric_enroll_finalize(dev);
	zassert_equal(ret, 0, "Failed to finalize");

	biometrics_emul_set_match_score(dev, 85);

	ret = biometric_match(dev, BIOMETRIC_MATCH_VERIFY, 300, K_SECONDS(5), &result);
	zassert_equal(ret, 0, "Match should succeed: %d", ret);
	zassert_equal(result.confidence, 85, "Expected confidence 85, got %d", result.confidence);
	zassert_equal(result.template_id, 300, "Expected template_id 300, got %u",
		      result.template_id);

	biometric_template_delete(dev, 300);
}

ZTEST(biometrics_emul, test_match_identify)
{
	const struct device *dev = get_biometrics_device();
	struct biometric_match_result result;
	uint8_t template_data[64];
	int ret;

	memset(template_data, 0x11, sizeof(template_data));

	biometric_template_store(dev, 400, template_data, sizeof(template_data));
	biometric_template_store(dev, 401, template_data, sizeof(template_data));
	biometric_template_store(dev, 402, template_data, sizeof(template_data));

	biometrics_emul_set_match_id(dev, 401);
	biometrics_emul_set_match_score(dev, 92);

	ret = biometric_match(dev, BIOMETRIC_MATCH_IDENTIFY, 0, K_SECONDS(5), &result);
	zassert_equal(ret, 0, "Identify should succeed: %d", ret);
	zassert_equal(result.confidence, 92, "Expected confidence 92, got %d", result.confidence);
	zassert_equal(result.template_id, 401, "Expected template_id 401, got %u",
		      result.template_id);

	biometric_template_delete_all(dev);
}

ZTEST(biometrics_emul, test_match_fail)
{
	const struct device *dev = get_biometrics_device();
	int ret;

	biometrics_emul_set_match_fail(dev, true);

	ret = biometric_match(dev, BIOMETRIC_MATCH_IDENTIFY, 0, K_SECONDS(5), NULL);
	zassert_equal(ret, -ENOENT, "Match should fail with ENOENT: %d", ret);

	biometrics_emul_set_match_fail(dev, false);
}

ZTEST(biometrics_emul, test_capture_timeout)
{
	const struct device *dev = get_biometrics_device();
	int ret;

	biometrics_emul_set_capture_timeout(dev, true);

	ret = biometric_enroll_start(dev, 500);
	zassert_equal(ret, 0, "Failed to start enrollment");

	ret = biometric_enroll_capture(dev, K_SECONDS(1), NULL);
	zassert_equal(ret, -ETIMEDOUT, "Capture should timeout: %d", ret);

	biometrics_emul_set_capture_timeout(dev, false);
	biometric_enroll_abort(dev);
}

ZTEST(biometrics_emul, test_led_control)
{
	const struct device *dev = get_biometrics_device();
	enum biometric_led_state state;
	int ret;

	ret = biometric_led_control(dev, BIOMETRIC_LED_OFF);
	zassert_equal(ret, 0, "Failed to set LED off");
	state = biometrics_emul_get_led_state(dev);
	zassert_equal(state, BIOMETRIC_LED_OFF, "LED state mismatch");

	ret = biometric_led_control(dev, BIOMETRIC_LED_ON);
	zassert_equal(ret, 0, "Failed to set LED on");
	state = biometrics_emul_get_led_state(dev);
	zassert_equal(state, BIOMETRIC_LED_ON, "LED state mismatch");

	ret = biometric_led_control(dev, BIOMETRIC_LED_BLINK);
	zassert_equal(ret, 0, "Failed to set LED blink");
	state = biometrics_emul_get_led_state(dev);
	zassert_equal(state, BIOMETRIC_LED_BLINK, "LED state mismatch");

	ret = biometric_led_control(dev, BIOMETRIC_LED_BREATHE);
	zassert_equal(ret, 0, "Failed to set LED breathe");
	state = biometrics_emul_get_led_state(dev);
	zassert_equal(state, BIOMETRIC_LED_BREATHE, "LED state mismatch");

	ret = biometric_led_control(dev, 99);
	zassert_equal(ret, -EINVAL, "Should reject invalid LED state");
}

static void *biometrics_emul_setup(void)
{
	return NULL;
}

static void biometrics_emul_before(void *fixture)
{
	ARG_UNUSED(fixture);

	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(biometrics));

	if (device_is_ready(dev)) {
		biometrics_emul_set_match_score(dev, 0);
		biometrics_emul_set_match_id(dev, 0);
		biometrics_emul_set_match_fail(dev, false);
		biometrics_emul_set_capture_timeout(dev, false);
		biometrics_emul_set_image_quality(dev, 0);

		biometric_template_delete_all(dev);
	}
}

ZTEST_SUITE(biometrics_emul, NULL, biometrics_emul_setup, biometrics_emul_before, NULL, NULL);
