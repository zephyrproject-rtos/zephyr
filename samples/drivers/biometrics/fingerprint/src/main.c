/*
 * Copyright (c) 2025 Siratul Islam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/biometrics.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define FINGERPRINT_NODE DT_ALIAS(fingerprint)

#if !DT_NODE_EXISTS(FINGERPRINT_NODE)
#error "Fingerprint sensor not defined. Add 'fingerprint = &fingerprint;' to your DTS aliases"
#endif

static const struct device *const fingerprint = DEVICE_DT_GET(FINGERPRINT_NODE);

static void print_capabilities(void)
{
	struct biometric_capabilities caps;
	int ret;

	ret = biometric_get_capabilities(fingerprint, &caps);
	if (ret < 0) {
		LOG_ERR("Failed to get capabilities: %d", ret);
		return;
	}

	LOG_INF("Sensor capabilities:");
	LOG_INF("  Max templates: %u", caps.max_templates);
	LOG_INF("  Template size: %u bytes", caps.template_size);
	LOG_INF("  Enrollment samples: %u", caps.enrollment_samples_required);
}

static int enroll_fingerprint(uint16_t template_id)
{
	struct biometric_capabilities caps;
	struct biometric_capture_result capture = {0};
	int ret;

	LOG_INF("Enrolling finger ID %u", template_id);

	ret = biometric_get_capabilities(fingerprint, &caps);
	if (ret < 0) {
		return ret;
	}

	ret = biometric_enroll_start(fingerprint, template_id);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("Place finger...");

	do {
		ret = biometric_enroll_capture(fingerprint, K_SECONDS(5), &capture);
		if (ret < 0) {
			biometric_enroll_abort(fingerprint);
			return ret;
		}

		LOG_INF("Sample %u/%u captured (quality: %u)", capture.samples_captured,
			caps.enrollment_samples_required, capture.quality);

		if (capture.samples_captured < caps.enrollment_samples_required) {
			LOG_INF("Lift finger and place again...");
			k_sleep(K_MSEC(500));
		}

	} while (capture.samples_captured < caps.enrollment_samples_required);

	ret = biometric_enroll_finalize(fingerprint);
	if (ret == 0) {
		LOG_INF("Enrollment complete");
	}

	return ret;
}

static void test_verification(uint16_t template_id)
{
	struct biometric_match_result result;
	int ret;

	LOG_INF("Verifying finger against ID %u", template_id);
	LOG_INF("Place finger...");

	ret = biometric_match(fingerprint, BIOMETRIC_MATCH_VERIFY, template_id, K_SECONDS(5),
			      &result);

	if (ret == 0) {
		LOG_INF("Match! Confidence: %d, Quality: %u", result.confidence,
			result.image_quality);
	} else if (ret == -ENOENT) {
		LOG_WRN("No match");
	} else if (ret == -ETIMEDOUT) {
		LOG_WRN("Timeout");
	} else {
		LOG_ERR("Verification failed: %d", ret);
	}
}

static void test_identification(void)
{
	struct biometric_match_result result;
	int ret;

	LOG_INF("Searching for any enrolled finger...");

	ret = biometric_match(fingerprint, BIOMETRIC_MATCH_IDENTIFY, 0, K_SECONDS(5), &result);

	if (ret == 0) {
		LOG_INF("Matched ID %u, confidence %d, quality %u", result.template_id,
			result.confidence, result.image_quality);
	} else if (ret == -ENOENT) {
		LOG_WRN("No match found");
	} else if (ret == -ETIMEDOUT) {
		LOG_WRN("Timeout");
	} else {
		LOG_ERR("Identification failed: %d", ret);
	}
}

static void list_templates(void)
{
	uint16_t ids[50];
	size_t count;
	int ret;

	ret = biometric_template_list(fingerprint, ids, ARRAY_SIZE(ids), &count);
	if (ret < 0) {
		LOG_ERR("Failed to list templates: %d", ret);
		return;
	}

	if (count == 0) {
		LOG_INF("No templates stored");
		return;
	}

	LOG_INF("Found %zu stored template(s):", count);
	for (size_t i = 0; i < count; i++) {
		LOG_INF("  - Template ID: %u", ids[i]);
	}
}

int main(void)
{
	LOG_INF("Fingerprint Sensor Test");

	if (!device_is_ready(fingerprint)) {
		LOG_ERR("Sensor not ready");
		return -ENODEV;
	}

	print_capabilities();

	LOG_INF("Enroll two fingers");
	enroll_fingerprint(1);
	enroll_fingerprint(2);

	list_templates();

	test_verification(1);
	test_identification();

	biometric_template_delete(fingerprint, 1);
	list_templates();

	LOG_INF("Test complete");
	return 0;
}
