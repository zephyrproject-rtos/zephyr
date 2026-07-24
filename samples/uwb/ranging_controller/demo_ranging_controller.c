/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/uwb/uwb.h>
#include "demo_ranging_config.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define DEMO_RANGING_CONTROLLER_TASK_SIZE 4800
#define DEMO_RANGING_CONTROLLER_TASK_NAME "DemoRngCtrlr"
#define DEMO_RANGING_CONTROLLER_TASK_PRIO 4

/** Ranging APP configuration setting here */
#define DEMO_RANGING_APP_SESSION_ID 0x11223344

static uint8_t g_device_mac_address[] = {0x22, 0x11};
static uint8_t g_destination_mac_address[] = {0x11, 0x22};
static struct k_sem g_inband_termination_semaphore;

/********************************************************************************/

void demo_ntf_callback_handler(const uint8_t *const packet, const uint32_t len)
{
	if ((NULL == packet) || (len <= UCI_HEADER_SIZE)) {
		return;
	}

	const uint8_t *pData = packet + UCI_HEADER_SIZE;

	const uint16_t opType = (uint16_t)(((uint16_t)packet[0] << 8) | (uint16_t)packet[1]);

	if (kGidOid_SessionStatus == opType) {
		if (UWB_SESSION_STOPPED_DUE_TO_INBAND_SIGNAL == pData[5]) {
			k_sem_give(&g_inband_termination_semaphore);
		}
	} else {
		uwb_ntf_callback_handler(packet, len);
	}
}

void StandaloneTask(void *args)
{
	const uint32_t session_id = DEMO_RANGING_APP_SESSION_ID;
	const k_timeout_t delay = Z_TIMEOUT_MS(5 * 60 * 1000);

	k_sem_init(&g_inband_termination_semaphore, 0, 1);

	uwb_device_info_t device_info = {0};
	int ret = uwb_uci_register_callback(demo_ntf_callback_handler);
	if (ret != 0) {
		LOG_ERR("Could not register callback");
		return;
	}

	uwb_status_code_t status = uwb_api_core_get_device_info(&device_info);
	uint32_t session_handle = 0;
	status = uwb_api_session_init(session_id, kUwb_SessionType_Ranging, &session_handle);
	if (kUwb_StatusCode_Success != status) {
		LOG_ERR("Could not initialize session");
		goto exit;
	}

	uwb_config_t configs[] = {
		{kUwb_AppConfig_NumberOfControlees, 1, ARR(1), 0},
		{kUwb_AppConfig_DstMacAddress, sizeof(g_destination_mac_address),
		 g_destination_mac_address, 0},
		{kUwb_AppConfig_DeviceRole, 1, ARR(kUwb_DeviceRole_Initiator), 0},
		{kUwb_AppConfig_MultiNodeMode, 1, ARR(kUwb_MultiNodeMode_UniCast), 0},
		{kUwb_AppConfig_MacAddressMode, 1, ARR(kUWB_MacAddressMode_2bytes), 0},
		{kUwb_AppConfig_ScheduleMode, 1, ARR(kUwb_ScheduledMode_TimeScheduled), 0},
		{kUwb_AppConfig_DeviceMacAddress, sizeof(g_device_mac_address),
		 g_device_mac_address, 0},
		{kUwb_AppConfig_RangingRoundUsage, 1, ARR(kUwb_RangingRoundUsage_DS_TWR), 0},
		{kUwb_AppConfig_DeviceType, 1, ARR(kUwb_DeviceType_Controller), 0},
	};
	status = uwb_api_set_app_configs(session_handle, configs,
					 sizeof(configs) / sizeof(configs[0]));
	if (kUwb_StatusCode_Success != status) {
		LOG_ERR("Could not configure application configurations");
		goto exit;
	}

#ifdef DEMO_APP_CONFIGS
	status = uwb_api_set_app_configs(session_handle, DEMO_APP_CONFIGS,
					 sizeof(DEMO_APP_CONFIGS) / sizeof(DEMO_APP_CONFIGS[0]));
	if (kUwb_StatusCode_Success != status) {
		LOG_ERR("Could not configure application configs");
		goto exit;
	}
#endif /** DEMO_APP_CONFIGS */

	status = uwb_api_session_start(session_handle);
	if (kUwb_StatusCode_Success != status) {
		LOG_ERR("Could not start ranging");
		goto exit;
	}

	/* When Ranging is terminated due to inband termination this semaphore will
	 * be signaled, otherwise ranging will be performed for the time specified */
	if (0 == k_sem_take(&g_inband_termination_semaphore, delay)) {
		status = kUwb_StatusCode_Success;
		LOG_INF("\n-------------------------------------------\n in band termination "
			"is done  \n-------------------------------------------\n");
		uwb_api_session_deinit(session_handle);
		goto exit;
	}
	status = uwb_api_session_stop(session_handle);
	if (status != kUwb_StatusCode_Success) {
		LOG_ERR("uwb_api_session_stop failed");
		goto exit;
	}

	status = uwb_api_session_deinit(session_handle);
	if (status != kUwb_StatusCode_Success) {
		LOG_ERR("uwb_api_session_deinit failed");
		goto exit;
	}

exit:
	if (status == kUwb_StatusCode_Success) {
		LOG_INF("Success!");
	} else {
		LOG_ERR("Failed!");
	}
}

const k_tid_t taskHandle;
K_THREAD_DEFINE(taskHandle, DEMO_RANGING_CONTROLLER_TASK_SIZE, StandaloneTask, NULL, NULL, NULL,
		K_PRIO_PREEMPT(DEMO_RANGING_CONTROLLER_TASK_PRIO), 0, 0);
