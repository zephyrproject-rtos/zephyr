/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/uwb/uwb.h>
#include "demo_ranging_config.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define DEMO_RANGING_CONTROLEE_TASK_SIZE 4800
#define DEMO_RANGING_CONTROLEE_TASK_NAME "DemoRngCtrlee"
#define DEMO_RANGING_CONTROLEE_TASK_PRIO 4

/** Ranging APP configuration setting here */
#define DEMO_RANGING_APP_SESSION_ID 0x11223344

static uint8_t g_device_mac_address[] = {0x11, 0x22};
static uint8_t g_destination_mac_address[] = {0x22, 0x11};
static struct k_sem g_inband_termination_semaphore;

/********************************************************************************/

void demo_ntf_callback_handler(const uint8_t *const packet, const uint32_t len)
{
	if ((NULL == packet) || (len <= UCI_HEADER_SIZE)) {
		return;
	}

	const uint8_t *pData = packet + UCI_HEADER_SIZE;

	const uint16_t opType = (uint16_t)(((uint16_t)packet[0] << 8) | (uint16_t)packet[1]);

	if (UCI_CONTROL_GID_OID_SESSION_STATUS == opType) {
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

	status = uwb_api_session_init(session_id, UWB_SESSION_TYPE_RANGING, &session_handle);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not initialize session");
		goto exit;
	}

	uwb_config_t configs[] = {
		{UWB_APP_CONFIG_NUMBER_OF_CONTROLEES, 1, ARR(1), 0},
		{UWB_APP_CONFIG_DST_MAC_ADDRESS, sizeof(g_destination_mac_address),
		 g_destination_mac_address, 0},
		{UWB_APP_CONFIG_DEVICE_ROLE, 1, ARR(UWB_DEVICE_ROLE_RESPONDER), 0},
		{UWB_APP_CONFIG_MULTI_NODE_MODE, 1, ARR(UWB_MULTI_NODE_MODE_UNICAST), 0},
		{UWB_APP_CONFIG_MAC_ADDRESS_MODE, 1, ARR(UWB_MAC_ADDR_MODE_2BYTES), 0},
		{UWB_APP_CONFIG_SCHEDULE_MODE, 1, ARR(UWB_SCHEDULE_MODE_TIME_SCHEDULED), 0},
		{UWB_APP_CONFIG_DEVICE_MAC_ADDRESS, sizeof(g_device_mac_address),
		 g_device_mac_address, 0},
		{UWB_APP_CONFIG_RANGING_ROUND_USAGE, 1, ARR(UWB_RANGING_ROUND_USAGE_DS_TWR), 0},
		{UWB_APP_CONFIG_DEVICE_TYPE, 1, ARR(UWB_DEVICE_TYPE_CONTROLEE), 0},
	};

	status = uwb_api_set_app_configs(session_handle, configs, ARRAY_SIZE(configs));
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not configure application configurations");
		goto exit;
	}

#ifdef DEMO_APP_CONFIGS
	status = uwb_api_set_app_configs(session_handle, DEMO_APP_CONFIGS,
					 ARRAY_SIZE(DEMO_APP_CONFIGS));
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not configure application configs");
		goto exit;
	}
#endif /* DEMO_APP_CONFIGS */

	status = uwb_api_session_start(session_handle);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not start ranging");
		goto exit;
	}

	/* When Ranging is terminated due to inband termination this semaphore will
	 * be signaled, otherwise ranging will be performed for the time specified
	 */
	if (0 == k_sem_take(&g_inband_termination_semaphore, delay)) {
		status = UWB_STATUS_CODE_SUCCESS;
		LOG_INF("In-band termination done");
		uwb_api_session_deinit(session_handle);
		goto exit;
	}
	status = uwb_api_session_stop(session_handle);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("uwb_api_session_stop failed");
		goto exit;
	}

	status = uwb_api_session_deinit(session_handle);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("uwb_api_session_deinit failed");
		goto exit;
	}

exit:
	if (status == UWB_STATUS_CODE_SUCCESS) {
		LOG_INF("Success!");
	} else {
		LOG_ERR("Failed!");
	}
}

const k_tid_t taskHandle;
K_THREAD_DEFINE(taskHandle, DEMO_RANGING_CONTROLEE_TASK_SIZE, StandaloneTask, NULL, NULL, NULL,
		K_PRIO_PREEMPT(DEMO_RANGING_CONTROLEE_TASK_PRIO), 0, 0);
