/*
 * Copyright 2024 NXP.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * UWB API test suite – exercises uwb_api_* functions backed by the
 * native_sim transport (uwb_transport_* implemented in
 * zephyr/drivers/uwb/uwb_transport_native_sim.c).
 */

#include <zephyr/ztest.h>
#include <zephyr/uwb/uwb.h>
#include <zephyr/uwb/tml.h>
#include <zephyr/uwb/status.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test_uwb, LOG_LEVEL_DBG);

/* -------------------------------------------------------------------------
 * Test fixture – init/deinit the full stack around every test
 * -----------------------------------------------------------------------*/

static void *uwb_suite_setup(void)
{
	return NULL;
}

static void uwb_suite_teardown(void *data)
{
	ARG_UNUSED(data);
}

ZTEST_SUITE(uwb_api_suite, NULL, uwb_suite_setup, NULL, NULL, uwb_suite_teardown);

/* -------------------------------------------------------------------------
 * Test: device reset
 * -----------------------------------------------------------------------*/

ZTEST(uwb_api_suite, test_device_reset)
{
	uwb_status_code_t status = uwb_api_core_device_reset(0);

	zassert_equal(status, kUwb_StatusCode_Success, "uwb_api_core_reset failed: 0x%x", status);
}

/* -------------------------------------------------------------------------
 * Test: get device info
 * -----------------------------------------------------------------------*/

ZTEST(uwb_api_suite, test_get_device_info)
{
	uwb_device_info_t info = {0};
	uwb_status_code_t status = uwb_api_core_get_device_info(&info);

	zassert_equal(status, kUwb_StatusCode_Success, "uwb_api_get_device_info failed: 0x%x",
		      status);

	/* The native_sim backend reports version 1.0 for all version fields */
	zassert_equal(info.uciVersionMajor, 3, "unexpected UCI major version: %u",
		      info.uciVersionMajor);
	zassert_equal(info.uciVersionMinor, 0, "unexpected UCI minor version: %u",
		      info.uciVersionMinor);
	zassert_equal(info.macVersionMajor, 3, "unexpected MAC major version: %u",
		      info.macVersionMajor);
	zassert_equal(info.macVersionMinor, 0, "unexpected MAC major version: %u",
		      info.macVersionMinor);
	zassert_equal(info.phyVersionMajor, 3, "unexpected PHY major version: %u",
		      info.phyVersionMajor);
	zassert_equal(info.phyVersionMinor, 0, "unexpected PHY major version: %u",
		      info.phyVersionMinor);

	LOG_INF("Device info: UCI %u.%u  MAC %u.%u  PHY %u.%u", info.uciVersionMajor,
		info.uciVersionMinor, info.macVersionMajor, info.macVersionMinor,
		info.phyVersionMajor, info.phyVersionMinor);
}

ZTEST(uwb_api_suite, test_get_device_capabilities)
{
	uwb_dev_caps_t capabilities = {0};
	uwb_status_code_t status = uwb_api_core_get_caps_info(&capabilities);

	zassert_equal(status, kUwb_StatusCode_Success, "uwb_api_get_device_info failed: 0x%x",
		      status);

	LOG_INF("Supported firaPhyVersionRange.highRange = %u.%u",
		capabilities.firaPhyVersionRange.versions.highMajor,
		capabilities.firaPhyVersionRange.versions.highMinor);
	LOG_INF("Supported firaPhyVersionRange.lowRange = %u.%u",
		capabilities.firaPhyVersionRange.versions.lowMajor,
		capabilities.firaPhyVersionRange.versions.lowMinor);
	LOG_INF("Supported firaMacVersionRange.highRange = %u.%u",
		capabilities.firaMacVersionRange.versions.highMajor,
		capabilities.firaMacVersionRange.versions.highMinor);
	LOG_INF("Supported firaMacVersionRange.lowRange = %u.%u",
		capabilities.firaMacVersionRange.versions.lowMajor,
		capabilities.firaMacVersionRange.versions.lowMinor);
}

ZTEST(uwb_api_suite, test_get_device_state)
{
	uint8_t device_state = kUci_DeviceState_NA;
	uwb_status_code_t status = uwb_api_get_device_state(&device_state);

	zassert_equal(status, kUwb_StatusCode_Success, "uwb_api_get_device_info failed: 0x%x",
		      status);

	zassert_equal(device_state, kUci_DeviceState_Ready,
		      "Device state not ready without any active sessions");
}

ZTEST(uwb_api_suite, test_session_state)
{
	uint32_t session_id = 0xDEADBEEF;
	uint32_t session_handle = 0;

	uwb_status_code_t status =
		uwb_api_session_init(session_id, kUwb_SessionType_Ranging, &session_handle);
	zassert_equal(status, kUwb_StatusCode_Success, "uwb_api_session_init failed: 0x%x", status);

	uint8_t session_state = kUwb_SessionStatus_Error;
	status = uwb_api_session_get_state(session_handle, &session_state);
	zassert_equal(status, kUwb_StatusCode_Success, "uwb_api_session_get_state failed: 0x%x",
		      status);
	zassert_equal(session_state, kUwb_SessionStatus_Initialized, "session state not init: 0x%x",
		      status);

	status = uwb_api_session_deinit(session_handle);
	zassert_equal(status, kUwb_StatusCode_Success, "uwb_api_session_deinit failed: 0x%x",
		      status);

	status = uwb_api_session_get_state(session_handle, &session_state);
	zassert_equal(status, kUci_Status_ErrorSessionNotExist,
		      "uwb_api_session_get_state failed: 0x%x", status);
}
