/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/uwb/uwb.h>
#include <zephyr/uwb/uci.h>
#include <zephyr/uwb/tml.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uwb_fira, CONFIG_UWB_LOG_LEVEL);

typedef struct {
	struct k_sem ntf_semaphore;
	bool isInitialized;
} uwb_api_context_t;

static uwb_api_context_t g_uwb_context = {0};

#if CONFIG_UWB_INITIALIZATION_POST_KERNEL
int uwb_init(void)
{
	if (uwb_api_initialize(uwb_ntf_callback_handler) != UWB_STATUS_CODE_SUCCESS) {
		return -1;
	}
	return 0;
}

/* Priority 999 is the last to run in POST_KERNEL. We don't actually
 * care when it runs, so long as it's before APPLICATION, when
 * uwb_api_* APIs can be called. Running it last will allow more urgent
 * initializations competing for CPU time to complete first.
 */
SYS_INIT(uwb_init, POST_KERNEL, 999);
#endif /* CONFIG_UWB_INITIALIZATION_POST_KERNEL */

uwb_status_code_t uwb_api_initialize(uwb_uci_callback_t *p_application_callback)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;

	LOG_DBG("%s: Enter", __func__);

	if (!g_uwb_context.isInitialized) {
		/** Initialize semaphore */
		if (0 != k_sem_init(&g_uwb_context.ntf_semaphore, 0, 1)) {
			return UWB_STATUS_CODE_FAILED;
		}
		g_uwb_context.isInitialized = true;
	}

	uwb_tml_init();

	int ret = uwb_uci_init();

	if (ret) {
		LOG_ERR("UCI Initialize failed");
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	ret = uwb_ntf_handler_start();
	if (ret) {
		LOG_ERR("Could not initialize notification handler");
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	ret = uwb_uci_register_callback(p_application_callback);
	if (ret) {
		LOG_ERR("UCI Register callback failed");
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	ret = uwb_vendor_initialize();
	if (ret) {
		status = UWB_STATUS_CODE_VENDOR_FAILED;
		LOG_ERR("uwb_vendor_initialize failed");
		goto exit;
	}

	if (UWB_STATUS_CODE_SUCCESS != uwb_tml_read()) {
		status = UWB_STATUS_CODE_FAILED;
		LOG_ERR("Could not start reader thread");
		goto exit;
	}

	status = UWB_STATUS_CODE_SUCCESS;

exit:
	if (status != UWB_STATUS_CODE_SUCCESS) {
		uwb_api_deinitialize();
	}
	LOG_DBG("%s: exit ", __func__);
	return status;
}

uwb_status_code_t uwb_api_deinitialize(void)
{
	LOG_DBG("%s: Enter", __func__);
	uwb_vendor_deinitialize();
	uwb_uci_deinit();
	uwb_ntf_handler_stop();
	uwb_tml_deinit();
	return UWB_STATUS_CODE_SUCCESS;
}

int __weak uwb_vendor_initialize(void)
{
	return 0;
}

void __weak uwb_vendor_deinitialize(void)
{
}

uwb_status_code_t uwb_api_core_device_reset(uint8_t reset_config)
{
	uwb_status_code_t status = UWB_STATUS_CODE_FAILED;
	const k_timeout_t uwb_ntf_timeout = Z_TIMEOUT_MS(UWB_NTF_TIMEOUT);

	LOG_DBG("%s: enter", __func__);

	uint8_t device_reset_ntf[10] = {0};
	uint32_t device_reset_ntf_len = sizeof(device_reset_ntf);

	void *scheduled_ntf = uwb_uci_schedule_packet_read(
		UCI_MT_NTF, UCI_GID_CORE, UCI_MSG_CORE_DEVICE_STATUS_NTF, device_reset_ntf,
		&device_reset_ntf_len, &g_uwb_context.ntf_semaphore);
	if (!scheduled_ntf) {
		LOG_ERR("Could not schedule packet read");
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return status;
	}

	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);
	int ret = uwb_uci_transceive_control_packet(UCI_GID_CORE, UCI_MSG_CORE_DEVICE_RESET,
						    &reset_config, 1, response, &response_len);
	if (ret) {
		status = UWB_STATUS_CODE_FAILED;
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		uwb_uci_remove_scheduled_packet(scheduled_ntf);
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return status;
	}

	if (k_sem_take(&g_uwb_context.ntf_semaphore, uwb_ntf_timeout) != 0) {
		LOG_ERR("Did not receive notification in time");
		status = UWB_STATUS_CODE_FAILED;
	} else {
		status = UWB_STATUS_CODE_SUCCESS;
		if (device_reset_ntf[UCI_HEADER_SIZE] != UWB_DEVICE_STATE_READY) {
			LOG_ERR("Device not in ready state after reset");
			status = UWB_STATUS_CODE_FAILED;
		}
	}
	uwb_uci_remove_scheduled_packet(scheduled_ntf);
	k_sem_reset(&g_uwb_context.ntf_semaphore);
	return status;
}

uwb_status_code_t uwb_api_core_get_device_info(uwb_device_info_t *p_dev_info)
{
	uwb_status_code_t status = UWB_STATUS_CODE_FAILED;

	LOG_DBG("%s: enter; ", __func__);

	if (!p_dev_info) {
		LOG_ERR("%s: p_dev_info is NULL", __func__);
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}

	uint8_t response[UCI_MAX_CTRL_PACKET_SIZE] = {0};
	uint32_t response_len = sizeof(response);
	int ret = uwb_uci_transceive_control_packet(UCI_GID_CORE, UCI_MSG_CORE_DEVICE_INFO, NULL, 0,
						    response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		return status;
	}

	status = response[UCI_HEADER_SIZE];
	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("%s failed", __func__);
		return status;
	}

	uint8_t *rspPtr = &response[UCI_HEADER_SIZE + 1];

	if (uwb_parse_generic_device_info(p_dev_info, rspPtr, response_len - UCI_HEADER_SIZE - 1) ==
	    false) {
		LOG_ERR("%s: Parsing Device Information Failed", __func__);
		status = UWB_STATUS_CODE_FAILED;
	} else {
		status = UWB_STATUS_CODE_SUCCESS;
	}

	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_core_get_caps_info(uwb_dev_caps_t *p_dev_cap)
{
	uwb_status_code_t status = UWB_STATUS_CODE_FAILED;

	LOG_DBG("%s: enter", __func__);

	if (!p_dev_cap) {
		LOG_ERR("%s: p_dev_cap is NULL", __func__);
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}

	uint8_t response[UCI_MAX_CTRL_PACKET_PAYLOAD_SIZE + UCI_HEADER_SIZE] = {0};
	uint32_t response_len = sizeof(response);

	int ret = uwb_uci_transceive_control_packet(UCI_GID_CORE, UCI_MSG_CORE_GET_CAPS_INFO, NULL,
						    0, response, &response_len);

	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		return status;
	}

	status = response[UCI_HEADER_SIZE];
	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("Get Capabilities Info command failed with status: 0x%02X", status);
		return status;
	}

	status = uwb_parse_generic_capability_info(p_dev_cap, &response[UCI_HEADER_SIZE + 1],
						   (uint16_t)(response_len - UCI_HEADER_SIZE - 1));
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("%s: Parsing Capability Information Failed", __func__);
	}

	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_core_set_config(uwb_config_t *const configs, const uint8_t num_configs)
{
	uwb_status_code_t status = UWB_STATUS_CODE_FAILED;

	LOG_DBG("%s: enter", __func__);

	if ((!configs) || (num_configs == 0)) {
		LOG_ERR("%s: Parameter value is NULL", __func__);
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}

	uint16_t payload_length = uwb_calculate_configs_command_buffer_length(configs, num_configs);

	if (payload_length == 0) {
		LOG_ERR("Could not calculate payload length");
		return UWB_STATUS_CODE_FAILED;
	}
	payload_length++; /* Number of params */
	uint32_t response_length = UCI_HEADER_SIZE + uwb_calculate_configs_response_buffer_length(
							     configs, num_configs);

	uint8_t *payload = (uint8_t *)k_malloc(payload_length);
	uint8_t *response = (uint8_t *)k_malloc(response_length);

	if ((!payload) || (!response)) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	payload[0] = num_configs;

	status = uwb_serialize_set_config_payload(configs, num_configs, payload, &payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		goto exit;
	}

	int ret = uwb_uci_transceive_control_packet(UCI_GID_CORE, UCI_MSG_CORE_SET_CONFIG, payload,
						    payload_length, response, &response_length);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}
	status = response[UCI_HEADER_SIZE];

	if (response_length > (UCI_HEADER_SIZE + 1)) {
		/* Parse response and update error codes in configs structure */
		uwb_parse_config_response(&response[UCI_HEADER_SIZE + 1],
					  response_length - UCI_HEADER_SIZE - 1, configs,
					  num_configs);
	}

exit:
	if (payload) {
		k_free(payload);
	}
	if (response) {
		k_free(response);
	}
	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_core_get_config(uwb_config_t *const configs, const uint8_t num_configs)
{
	uwb_status_code_t status = UWB_STATUS_CODE_FAILED;

	LOG_DBG("%s: enter", __func__);

	if ((!configs) || (num_configs == 0)) {
		LOG_ERR("%s: param_value is NULL", __func__);
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}

	uint16_t payload_length =
		uwb_calculate_get_config_command_buffer_length(configs, num_configs);
	if (payload_length == 0) {
		LOG_ERR("Could not calculate payload length");
		return UWB_STATUS_CODE_FAILED;
	}
	payload_length++; /* Number of params */

	uint8_t response[UCI_MAX_CTRL_PACKET_PAYLOAD_SIZE + UCI_HEADER_SIZE] = {0};
	uint32_t response_len = sizeof(response);

	uint8_t *payload = (uint8_t *)k_malloc(payload_length);

	if (!payload) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	status = serialize_get_config_payload(configs, num_configs, payload, &payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		goto exit;
	}

	int ret = uwb_uci_transceive_control_packet(UCI_GID_CORE, UCI_MSG_CORE_GET_CONFIG, payload,
						    payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	status = response[UCI_HEADER_SIZE];

	if (status == UCI_STATUS_CODE_OK) {
		uwb_parse_get_config_response(&response[UCI_HEADER_SIZE + 1], response_len, configs,
					      num_configs);
	}

exit:
	if (payload) {
		k_free(payload);
	}
	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_core_query_uwbs_timestamp(uint8_t *const p_timestamp_value,
						    const uint8_t len)
{
	uwb_status_code_t status = UWB_STATUS_CODE_FAILED;

	LOG_DBG("%s: Enter", __func__);

	if ((len < 8) || (!p_timestamp_value)) {
		LOG_ERR("%s: Invalid arguments", __func__);
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}

	uint8_t response[20] = {0};
	uint32_t response_len = sizeof(response);
	int ret = uwb_uci_transceive_control_packet(UCI_GID_CORE, UCI_MSG_CORE_QUERY_UWBS_TIMESTAMP,
						    NULL, 0, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		return status;
	}

	status = response[UCI_HEADER_SIZE];
	if (status == UCI_STATUS_CODE_OK) {
		LOG_DBG("%s: Query timestamp cmd successful", __func__);
		/* rsp_data contains complete rsp, we have to skip Header */
		uint8_t *rspPtr = &response[UCI_HEADER_SIZE + 1];

		if (response_len >
		    (UCI_MSG_CORE_UWBS_TIMESTAMP_LEN + UCI_RESPONSE_PAYLOAD_OFFSET)) {
			LOG_ERR("%s: Response data size is more than response buffer", __func__);
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		uint32_t index = 0;

		UWB_STREAM_TO_ARRAY(p_timestamp_value, rspPtr, UCI_MSG_CORE_UWBS_TIMESTAMP_LEN,
				    index);
	}

	LOG_DBG("%s: Exit", __func__);
	return status;
}

uwb_status_code_t uwb_api_get_device_state(uint8_t *const p_device_state)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;
	uint8_t configParam = UWB_CORE_CONFIG_DEVICE_STATE;

	LOG_DBG("%s: Enter", __func__);

	if (!p_device_state) {
		LOG_ERR("%s: Invalid argument", __func__);
		return status;
	}

	uint8_t payload[1 /* number of params */ + sizeof(configParam)] = {0};
	uint16_t payload_length = uwb_serialize_get_core_config_payload(1, sizeof(configParam),
									&configParam, payload);

	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);
	int ret = uwb_uci_transceive_control_packet(UCI_GID_CORE, UCI_MSG_CORE_GET_CONFIG, payload,
						    payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		return UWB_STATUS_CODE_FAILED;
	}

	status = response[UCI_HEADER_SIZE];
	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("Get UWB Device state failed");
		return status;
	}

	*p_device_state = response[UCI_HEADER_SIZE + 1];

	return status;
}

uwb_status_code_t uwb_api_session_init(const uint32_t session_id,
				       const uwb_session_type_t session_type,
				       uint32_t *const p_session_handle)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;
	const k_timeout_t uwb_ntf_timeout = Z_TIMEOUT_MS(UWB_NTF_TIMEOUT);

	LOG_DBG("%s: enter", __func__);
	if (session_type == UWB_SESSION_TYPE_DEVICE_TEST) {
		if (session_id != 0x00) {
			return status;
		}
	}
	if (!p_session_handle) {
		LOG_ERR("%s: SessionHandle is NULL", __func__);
		return status;
	}

	uint8_t payload[sizeof(session_id) + sizeof(uint8_t)] = {0};
	uint16_t payload_length =
		uwb_serialize_session_init_payload(session_id, session_type, payload);

	uint8_t session_status_ntf[10] = {0};
	uint32_t session_status_ntf_len = sizeof(session_status_ntf);

	void *scheduled_ntf = uwb_uci_schedule_packet_read(
		UCI_MT_NTF, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_STATUS_NTF, session_status_ntf,
		&session_status_ntf_len, &g_uwb_context.ntf_semaphore);
	if (!scheduled_ntf) {
		LOG_ERR("Could not schedule packet read");
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return UWB_STATUS_CODE_FAILED;
	}

	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);
	int ret =
		uwb_uci_transceive_control_packet(UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_INIT,
						  payload, payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		uwb_uci_remove_scheduled_packet(scheduled_ntf);
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return UWB_STATUS_CODE_FAILED;
	}

	status = response[UCI_HEADER_SIZE];
	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("%s :  failed", __func__);
		uwb_uci_remove_scheduled_packet(scheduled_ntf);
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return status;
	}
	if (response_len == 1) {
		/* Backward compatibility for old Fira spec where devices don't return session
		 * handle
		 */
		*p_session_handle = session_id;
	} else {
		uint8_t *response_ptr = &response[UCI_HEADER_SIZE + 1];
		uint8_t index = 0;

		UWB_STREAM_TO_UINT32(*p_session_handle, response_ptr, index);
	}

	do {
		if (k_sem_take(&g_uwb_context.ntf_semaphore, uwb_ntf_timeout) != 0) {
			LOG_ERR("Did not receive notification in time");
			status = UWB_STATUS_CODE_FAILED;
		} else {
			status = UWB_STATUS_CODE_SUCCESS;

			uint32_t ntf_session_handle = 0;
			uint8_t *notification_ptr = &session_status_ntf[UCI_HEADER_SIZE];
			uint8_t index = 0;

			UWB_STREAM_TO_UINT32(ntf_session_handle, notification_ptr, index);
			uint8_t session_state = session_status_ntf[UCI_HEADER_SIZE + index];

			if (ntf_session_handle != *p_session_handle) {
				LOG_DBG("Session handle mismatch in notification");
				status = UWB_STATUS_CODE_FAILED;
				continue;
			} else if (session_state != UWB_SESSION_STATUS_INITIALIZED) {
				LOG_ERR("Session not in initialized state after init");
				status = UWB_STATUS_CODE_FAILED;
			}
		}
		break;
	} while (true);
	uwb_uci_remove_scheduled_packet(scheduled_ntf);
	k_sem_reset(&g_uwb_context.ntf_semaphore);
	return status;
}

uwb_status_code_t uwb_api_session_deinit(const uint32_t session_handle)
{
	LOG_DBG("%s: enter", __func__);
	const k_timeout_t uwb_ntf_timeout = Z_TIMEOUT_MS(UWB_NTF_TIMEOUT);

	uint8_t payload[sizeof(session_handle)] = {0};
	uint16_t payload_length = 0;

	UWB_UINT32_TO_STREAM(payload, session_handle, payload_length);

	uint8_t session_status_ntf[10] = {0};
	uint32_t session_status_ntf_len = sizeof(session_status_ntf);

	void *scheduled_ntf = uwb_uci_schedule_packet_read(
		UCI_MT_NTF, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_STATUS_NTF, session_status_ntf,
		&session_status_ntf_len, &g_uwb_context.ntf_semaphore);
	if (!scheduled_ntf) {
		LOG_ERR("Could not schedule packet read");
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return UWB_STATUS_CODE_FAILED;
	}

	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);
	int ret =
		uwb_uci_transceive_control_packet(UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_DEINIT,
						  payload, payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		uwb_uci_remove_scheduled_packet(scheduled_ntf);
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return UWB_STATUS_CODE_FAILED;
	}

	uwb_status_code_t status = response[UCI_HEADER_SIZE];

	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("%s :  failed", __func__);
		uwb_uci_remove_scheduled_packet(scheduled_ntf);
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return status;
	}

	do {
		if (k_sem_take(&g_uwb_context.ntf_semaphore, uwb_ntf_timeout) != 0) {
			LOG_ERR("Did not receive notification in time");
			status = UWB_STATUS_CODE_FAILED;
		} else {
			status = UWB_STATUS_CODE_SUCCESS;

			uint32_t ntf_session_handle = 0;
			uint8_t session_state = 0;
			uint8_t index = UCI_HEADER_SIZE;

			UWB_STREAM_TO_UINT32(ntf_session_handle, session_status_ntf, index);
			UWB_STREAM_TO_UINT8(session_state, session_status_ntf, index);

			if (ntf_session_handle != session_handle) {
				LOG_DBG("Session handle mismatch in notification");
				status = UWB_STATUS_CODE_FAILED;
				continue;
			} else if (session_state != UWB_SESSION_STATUS_DE_INITIALIZED) {
				LOG_ERR("Session not in deinitialized state after deinit");
				status = UWB_STATUS_CODE_FAILED;
			}
		}
		break;
	} while (true);
	uwb_uci_remove_scheduled_packet(scheduled_ntf);
	k_sem_reset(&g_uwb_context.ntf_semaphore);

	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_set_app_configs(const uint32_t session_handle,
					  uwb_config_t *const configs, const uint8_t num_configs)
{
	uwb_status_code_t status = UWB_STATUS_CODE_FAILED;

	LOG_DBG("%s: enter", __func__);

	if ((!configs) || (num_configs == 0)) {
		LOG_ERR("%s: configs is NULL or num_configs is 0", __func__);
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}

	uint16_t payload_length = uwb_calculate_configs_command_buffer_length(configs, num_configs);

	if (payload_length) {
		LOG_ERR("Could not calculate payload length");
		return UWB_STATUS_CODE_FAILED;
	}

	/* Add session handle (4 bytes) and number of params (1 byte) */
	payload_length += sizeof(uint32_t) + 1;
	uint32_t response_length = UCI_HEADER_SIZE + uwb_calculate_configs_response_buffer_length(
							     configs, num_configs);
	uint8_t *payload = (uint8_t *)k_malloc(payload_length);
	uint8_t *response = (uint8_t *)k_malloc(response_length);

	if ((!payload) || (!response)) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	/* Serialize session handle */
	uint32_t index = 0;

	UWB_UINT32_TO_STREAM(payload, session_handle, index);

	uint16_t config_payload_len = payload_length - index;

	/* Serialize configs using the unified serialization function */
	status = uwb_serialize_set_config_payload(configs, num_configs, &payload[index],
						  &config_payload_len);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("%s: uwb_serialize_set_config_payload failed", __func__);
		goto exit;
	}

	int ret = uwb_uci_transceive_control_packet(UCI_GID_SESSION_MANAGE,
						    UCI_MSG_SESSION_SET_APP_CONFIG, payload,
						    payload_length, response, &response_length);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	status = response[UCI_HEADER_SIZE];

	if (response_length > (UCI_HEADER_SIZE + 1)) {
		/* Parse response and update error codes in configs structure */
		uwb_parse_config_response(&response[UCI_HEADER_SIZE + 1],
					  response_length - UCI_HEADER_SIZE - 1, configs,
					  num_configs);
	}

exit:
	if (payload) {
		k_free(payload);
	}
	if (response) {
		k_free(response);
	}
	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_get_app_configs(const uint32_t session_handle,
					  uwb_config_t *const configs, const uint8_t num_configs)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;

	LOG_DBG("%s: enter", __func__);

	if ((!configs) || (num_configs == 0)) {
		LOG_ERR("%s: param_value is NULL", __func__);
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}

	uint16_t payload_length =
		uwb_calculate_get_config_command_buffer_length(configs, num_configs);
	if (payload_length == 0) {
		LOG_ERR("Could not calculate payload length");
		return UWB_STATUS_CODE_FAILED;
	}
	payload_length = payload_length + UCI_SESSION_HANDLE_LENGTH +
			 sizeof(uint8_t); /* Session handle + number of params */

	uint8_t response[UCI_MAX_CTRL_PACKET_PAYLOAD_SIZE + UCI_HEADER_SIZE] = {0};
	uint32_t response_len = sizeof(response);
	uint8_t *payload = (uint8_t *)k_malloc(payload_length);

	if (!payload) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	/* Serialize session handle */
	uint32_t index = 0;

	UWB_UINT32_TO_STREAM(payload, session_handle, index);
	uint16_t config_payload_len = payload_length - index;

	/* Serialize configs using the unified serialization function */
	status = serialize_get_config_payload(configs, num_configs, &payload[index],
					      &config_payload_len);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		goto exit;
	}

	int ret = uwb_uci_transceive_control_packet(UCI_GID_SESSION_MANAGE,
						    UCI_MSG_SESSION_GET_APP_CONFIG, payload,
						    payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	status = response[UCI_HEADER_SIZE];

	if (status == UCI_STATUS_CODE_OK) {
		uwb_parse_get_config_response(&response[UCI_HEADER_SIZE + 1], response_len, configs,
					      num_configs);
	}

exit:
	if (payload) {
		k_free(payload);
	}
	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_session_get_count(uint8_t *const p_session_count)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;
	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);

	LOG_DBG("%s: enter", __func__);

	if (!p_session_count) {
		LOG_ERR("%s: Invalid arguments passed", __func__);
		return status;
	}

	int ret =
		uwb_uci_transceive_control_packet(UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_GET_COUNT,
						  NULL, 0, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		return UWB_STATUS_CODE_FAILED;
	}

	/* Reset session state to default value */
	status = response[UCI_HEADER_SIZE];
	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("%s :  failed", __func__);
		return status;
	}
	*p_session_count = response[UCI_HEADER_SIZE + 1];

	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_session_get_state(const uint32_t session_handle, uint8_t *p_session_state)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;
	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);
	uint8_t payload[sizeof(session_handle)] = {0};

	LOG_DBG("%s: enter", __func__);

	if (!p_session_state) {
		LOG_ERR("%s: Parameter p_session_state value is NULL", __func__);
		return status;
	}

	uint16_t payload_length = 0;

	UWB_UINT32_TO_STREAM(payload, session_handle, payload_length);
	int ret =
		uwb_uci_transceive_control_packet(UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_GET_STATE,
						  payload, payload_length, response, &response_len);

	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		return UWB_STATUS_CODE_FAILED;
	}

	/* Reset session state to default value */
	*p_session_state = 0xFF;
	status = response[UCI_HEADER_SIZE];
	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("%s :  failed", __func__);
		return status;
	}

	*p_session_state = response[UCI_HEADER_SIZE + 1];

	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_update_controller_multicast_list(
	const uint32_t session_handle, const uwb_multicast_controller_actions_t action,
	uwb_multicast_controlee_list_context_t *const p_controlee_list,
	const uint8_t num_controlees)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;

	LOG_DBG("%s: enter", __func__);

	if (!p_controlee_list) {
		LOG_DBG("%s: Invalid argument", __func__);
		return status;
	}

	if (action == UWB_MULTICAST_CONTROLLER_ACTION_DEL_CONTROLEE) {
		for (uint8_t i = 0; i < num_controlees; i++) {
			if (p_controlee_list[i].subsession_id != 0x0) {
				/* for deletion, sub Session Handle must be zero */
				return status;
			}
		}
	}

	uint16_t payload_length = uwb_calculate_update_controller_multicast_list_payload_length(
		action, num_controlees);
	if (payload_length == 0) {
		return status;
	}
	uint32_t response_length =
		UCI_HEADER_SIZE +
		uwb_calculate_update_controller_multicast_list_response_length(num_controlees);

	uint8_t *payload = (uint8_t *)k_malloc(payload_length);
	uint8_t *response = (uint8_t *)k_malloc(response_length);

	if ((!payload) || (!response)) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	status = uwb_serialize_update_controller_multicast_list_payload(
		session_handle, action, p_controlee_list, num_controlees, payload, &payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		goto exit;
	}

	int ret = uwb_uci_transceive_control_packet(
		UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST, payload,
		payload_length, response, &response_length);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	status = response[UCI_HEADER_SIZE];
	if (status != UCI_STATUS_CODE_OK) {
		LOG_DBG("%s: SESSION_MC_LIST_UPDATE command returned 0x%x ", __func__, status);
		/*
		 * Whether the action is to Add or Delete the controlees from the list, if the
		 * response code is STATUS_FAILURE, there would be response data with the controlee
		 * MAC addresses and respective status for the operations. that needs to be reported
		 * to the caller.
		 */
		uwb_deserialize_update_controller_multicast_list_resp(
			p_controlee_list, num_controlees, &response[UCI_HEADER_SIZE + 1],
			response_length - UCI_HEADER_SIZE - 1);
	}

exit:
	if (payload) {
		k_free(payload);
	}
	if (response) {
		k_free(response);
	}
	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t
uwb_api_update_dt_anchor_ranging_round(const uint32_t session_handle, const uint8_t n_active_rounds,
				       const uwb_mac_addr_mode_t mac_addressing_mode,
				       const uwb_active_rounds_config_t *const round_config_list,
				       uint8_t *const p_failed_rounds,
				       uint8_t *const p_num_failed_rounds)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;

	LOG_DBG("%s: enter", __func__);

	if ((n_active_rounds == 0) || (!round_config_list)) {
		LOG_ERR("Invalid arguments passed");
		LOG_DBG("%s: n_active_rounds is %d, round_config_list is 0x%p", __func__,
			n_active_rounds, round_config_list);
		return status;
	}

	uint16_t payload_length = uwb_calculate_dt_anchor_ranging_round_command_length(
		n_active_rounds, mac_addressing_mode, round_config_list);
	if (payload_length == 0) {
		LOG_ERR("Could not calculate payload length");
		return status;
	}

	uint32_t response_length =
		UCI_HEADER_SIZE + 1 /* Status */ + 1 /* Number of rounds */ + n_active_rounds;

	uint8_t *payload = (uint8_t *)k_malloc(payload_length);
	uint8_t *response = (uint8_t *)k_malloc(response_length);

	if ((!payload) || (!response)) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	status = uwb_serialize_update_active_rounds_anchor_payload(
		session_handle, n_active_rounds, mac_addressing_mode, round_config_list, payload,
		&payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not serialize DT-Anchor ranging rounds payload");
		goto exit;
	}

	int ret = uwb_uci_transceive_control_packet(
		UCI_GID_SESSION_MANAGE, UCI_MSG_UPDATE_ACTIVE_ROUNDS_OF_ANCHOR_DEVICE, payload,
		payload_length, response, &response_length);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	status = response[UCI_HEADER_SIZE];
	if (status != UCI_STATUS_CODE_OK) {
		uint8_t offset = UCI_HEADER_SIZE + 1;
		uint8_t num_failed_rounds = 0;

		UWB_STREAM_TO_UINT8(num_failed_rounds, response, offset);
		if (p_num_failed_rounds) {
			num_failed_rounds = (num_failed_rounds < *p_num_failed_rounds)
						    ? num_failed_rounds
						    : *p_num_failed_rounds;
			*p_num_failed_rounds = num_failed_rounds;
			if (p_failed_rounds) {
				for (uint8_t failed_round = 0; failed_round < num_failed_rounds;
				     failed_round++) {
					UWB_STREAM_TO_UINT8(p_failed_rounds[failed_round], response,
							    offset);
				}
			}
		}
	}

exit:
	if (payload) {
		k_free(payload);
	}
	if (response) {
		k_free(response);
	}
	LOG_DBG("%s: Exit", __func__);
	return status;
}

uwb_status_code_t uwb_api_update_dt_tag_ranging_round(const uint32_t session_handle,
						      const uint8_t n_active_rounds,
						      const uint8_t *const p_ranging_round_indices,
						      uint8_t *const p_failed_rounds,
						      uint8_t *const p_num_failed_rounds)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;

	LOG_DBG("%s: enter", __func__);

	if ((!n_active_rounds) || (!p_ranging_round_indices)) {
		LOG_ERR("Invalid arguments");
		LOG_DBG("%s: n_active_rounds is %d, p_ranging_round_indices is 0x%p", __func__,
			n_active_rounds, p_ranging_round_indices);
		return status;
	}

	uint16_t payload_length =
		uwb_calculate_update_active_rounds_receiver_command_length(n_active_rounds);
	uint32_t response_length = UCI_HEADER_SIZE + 1 /* Status */ +
				   1 /* Number of ranging rounds */ + n_active_rounds;

	uint8_t *payload = (uint8_t *)k_malloc(payload_length);
	uint8_t *response = (uint8_t *)k_malloc(response_length);

	if ((!payload) || (!response)) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	status = uwb_serialize_update_tag_active_rounds_payload(
		session_handle, n_active_rounds, p_ranging_round_indices, payload, &payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		goto exit;
	}

	int ret = uwb_uci_transceive_control_packet(
		UCI_GID_SESSION_MANAGE, UCI_MSG_UPDATE_ACTIVE_ROUNDS_OF_RECEIVER_DEVICE, payload,
		payload_length, response, &response_length);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	status = response[UCI_HEADER_SIZE];
	uint8_t offset = UCI_HEADER_SIZE + 1;
	uint8_t num_failed_rounds = 0;

	UWB_STREAM_TO_UINT8(num_failed_rounds, response, offset);
	if (p_num_failed_rounds) {
		num_failed_rounds = (num_failed_rounds < *p_num_failed_rounds)
					    ? num_failed_rounds
					    : *p_num_failed_rounds;
		*p_num_failed_rounds = num_failed_rounds;
		if (p_failed_rounds) {
			for (uint8_t failed_round = 0; failed_round < num_failed_rounds;
			     failed_round++) {
				UWB_STREAM_TO_UINT8(p_failed_rounds[failed_round], response,
						    offset);
			}
		}
	}

exit:
	if (payload) {
		k_free(payload);
	}
	if (response) {
		k_free(response);
	}
	LOG_DBG("%s: Exit", __func__);
	return status;
}

uwb_status_code_t uwb_api_query_data_size_in_ranging(const uint32_t connection_identifier,
						     uint16_t *const p_size)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;
	uint8_t response[UCI_HEADER_SIZE + sizeof(uint32_t) /* SessionID */ +
			 sizeof(uint8_t) /* Status */ + sizeof(uint16_t) /* dataSize */] = {0};
	uint32_t response_len = sizeof(response);

	LOG_DBG("%s: Enter", __func__);

	if (!p_size) {
		LOG_ERR("%s: Invalid arguments", __func__);
		return status;
	}

	uint8_t payload[sizeof(connection_identifier)] = {0};
	uint8_t payload_length = 0;

	UWB_UINT32_TO_STREAM(payload, connection_identifier, payload_length);

	int ret = uwb_uci_transceive_control_packet(
		UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_QUERY_DATA_SIZE_IN_RANGING, payload,
		payload_length, response, &response_len);

	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		return UWB_STATUS_CODE_FAILED;
	}

	uint32_t connection_id_rsp = 0;
	uint32_t response_offset = UCI_HEADER_SIZE;

	UWB_STREAM_TO_UINT32(connection_id_rsp, response, response_offset);
	if (connection_id_rsp != connection_identifier) {
		LOG_ERR("Invalid connection identifier received in response");
		return UWB_STATUS_CODE_FAILED;
	}
	UWB_STREAM_TO_UINT8(status, response, response_offset);
	if (status != UCI_STATUS_CODE_OK) {
		*p_size = 0;
	} else {
		UWB_STREAM_TO_UINT16((*p_size), response, response_offset);
	}

	LOG_DBG("%s: Exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_set_controller_hus_session(
	const uint32_t session_handle, const uint8_t num_phases,
	const uwb_hus_controller_secondary_session_config_t *const p_hus_secondary_session_config)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;

	LOG_DBG("%s: Enter", __func__);

	if ((!p_hus_secondary_session_config) && (num_phases != 0)) {
		LOG_ERR("%s: Invalid arguments", __func__);
		return status;
	}

	uint16_t payload_length = uwb_calculate_controller_hus_session_payload_length(
		p_hus_secondary_session_config, num_phases);

	if (payload_length == 0) {
		LOG_ERR("Could not calculate payload length");
		return status;
	}

	uint8_t *payload = (uint8_t *)k_malloc(payload_length);

	if (!payload) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);

	status = uwb_serialize_controller_hus_session_payload(session_handle, num_phases,
							      p_hus_secondary_session_config,
							      payload, &payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not serialize payload");
		goto exit;
	}

	int ret = uwb_uci_transceive_control_packet(
		UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_SET_HUS_CONTROLLER_CONFIG_CMD, payload,
		payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	status = response[UCI_HEADER_SIZE];

exit:
	if (payload) {
		k_free(payload);
	}
	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t
uwb_api_set_controlee_hus_session(const uint32_t session_handle, const uint8_t num_phases,
				  const uint32_t *const p_hus_secondary_session_handles)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;

	LOG_DBG("%s: Enter", __func__);

	if ((!p_hus_secondary_session_handles) && (num_phases != 0)) {
		LOG_ERR("%s: Invalid arguments", __func__);
		return status;
	}

	uint16_t payload_length =
		uwb_calculate_set_controlee_hus_session_payload_length(num_phases);

	if (payload_length == 0) {
		LOG_ERR("Could not calculate payload length");
		return status;
	}

	uint8_t *payload = (uint8_t *)k_malloc(payload_length);

	if (!payload) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);

	status = uwb_serialize_controlee_hus_session_payload(session_handle, num_phases,
							     p_hus_secondary_session_handles,
							     payload, &payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not serialize buffer");
		goto exit;
	}

	int ret = uwb_uci_transceive_control_packet(
		UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_SET_HUS_CONTROLEE_CONFIG_CMD, payload,
		payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	status = response[UCI_HEADER_SIZE];

exit:
	if (payload) {
		k_free(payload);
	}
	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_dtpcm_config(const uint32_t session_handle,
				       const uint8_t dtpcm_repetition,
				       const uint8_t data_transfer_control,
				       const uwb_data_tx_phase_mng_list_t *const dtpml,
				       const uint8_t dtpml_size)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;

	LOG_DBG("%s: Enter", __func__);

	uint16_t payload_length =
		uwb_calculate_dtpcm_payload_length(data_transfer_control, dtpml_size);

	if (payload_length == 0) {
		LOG_ERR("Could not calculate payload length");
		return status;
	}

	uint8_t *payload = (uint8_t *)k_malloc(payload_length);

	if (!payload) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);

	status =
		uwb_serialize_dtpcm_payload(session_handle, dtpcm_repetition, data_transfer_control,
					    dtpml, dtpml_size, payload, &payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not serialize payload");
		goto exit;
	}

	int ret = uwb_uci_transceive_control_packet(
		UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_DATA_TRANSFER_PHASE_CONFIG, payload,
		payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	status = response[UCI_HEADER_SIZE];

exit:
	if (payload) {
		k_free(payload);
	}
	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_session_start(const uint32_t session_handle)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;
	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);
	uint8_t payload[sizeof(session_handle)] = {0};
	const k_timeout_t uwb_ntf_timeout = Z_TIMEOUT_MS(UWB_NTF_TIMEOUT);
	uint16_t payload_length = 0;

	LOG_DBG("%s: enter", __func__);
	UWB_UINT32_TO_STREAM(payload, session_handle, payload_length);

	uint8_t session_status_ntf[10] = {0};
	uint32_t session_status_ntf_len = sizeof(session_status_ntf);

	void *scheduled_ntf = uwb_uci_schedule_packet_read(
		UCI_MT_NTF, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_STATUS_NTF, session_status_ntf,
		&session_status_ntf_len, &g_uwb_context.ntf_semaphore);
	if (!scheduled_ntf) {
		LOG_ERR("Could not schedule packet read");
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return UWB_STATUS_CODE_FAILED;
	}

	int ret =
		uwb_uci_transceive_control_packet(UCI_GID_RANGE_MANAGE, UCI_MSG_RANGE_START,
						  payload, payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		uwb_uci_remove_scheduled_packet(scheduled_ntf);
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return UWB_STATUS_CODE_FAILED;
	}

	status = response[UCI_HEADER_SIZE];
	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("%s :  failed", __func__);
		uwb_uci_remove_scheduled_packet(scheduled_ntf);
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return status;
	}

	do {
		if (k_sem_take(&g_uwb_context.ntf_semaphore, uwb_ntf_timeout) != 0) {
			LOG_ERR("Did not receive notification in time");
			status = UWB_STATUS_CODE_FAILED;
		} else {
			status = UWB_STATUS_CODE_SUCCESS;

			uint8_t session_state = 0xFF;
			uint32_t ntf_session_handle = 0;
			uint32_t index = UCI_HEADER_SIZE;

			UWB_STREAM_TO_UINT32(ntf_session_handle, session_status_ntf, index);
			UWB_STREAM_TO_UINT8(session_state, session_status_ntf, index);

			if (ntf_session_handle != session_handle) {
				LOG_DBG("Session handle mismatch in notification");
				status = UWB_STATUS_CODE_FAILED;
				continue;
			} else if (session_state != UWB_SESSION_STATUS_ACTIVE) {
				LOG_ERR("Session not in active state after start ranging");
				status = UWB_STATUS_CODE_FAILED;
			}
		}
		break;
	} while (true);
	uwb_uci_remove_scheduled_packet(scheduled_ntf);
	k_sem_reset(&g_uwb_context.ntf_semaphore);

	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_session_stop(const uint32_t session_handle)
{
	uwb_status_code_t status = UWB_STATUS_CODE_FAILED;
	uint8_t payload[sizeof(session_handle)] = {0};
	uint16_t payload_length = 0;
	const k_timeout_t uwb_ntf_timeout = Z_TIMEOUT_MS(UWB_NTF_TIMEOUT);

	LOG_DBG("%s: enter", __func__);
	UWB_UINT32_TO_STREAM(payload, session_handle, payload_length);

	uint8_t session_status_ntf[10] = {0};
	uint32_t session_status_ntf_len = sizeof(session_status_ntf);

	void *scheduled_ntf = uwb_uci_schedule_packet_read(
		UCI_MT_NTF, UCI_GID_SESSION_MANAGE, UCI_MSG_SESSION_STATUS_NTF, session_status_ntf,
		&session_status_ntf_len, &g_uwb_context.ntf_semaphore);
	if (!scheduled_ntf) {
		LOG_ERR("Could not schedule packet read");
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return UWB_STATUS_CODE_FAILED;
	}

	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);
	int ret =
		uwb_uci_transceive_control_packet(UCI_GID_RANGE_MANAGE, UCI_MSG_RANGE_STOP, payload,
						  payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		uwb_uci_remove_scheduled_packet(scheduled_ntf);
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return UWB_STATUS_CODE_FAILED;
	}

	status = response[UCI_HEADER_SIZE];
	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("%s :  failed", __func__);
		uwb_uci_remove_scheduled_packet(scheduled_ntf);
		k_sem_reset(&g_uwb_context.ntf_semaphore);
		return status;
	}

	do {
		if (k_sem_take(&g_uwb_context.ntf_semaphore, uwb_ntf_timeout) != 0) {
			LOG_ERR("Did not receive notification in time");
			status = UWB_STATUS_CODE_FAILED;
		} else {
			status = UWB_STATUS_CODE_SUCCESS;

			uint8_t session_state = 0xFF;
			uint32_t ntf_session_handle = 0;
			uint32_t index = UCI_HEADER_SIZE;

			UWB_STREAM_TO_UINT32(ntf_session_handle, session_status_ntf, index);
			UWB_STREAM_TO_UINT8(session_state, session_status_ntf, index);

			if (ntf_session_handle != session_handle) {
				LOG_DBG("Session handle mismatch in notification");
				status = UWB_STATUS_CODE_FAILED;
				continue;
			} else if (session_state != UWB_SESSION_STATUS_IDLE) {
				LOG_ERR("Session not in initialized state after init");
				status = UWB_STATUS_CODE_FAILED;
			}
		}
		break;
	} while (true);
	uwb_uci_remove_scheduled_packet(scheduled_ntf);
	k_sem_reset(&g_uwb_context.ntf_semaphore);

	LOG_DBG("%s: exit status %d", __func__, status);
	return status;
}

uwb_status_code_t uwb_api_session_get_ranging_count(const uint32_t session_handle,
						    uint32_t *const p_ranging_count)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;
	uint8_t response[10] = {0};
	uint32_t response_len = sizeof(response);
	uint8_t payload[sizeof(session_handle)] = {0};
	uint16_t payload_length = 0;

	LOG_DBG("%s: enter", __func__);

	if (!p_ranging_count) {
		LOG_ERR("%s: p_ranging_count is NULL", __func__);
		return status;
	}

	/* Serialize session handle into payload */
	UWB_UINT32_TO_STREAM(payload, session_handle, payload_length);

	/* Send SESSION_GET_RANGING_COUNT command */
	int ret = uwb_uci_transceive_control_packet(UCI_GID_SESSION_MANAGE,
						    UCI_MSG_RANGE_GET_RANGING_COUNT, payload,
						    payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		return UWB_STATUS_CODE_FAILED;
	}

	/* Parse response */
	uint32_t offset = UCI_HEADER_SIZE;

	/* Get status */
	UWB_STREAM_TO_UINT8(status, response, offset);

	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("%s: Get ranging count failed with status 0x%02X", __func__, status);
		return status;
	}

	/* Extract session handle from response */
	UWB_STREAM_TO_UINT32((*p_ranging_count), response, offset);

	LOG_DBG("%s: exit status %d, ranging count: %u", __func__, status, *p_ranging_count);
	return status;
}

uwb_status_code_t
uwb_api_logical_link_create(const uint32_t session_handle,
			    const uwb_link_layer_mode_selector_t link_layer_mode_selector,
			    const uint8_t *const destination_address,
			    const uint8_t logical_link_class, const uint8_t max_sdu_size_length,
			    const uint8_t max_sdu_size_value,
			    uint32_t *const p_logical_link_connect_id)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;
	uint8_t response[20] = {0};
	uint32_t response_len = sizeof(response);

	LOG_DBG("%s: Enter", __func__);

	if ((!destination_address) || (!p_logical_link_connect_id) || (logical_link_class != 0)) {
		LOG_ERR("%s: Invalid input parameters", __func__);
		return status;
	}

	uint16_t payload_length = uwb_calculate_logical_link_create_payload_length();
	uint8_t *payload = (uint8_t *)k_malloc(payload_length);

	if (!payload) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	/* Serialize logical link create command payload */
	status = uwb_serialize_create_logical_link_cmd(
		session_handle, link_layer_mode_selector, destination_address, logical_link_class,
		max_sdu_size_length, max_sdu_size_value, payload, &payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not serialize payload");
		goto exit;
	}

	int ret =
		uwb_uci_transceive_control_packet(UCI_GID_RANGE_MANAGE, UCI_MSG_LOGICAL_LINK_CREATE,
						  payload, payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	uint8_t offset = UCI_HEADER_SIZE;

	UWB_STREAM_TO_UINT8(status, response, offset);
	if (status != UCI_STATUS_CODE_OK) {
		goto exit;
	}
	UWB_STREAM_TO_UINT32((*p_logical_link_connect_id), response, offset);

exit:
	if (payload) {
		k_free(payload);
	}
	LOG_DBG("%s: Exit", __func__);
	return status;
}

uwb_status_code_t uwb_api_logical_link_close(const uint32_t logical_link_connect_id)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;
	uint8_t response[20] = {0};
	uint32_t response_len = sizeof(response);
	uint8_t payload[sizeof(logical_link_connect_id)] = {0};
	uint16_t payload_length = 0;

	LOG_DBG("%s: Enter", __func__);

	/* Serialize logical link id into payload */
	UWB_UINT32_TO_STREAM(payload, logical_link_connect_id, payload_length);

	int ret =
		uwb_uci_transceive_control_packet(UCI_GID_RANGE_MANAGE, UCI_MSG_LOGICAL_LINK_CLOSE,
						  payload, payload_length, response, &response_len);
	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		return UWB_STATUS_CODE_FAILED;
	}

	status = response[UCI_HEADER_SIZE];

	LOG_DBG("%s: Exit", __func__);
	return status;
}

uwb_status_code_t uwb_api_logical_link_get_param(
	const uint32_t connection_identifier,
	uwb_logical_link_get_params_rsp_t *const p_logical_link_get_params_rsp)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;
	uint8_t response[50] = {0};
	uint32_t response_len = sizeof(response);
	uint8_t payload[sizeof(connection_identifier)] = {0};
	uint16_t payload_length = 0;

	LOG_DBG("%s: Enter", __func__);

	if (!p_logical_link_get_params_rsp) {
		LOG_ERR("%s: Invalid input parameters", __func__);
		return status;
	}

	/* Serialize logical link connection id into payload */
	UWB_UINT32_TO_STREAM(payload, connection_identifier, payload_length);

	int ret = uwb_uci_transceive_control_packet(UCI_GID_RANGE_MANAGE,
						    UCI_MSG_LOGICAL_LINK_GET_PARAM, payload,
						    payload_length, response, &response_len);

	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		return UWB_STATUS_CODE_FAILED;
	}

	uint8_t offset = UCI_HEADER_SIZE;

	UWB_STREAM_TO_UINT8(status, response, offset);

	if (status != UCI_STATUS_CODE_OK) {
		LOG_ERR("%s :  failed", __func__);
		return status;
	}

	if (response_len > (UCI_HEADER_SIZE + 1)) {
		uint8_t *rspPtr = &response[offset];

		status = uwb_deserialize_link_get_params_payload(rspPtr,
								 p_logical_link_get_params_rsp);
	} else {
		LOG_ERR("%s: Incorrect response length", __func__);
		status = UWB_STATUS_CODE_FAILED;
	}

	LOG_DBG("%s: Exit", __func__);
	return status;
}

uwb_status_code_t uwb_api_send_data(const uint32_t connection_identifier,
				    const uint8_t *const destination_address,
				    const uint16_t sequence_number, const uint8_t *const data,
				    const uint16_t data_size)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;

	LOG_DBG("%s: Enter", __func__);

	if ((!destination_address) || (!data) || (data_size == 0)) {
		LOG_ERR("%s: Invalid input parameters", __func__);
		return status;
	}

	uint16_t payload_length = uwb_calculate_send_data_payload_length(data_size);

	if (payload_length == 0) {
		LOG_ERR("Could not calculate payload length");
		return status;
	}

	uint8_t data_credit_ntf[UWB_DATA_CREDIT_NTF_MAX_LEN] = {0};
	uint32_t data_credit_ntf_length = UWB_DATA_CREDIT_NTF_MAX_LEN;
	uint8_t *payload = (uint8_t *)k_malloc(payload_length);

	if (!payload) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	status = uwb_serialize_send_data_payload(connection_identifier, destination_address,
						 sequence_number, data, data_size, payload,
						 &payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not serialize SendData payload");
		goto exit;
	}

	int ret = uwb_uci_transceive_data_packet(UCI_DPF_SEND, payload, payload_length,
						 data_credit_ntf, &data_credit_ntf_length);

	if (ret) {
		LOG_ERR("%s: Could not transceive UCI packet", __func__);
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	uint32_t received_connection_id = 0;
	uint8_t offset = UCI_HEADER_SIZE;

	UWB_STREAM_TO_UINT32(received_connection_id, data_credit_ntf, offset);
	if (received_connection_id != connection_identifier) {
		LOG_ERR("Received connection identifier mismatch");
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}
	uint8_t credit_available = data_credit_ntf[offset];

	if (credit_available != UWB_DATA_CREDIT_AVAILABLE) {
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

exit:
	if (payload) {
		k_free(payload);
	}
	LOG_DBG("%s: Exit", __func__);
	return status;
}

uwb_status_code_t uwb_api_logical_link_send_data(const uint32_t connection_identifier,
						 const uint16_t sequence_number,
						 const uint8_t *const data,
						 const uint16_t data_size)
{
	uwb_status_code_t status = UWB_STATUS_CODE_INVALID_ARGUMENT;

	LOG_DBG("%s: Enter", __func__);

	if ((!data) || (data_size == 0)) {
		LOG_ERR("%s: Invalid input parameters", __func__);
		return status;
	}

	uint16_t payload_length = uwb_calculate_ll_send_data_payload_length(data_size);
	uint8_t data_credit_ntf[UWB_DATA_CREDIT_NTF_MAX_LEN] = {0};
	uint32_t data_credit_ntf_length = UWB_DATA_CREDIT_NTF_MAX_LEN;
	uint8_t *payload = (uint8_t *)k_malloc(payload_length);

	if (!payload) {
		LOG_ERR("Failed to allocate memory");
		status = UWB_STATUS_CODE_NO_SPACE_LEFT;
		goto exit;
	}

	status = uwb_serialize_ll_send_data_payload(connection_identifier, sequence_number, data,
						    data_size, payload, &payload_length);
	if (status != UWB_STATUS_CODE_SUCCESS) {
		LOG_ERR("Could not serialize payload");
		goto exit;
	}

	int ret = uwb_uci_transceive_data_packet(UCI_DPF_LL_SEND, payload, payload_length,
						 data_credit_ntf, &data_credit_ntf_length);

	if (ret) {
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

	uint32_t received_connection_id = 0;
	uint8_t offset = UCI_HEADER_SIZE;

	UWB_STREAM_TO_UINT32(received_connection_id, data_credit_ntf, offset);
	if (received_connection_id != connection_identifier) {
		LOG_ERR("Received connection identifier mismatch");
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}
	uint8_t credit_available = data_credit_ntf[offset];

	if (credit_available != UWB_DATA_CREDIT_AVAILABLE) {
		status = UWB_STATUS_CODE_FAILED;
		goto exit;
	}

exit:
	if (payload) {
		k_free(payload);
	}
	LOG_DBG("%s: Exit", __func__);
	return status;
}
