/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include <ironside_client/psa/initial_attestation.h>
#include <ironside_client/services/boot_report.h>
#include <psa/crypto.h>

LOG_MODULE_REGISTER(secure_services, CONFIG_LOG_DEFAULT_LEVEL);

#define CHALLENGE_SIZE PSA_INITIAL_ATTEST_CHALLENGE_SIZE_32

static const char *booted_image_str(uint32_t booted_image)
{
	switch (booted_image) {
	case IRONSIDE_BOOTED_IMAGE_APP:
		return "application";
	case IRONSIDE_BOOTED_IMAGE_DFU:
		return "IronSide DFU image";
	default:
		__ASSERT(false, "unknown booted image %u", (unsigned int)booted_image);
		return "UNKNOWN";
	}
}

static const char *lifecycle_str(uint32_t lifecycle)
{
	switch (lifecycle) {
	case PSA_LIFECYCLE_ASSEMBLY_AND_TEST:
		return "assembly and test";
	case PSA_LIFECYCLE_PSA_ROT_PROVISIONING:
		return "PSA RoT provisioning";
	case PSA_LIFECYCLE_SECURED:
		return "secured";
	case PSA_LIFECYCLE_NON_PSA_ROT_DEBUG:
		return "non-PSA RoT debug";
	case PSA_LIFECYCLE_DECOMMISSIONED:
		return "decommissioned";
	default:
		__ASSERT(false, "unknown lifecycle 0x%04X", (unsigned int)lifecycle);
		return "UNKNOWN";
	}
}

static const char *dfu_reason_str(uint32_t dfu_reason)
{
	switch (dfu_reason) {
	case IRONSIDE_DFU_REASON_ADAC_BOOT_MODE:
		return "a debugger set the ADAC boot mode";
	case IRONSIDE_DFU_REASON_REQUEST_SERVICE:
		return "a non-secure image asked for it";
	case IRONSIDE_DFU_REASON_APP_SIGNATURE:
		return "the application's signature or header was rejected";
	case IRONSIDE_DFU_REASON_APP_ROLLBACK:
		return "the application is below its rollback counter";
	case IRONSIDE_DFU_REASON_APP_UNAVAILABLE:
		return "there is no usable application";
	case IRONSIDE_DFU_REASON_TRIGGER_RESETREAS:
		return "the configured reset-reason trigger fired";
	case IRONSIDE_DFU_REASON_TRIGGER_GPIO:
		return "the configured GPIO was active";
	default:
		__ASSERT(false, "unknown dfu reason %u", (unsigned int)dfu_reason);
		return "UNKNOWN";
	}
}

static void log_version(const char *image, struct ironside_image_version version)
{
	LOG_INF("%s v%u.%u.%u+%u", image, (unsigned int)version.major,
		(unsigned int)version.minor, (unsigned int)version.patch,
		(unsigned int)version.tweak);
}

static int boot_report(void)
{
	struct ironside_boot_report report;
	size_t len;
	int32_t err;

	err = ironside_boot_report_get(&report, sizeof(report), &len);
	if (err != 0) {
		LOG_ERR("ironside_boot_report_get() failed (%d).", err);
		return -EIO;
	}

	if (len != sizeof(report)) {
		LOG_ERR("Boot report is %zu bytes, the client describes %zu: this device's "
			"IronSide TE and this client library do not match.", len, sizeof(report));
		return -EIO;
	}

	LOG_INF("Booted the %s, lifecycle %s.", booted_image_str(report.booted_image),
						lifecycle_str(report.lifecycle));

	log_version("IROT", report.irot_version);
	log_version("UROT", report.urot_version);
	log_version("Application", report.app_version);
	log_version("IronSide DFU", report.dfu_version);

	/* NONE whenever the application booted, which is the only case here. */
	if (report.dfu_reason != IRONSIDE_DFU_REASON_NONE) {
		LOG_ERR("IronSide DFU was chosen because %s.", dfu_reason_str(report.dfu_reason));
		return -EIO;
	}

	if (report.update_applied != 0U) {
		LOG_ERR("Update candidate applied this boot at 0x%08x.",
			(unsigned int)report.update_target);
		return -EIO;
	}

	return 0;
}

static int random_bytes(uint8_t *out, size_t len)
{
	psa_status_t status;

	status = psa_generate_random(out, len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_generate_random failed (%d)", status);
		return -EIO;
	}

	LOG_HEXDUMP_INF(out, len, "Random bytes:");

	return 0;
}

static int attest(const uint8_t *challenge, size_t challenge_size)
{
	static uint8_t token[PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE];
	size_t expected_size;
	size_t token_size;
	psa_status_t status;

	status = psa_initial_attest_get_token_size(challenge_size, &expected_size);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_initial_attest_get_token_size() failed (%d).", status);
		return -EIO;
	}

	status = psa_initial_attest_get_token(challenge, challenge_size, token, sizeof(token),
					      &token_size);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_initial_attest_get_token() failed (%d).", status);
		return -EIO;
	}

	if (token_size != expected_size) {
		LOG_ERR("Token is %zu bytes, but %zu were announced.", token_size, expected_size);
		return -EIO;
	}

	LOG_INF("Attestation token: %zu bytes.", token_size);

	return 0;
}

int main(void)
{
	int err;
	uint8_t challenge[CHALLENGE_SIZE];

	err = boot_report();
	if (err != 0) {
		return err;
	}

	err = random_bytes(challenge, sizeof(challenge));
	if (err != 0) {
		return err;
	}

	err = attest(challenge, sizeof(challenge));
	if (err != 0) {
		return err;
	}

	LOG_INF("All IronSide TE services responded.");

	return 0;
}
