/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/uwb/uwb.h>
#include <zephyr/uwb/types.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uwb_api_helper, CONFIG_UWB_LOG_LEVEL);

uint16_t uwb_calculate_configs_command_buffer_length(const uwb_config_t *const configs,
						     const uint8_t num_configs)
{
	uint16_t total_buffer_length = 0;

	for (uint8_t config_index = 0; config_index < num_configs; config_index++) {
		const uwb_config_t *const config = &configs[config_index];

		/* e.g. E000 01 01 => tag= E000 length = 01 value = 01*/
		/* Calculate length of tag */
		if (config->tag > UWB_CONFIG_EXTENDED_PARAMS_START) {
			/* Extended configs - tag length is 2 */
			total_buffer_length += 2;
		} else {
			total_buffer_length += 1;
		}

		/* Only one byte length */
		total_buffer_length++;

		/* Calculate length of payload */
		uint8_t payload_length = config->length;

		if (payload_length == 0) {
			/* No payload */
			continue;
		}
		if (!config->value) {
			/* NULL parameter passed */
			LOG_ERR("Invalid argument");
			return 0;
		}
		total_buffer_length += payload_length;
	}
	return total_buffer_length;
}

uint16_t uwb_calculate_configs_response_buffer_length(const uwb_config_t *const configs,
						      const uint8_t num_configs)
{
	uint16_t total_length = 0;

	for (uint8_t config_index = 0; config_index < num_configs; config_index++) {
		const uwb_config_t *const config = &configs[config_index];
		/* Calculate length of tag */
		if (config->tag > UWB_CONFIG_EXTENDED_PARAMS_START) {
			/* Extended configs - tag length is 2 */
			total_length += 2;
		} else {
			total_length += 1;
		}

		/* One byte of error code */
		total_length++;
	}
	return total_length;
}

uwb_status_code_t uwb_serialize_set_config_payload(const uwb_config_t *const configs,
						   const uint8_t num_configs,
						   uint8_t *const p_buffer, uint16_t *p_buffer_len)
{
	uint16_t buffer_size = *p_buffer_len;
	uint16_t serialized_length = 0;

	if ((serialized_length + sizeof(uint8_t)) > buffer_size) {
		/* Buffer overflow */
		LOG_ERR("%s: Buffer overflow", __func__);
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_buffer, num_configs, serialized_length);

	for (uint8_t config_index = 0; config_index < num_configs; config_index++) {
		const uwb_config_t *const config = &configs[config_index];

		if (config->tag > UWB_CONFIG_EXTENDED_PARAMS_START) {
			if ((serialized_length + sizeof(uint16_t)) > buffer_size) {
				/* Buffer overflow */
				LOG_ERR("%s: Buffer overflow", __func__);
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			UWB_UINT16_TO_BE_STREAM(p_buffer, config->tag, serialized_length);
		} else {
			if ((serialized_length + sizeof(uint8_t)) > buffer_size) {
				/* Buffer overflow */
				LOG_ERR("%s: Buffer overflow", __func__);
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			UWB_UINT8_TO_STREAM(p_buffer, config->tag, serialized_length);
		}
		if ((serialized_length + sizeof(uint8_t)) > buffer_size) {
			/* Buffer overflow */
			LOG_ERR("%s: Buffer overflow", __func__);
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		if (config->length == 0) {
			LOG_ERR("%s: Value of length is zero", __func__);
			return UWB_STATUS_CODE_INVALID_ARGUMENT;
		}

		UWB_UINT8_TO_STREAM(p_buffer, config->length, serialized_length);

		if (config->value == NULL) {
			LOG_ERR("%s: NULL value pointer with non-zero length", __func__);
			return UWB_STATUS_CODE_INVALID_ARGUMENT;
		}
		if ((serialized_length + config->length) > buffer_size) {
			/* Buffer overflow */
			LOG_ERR("%s: Buffer overflow", __func__);
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_ARRAY_TO_STREAM(p_buffer, config->value, config->length, serialized_length);
	}
	*p_buffer_len = serialized_length;
	return UWB_STATUS_CODE_SUCCESS;
}

static uwb_config_t *uwb_get_uwb_config_with_id(const uint16_t tag, uwb_config_t *const configs,
						const uint8_t num_configs)
{
	for (uint8_t config_index = 0; config_index < num_configs; config_index++) {
		if (tag == configs[config_index].tag) {
			/* Found matching entry */
			return &configs[config_index];
		}
	}
	return NULL;
}

void uwb_parse_config_response(const uint8_t *const p_buffer, const uint16_t buffer_length,
			       uwb_config_t *const configs, const uint8_t num_configs)
{
	uint16_t offset = 0;

	if (p_buffer == NULL || buffer_length == 0) {
		LOG_ERR("%s: Insufficient buffer ", __func__);
		return;
	}

	uint8_t received_num_configs = p_buffer[offset++];

	while (received_num_configs--) {
		/* Check if we can read at least the tag byte */
		if (offset >= buffer_length) {
			LOG_ERR("%s: Buffer overrun detected at tag read", __func__);
			return;
		}

		uint16_t tag = p_buffer[offset];
		/* Try finding Fira generic tag entry */
		uwb_config_t *entry = uwb_get_uwb_config_with_id(tag, configs, num_configs);

		if (entry) {
			/* Found entry matching the tag */
			offset++;
			if (offset >= buffer_length) {
				LOG_ERR("%s: Buffer overrun detected at tag read", __func__);
				return;
			}
			entry->status = p_buffer[offset++];
		} else {
			if (offset + 1 >= buffer_length) {
				LOG_ERR("%s: Buffer overrun detected at tag read", __func__);
				return;
			}
			/* Try with extended ID */
			tag = (tag << 8) | (p_buffer[offset + 1]);
			entry = uwb_get_uwb_config_with_id(tag, configs, num_configs);
			if (!entry) {
				LOG_ERR("Could not find a matching entry; aborting");
				return;
			}
			offset += 2;
			if (offset >= buffer_length) {
				LOG_ERR("%s: Buffer overrun detected at tag read", __func__);
				return;
			}
			entry->status = p_buffer[offset++];
		}
	}
}

uint16_t uwb_calculate_get_config_command_buffer_length(const uwb_config_t *const configs,
							const uint8_t num_configs)
{
	uint16_t total_length = 0;

	for (uint8_t config_index = 0; config_index < num_configs; config_index++) {
		const uwb_config_t *const config = &configs[config_index];

		/* Calculate length of tag */
		if (config->tag > UWB_CONFIG_EXTENDED_PARAMS_START) {
			/* Extended configs - tag length is 2 */
			total_length += 2;
		} else {
			total_length += 1;
		}
	}
	return total_length;
}

uwb_status_code_t serialize_get_config_payload(uwb_config_t *const configs,
					       const uint8_t num_configs, uint8_t *const p_buffer,
					       uint16_t *p_buffer_len)
{
	uint16_t buffer_size = *p_buffer_len;
	uint16_t serialized_length = 0;

	if ((serialized_length + sizeof(uint8_t)) > buffer_size) {
		/* Buffer overflow */
		LOG_ERR("Buffer overflow");
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_buffer, num_configs, serialized_length);

	for (uint8_t config_index = 0; config_index < num_configs; config_index++) {
		const uwb_config_t *const config = &configs[config_index];

		if (config->tag > UWB_CONFIG_EXTENDED_PARAMS_START) {
			if ((serialized_length + sizeof(uint16_t)) > buffer_size) {
				/* Buffer overflow */
				LOG_ERR("Buffer overflow");
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			UWB_UINT16_TO_BE_STREAM(p_buffer, config->tag, serialized_length);
		} else {
			if ((serialized_length + sizeof(uint8_t)) > buffer_size) {
				/* Buffer overflow */
				LOG_ERR("Buffer overflow");
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			UWB_UINT8_TO_STREAM(p_buffer, config->tag, serialized_length);
		}
	}
	*p_buffer_len = serialized_length;
	return UWB_STATUS_CODE_SUCCESS;
}

void uwb_parse_get_config_response(const uint8_t *const p_buffer, const uint16_t buffer_length,
				   uwb_config_t *const configs, const uint8_t num_configs)
{
	uint16_t offset = 0;

	if (p_buffer == NULL || buffer_length == 0) {
		LOG_ERR("%s: Insufficient buffer ", __func__);
		return;
	}

	uint8_t received_num_configs = p_buffer[offset++];

	while (received_num_configs--) {
		/* Check if we can read at least the tag byte */
		if (offset >= buffer_length) {
			LOG_ERR("%s: Buffer overrun detected at tag read", __func__);
			return;
		}

		uint16_t tag = p_buffer[offset];
		/* Try finding Fira generic tag entry */
		uwb_config_t *entry = uwb_get_uwb_config_with_id(tag, configs, num_configs);

		if (entry) {
			/* Found entry matching the tag */
			offset++;
			if (offset >= buffer_length) {
				LOG_ERR("%s: Buffer overrun detected at tag read", __func__);
				return;
			}
			uint16_t length = p_buffer[offset++];

			/* Check if we have enough data for the value */
			if ((offset + length) > buffer_length) {
				LOG_ERR("%s: Buffer overrun detected reading value (need %u, have "
					"%u)",
					__func__, offset + length, buffer_length);
				return;
			}

			if (!entry->value) {
				LOG_WRN("NULL buffer passed, skip parsing");
				offset += length;
				continue;
			}
			if (entry->length < length) {
				LOG_WRN("Insufficient buffer, skip parsing");
				offset += length;
				continue;
			}
			entry->length = length;
			UWB_STREAM_TO_ARRAY(entry->value, p_buffer, entry->length, offset);
		} else {
			if (offset + 1 >= buffer_length) {
				LOG_ERR("%s: Buffer overrun detected at tag read", __func__);
				return;
			}
			/* Try with extended ID */
			tag = (tag << 8) | (p_buffer[offset + 1]);
			entry = uwb_get_uwb_config_with_id(tag, configs, num_configs);
			if (!entry) {
				LOG_ERR("Could not find a matching entry; aborting");
				return;
			}
			offset += 2;
			if (offset >= buffer_length) {
				LOG_ERR("%s: Buffer overrun detected at tag read", __func__);
				return;
			}
			uint16_t length = p_buffer[offset++];

			/* Check if we have enough data for the value */
			if ((offset + length) > buffer_length) {
				LOG_ERR("%s: Buffer overrun detected reading extended value (need "
					"%u, have %u)",
					__func__, offset + length, buffer_length);
				return;
			}

			if (!entry->value) {
				LOG_WRN("NULL buffer passed, skip parsing");
				offset += length;
				continue;
			}
			if (entry->length < length) {
				LOG_WRN("Insufficient buffer, skip parsing");
				offset += length;
				continue;
			}
			entry->length = length;
			UWB_STREAM_TO_ARRAY(entry->value, p_buffer, entry->length, offset);
		}
	}
}

uint16_t uwb_serialize_get_core_config_payload(const uint8_t num_params, const uint8_t param_len,
					       const uint8_t *const param_id,
					       uint8_t *const p_cmd_buf)
{
	uint16_t offset = 0;

	if (!p_cmd_buf || (param_len > 0 && !param_id)) {
		LOG_ERR("%s: NULL pointer parameter", __func__);
		return 0;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, num_params, offset);
	UWB_ARRAY_TO_STREAM(p_cmd_buf, param_id, param_len, offset);
	return offset;
}

uint16_t uwb_serialize_session_init_payload(const uint32_t session_id,
					    const uwb_session_type_t session_type,
					    uint8_t *const p_cmd_buf)
{
	uint16_t offset = 0;

	if (!p_cmd_buf) {
		LOG_ERR("%s: NULL pointer parameter", __func__);
		return 0;
	}
	UWB_UINT32_TO_STREAM(p_cmd_buf, session_id, offset);
	UWB_UINT8_TO_STREAM(p_cmd_buf, session_type, offset);
	return offset;
}

bool uwb_parse_generic_device_info(uwb_device_info_t *const p_dev_info,
				   const uint8_t *const device_info_data,
				   const uint16_t device_info_length)
{
	uint16_t offset = 0;

	if (device_info_length == 0) {
		LOG_ERR("%s: device_info_length is zero", __func__);
		return false;
	}

	if (device_info_data == NULL) {
		LOG_ERR("%s: device_info_data is NULL", __func__);
		return false;
	}

	if (p_dev_info == NULL) {
		LOG_ERR("%s: p_dev_info is NULL", __func__);
		return false;
	}
	UWB_STREAM_TO_UINT8(p_dev_info->uciVersionMajor, device_info_data, offset);
	if (offset >= device_info_length) {
		return false;
	}
	UWB_STREAM_TO_UINT8(p_dev_info->uciVersionMinor, device_info_data, offset);
	if (offset >= device_info_length) {
		return false;
	}
	UWB_STREAM_TO_UINT8(p_dev_info->macVersionMajor, device_info_data, offset);
	if (offset >= device_info_length) {
		return false;
	}
	UWB_STREAM_TO_UINT8(p_dev_info->macVersionMinor, device_info_data, offset);
	if (offset >= device_info_length) {
		return false;
	}
	UWB_STREAM_TO_UINT8(p_dev_info->phyVersionMajor, device_info_data, offset);
	if (offset >= device_info_length) {
		return false;
	}
	UWB_STREAM_TO_UINT8(p_dev_info->phyVersionMinor, device_info_data, offset);
	if (offset >= device_info_length) {
		return false;
	}
	UWB_STREAM_TO_UINT8(p_dev_info->uciExtensionVersionMajor, device_info_data, offset);
	if (offset >= device_info_length) {
		return false;
	}
	UWB_STREAM_TO_UINT8(p_dev_info->uciExtensionVersionMinor, device_info_data, offset);
	if (offset >= device_info_length) {
		return false;
	}
	uint8_t available_vendor_data_size = p_dev_info->vendorSpecificLength;

	UWB_STREAM_TO_UINT8(p_dev_info->vendorSpecificLength, device_info_data, offset);
	if (available_vendor_data_size >= p_dev_info->vendorSpecificLength) {
		if (p_dev_info->vendorSpecificData) {
			if (offset + p_dev_info->vendorSpecificLength > device_info_length) {
				return false;
			}
			UWB_STREAM_TO_ARRAY(p_dev_info->vendorSpecificData, device_info_data,
					    p_dev_info->vendorSpecificLength, offset);
		}
	}
	return true;
}

uwb_status_code_t uwb_parse_generic_capability_info(uwb_dev_caps_t *const p_dev_cap,
						    const uint8_t *const caps_info_data,
						    const uint16_t caps_info_len)
{
	uwb_status_code_t status = UWB_STATUS_CODE_SUCCESS;
	uint16_t offset = 0;
	uint8_t received_num_configs = caps_info_data[offset++];

	if ((!p_dev_cap) || (caps_info_len == 0) || (!caps_info_data)) {
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}

	uint8_t *pRawTlvBuffer = p_dev_cap->extraCapsBuffer;
	const uint16_t rawTlvBufferLength = p_dev_cap->extraCapsLength;

	p_dev_cap->extraCapsLength = 0;

	while (received_num_configs--) {
		uint16_t tag = caps_info_data[offset++];
		/* Parse length */
		uint8_t length = caps_info_data[offset++];

		if ((length + offset) > caps_info_len) {
			LOG_ERR("%s: Invalid length for capability info : %d, %d, %d", __func__,
				length, offset, caps_info_len);
			status = UWB_STATUS_CODE_CORRUPTED;
			break;
		}

		switch (tag) {
		case UWB_CAPABILITY_PARAM_MAX_DATA_MESSAGE_SIZE: {
			if (length != sizeof(p_dev_cap->maxDataMessageSize)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT16(p_dev_cap->maxDataMessageSize, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_MAX_DATA_PACKET_PAYLOAD_SIZE: {
			if (length != sizeof(p_dev_cap->maxDataPacketPayloadSize)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT16(p_dev_cap->maxDataPacketPayloadSize, caps_info_data,
					     offset);
		} break;
		case UWB_CAPABILITY_PARAM_FIRA_PHY_VERSION_RANGE: {
			if (length != sizeof(p_dev_cap->firaPhyVersionRange)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			p_dev_cap->firaPhyVersionRange.versions.highMinor =
				caps_info_data[offset++];
			p_dev_cap->firaPhyVersionRange.versions.highMajor =
				caps_info_data[offset++];
			p_dev_cap->firaPhyVersionRange.versions.lowMinor = caps_info_data[offset++];
			p_dev_cap->firaPhyVersionRange.versions.lowMajor = caps_info_data[offset++];
		} break;
		case UWB_CAPABILITY_PARAM_FIRA_MAC_VERSION_RANGE: {
			if (length != sizeof(p_dev_cap->firaMacVersionRange)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			p_dev_cap->firaMacVersionRange.versions.highMinor =
				caps_info_data[offset++];
			p_dev_cap->firaMacVersionRange.versions.highMajor =
				caps_info_data[offset++];
			p_dev_cap->firaMacVersionRange.versions.lowMinor = caps_info_data[offset++];
			p_dev_cap->firaMacVersionRange.versions.lowMajor = caps_info_data[offset++];
		} break;
		case UWB_CAPABILITY_PARAM_DEVICE_TYPE: {
			if (length != sizeof(p_dev_cap->deviceType)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->deviceType, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_DEVICE_ROLES: {
			if (length != sizeof(p_dev_cap->deviceRoles)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT16(p_dev_cap->deviceRoles, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_RANGING_METHOD: {
			if (length != sizeof(p_dev_cap->rangingMethod)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT16(p_dev_cap->rangingMethod, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_STS_CONFIG: {
			if (length != sizeof(p_dev_cap->stsConfig)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->stsConfig, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_MULTI_NODE_MODE: {
			if (length != sizeof(p_dev_cap->multiNodeMode)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->multiNodeMode, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_RANGING_TIME_STRUCT: {
			if (length != sizeof(p_dev_cap->rangingTimeStruct)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->rangingTimeStruct, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_SCHEDULE_MODE: {
			if (length != sizeof(p_dev_cap->scheduleMode)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->scheduleMode, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_HOPPING_MODE: {
			if (length != sizeof(p_dev_cap->hoppingMode)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->hoppingMode, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_BLOCK_STRIDING: {
			if (length != sizeof(p_dev_cap->blockStriding)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->blockStriding, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_UWB_INITIATION_TIME: {
			if (length != sizeof(p_dev_cap->uwbInitiationTime)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->uwbInitiationTime, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_CHANNELS: {
			if (length != sizeof(p_dev_cap->channels)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->channels, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_RFRAME_CONFIG: {
			if (length != sizeof(p_dev_cap->rframeConfig)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->rframeConfig, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_CC_CONSTRAINT_LENGTH: {
			if (length != sizeof(p_dev_cap->ccConstraintLength)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->ccConstraintLength, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_BPRF_PARAMETER_SETS: {
			if (length != sizeof(p_dev_cap->bprfParameterSets)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->bprfParameterSets, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_HPRF_PARAMETER_SETS: {
			if (length != sizeof(p_dev_cap->hprfParameterSets)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_ARRAY(p_dev_cap->hprfParameterSets, caps_info_data,
					    sizeof(p_dev_cap->hprfParameterSets), offset);
		} break;
		case UWB_CAPABILITY_PARAM_AOA_SUPPORT: {
			if (length != sizeof(p_dev_cap->aoaSupport)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->aoaSupport, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_EXTENDED_MAC_ADDRESS: {
			if (length != sizeof(p_dev_cap->extendedMacAddress)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->extendedMacAddress, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_SESSION_KEY_LENGTH: {
			if (length != sizeof(p_dev_cap->sessionKeyLength)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->sessionKeyLength, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_DT_ANCHOR_MAX_ACTIVE_RR: {
			if (length != sizeof(p_dev_cap->dtAnchorMaxActiveRr)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->dtAnchorMaxActiveRr, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_DT_TAG_MAX_ACTIVE_RR: {
			if (length != sizeof(p_dev_cap->dtTagMaxActiveRr)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->dtTagMaxActiveRr, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_LL_CAPABILITY_PARAM: {
			if (length != sizeof(p_dev_cap->llCapabilityParam)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT16(p_dev_cap->llCapabilityParam, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_BYPASS_MODE_SUPPORT: {
			if (length != sizeof(p_dev_cap->bypassModeSupport)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->bypassModeSupport, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_MIN_SLOT_DURATION_SUPPORT: {
			if (length != sizeof(p_dev_cap->minSlotDurationSupport)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT16(p_dev_cap->minSlotDurationSupport, caps_info_data,
					     offset);
		} break;
		case UWB_CAPABILITY_PARAM_FIRA_LL_VERSION: {
			if (length != sizeof(p_dev_cap->firaLlVersion)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->firaLlVersion, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_DT_TAG_BLOCK_SKIPPING: {
			if (length != sizeof(p_dev_cap->dtTagBlockSkipping)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->dtTagBlockSkipping, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_PSDU_LENGTH_SUPPORT: {
			if (length != sizeof(p_dev_cap->psduLengthSupport)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->psduLengthSupport, caps_info_data, offset);
		} break;

		case UWB_CAPABILITY_PARAM_CCC_SLOT_BITMASK: {
			if (length != sizeof(p_dev_cap->ccc_slot_bitmask)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->ccc_slot_bitmask, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_CCC_SYNC_CODE_INDEX_BITMASK: {
			if (length != sizeof(p_dev_cap->ccc_sync_code_index_bitmask)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT32(p_dev_cap->ccc_sync_code_index_bitmask, caps_info_data,
					     offset);
		} break;
		case UWB_CAPABILITY_PARAM_CCC_HOPPING_CONFIG_BITMASK: {
			if (length != sizeof(p_dev_cap->ccc_hopping_config_bitmask)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->ccc_hopping_config_bitmask, caps_info_data,
					    offset);
		} break;
		case UWB_CAPABILITY_PARAM_CCC_CHANNEL_BITMASK: {
			if (length != sizeof(p_dev_cap->ccc_channel_bitmask)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->ccc_channel_bitmask, caps_info_data, offset);
		} break;
		case UWB_CAPABILITY_PARAM_CCC_SUPPORTED_PROTOCOL_VERSION: {
			p_dev_cap->num_ccc_supported_protocol_versions =
				length / sizeof(p_dev_cap->ccc_supported_protocol_versions[0]);
			if ((length == 0) ||
			    (p_dev_cap->num_ccc_supported_protocol_versions >
			     CONFIG_UWB_NUM_SUPPORTED_CCC_PROTOCOL_VERSIONS) ||
			    ((length % sizeof(p_dev_cap->ccc_supported_protocol_versions[0])) !=
			     0)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			for (uint8_t iter = 0;
			     iter < p_dev_cap->num_ccc_supported_protocol_versions; iter++) {
				UWB_STREAM_TO_UINT16(
					p_dev_cap->ccc_supported_protocol_versions[iter],
					caps_info_data, offset);
			}
		} break;
		case UWB_CAPABILITY_PARAM_CCC_SUPPORTED_UWB_CONFIG_ID: {
			p_dev_cap->num_ccc_supported_uwb_config_id =
				length / sizeof(p_dev_cap->ccc_supported_uwb_config_id[0]);
			if ((length == 0) ||
			    (p_dev_cap->num_ccc_supported_uwb_config_id >
			     CONFIG_UWB_NUM_SUPPORTED_CCC_UWB_CONFIG_ID) ||
			    ((length % sizeof(p_dev_cap->ccc_supported_uwb_config_id[0])) != 0)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			for (uint8_t iter = 0; iter < p_dev_cap->num_ccc_supported_uwb_config_id;
			     iter++) {
				UWB_STREAM_TO_UINT16(p_dev_cap->ccc_supported_uwb_config_id[iter],
						     caps_info_data, offset);
			}
		} break;
		case UWB_CAPABILITY_PARAM_CCC_SUPPORTED_PULSESHAPE_COMBO: {
			p_dev_cap->num_ccc_supported_pulseshape_combo =
				length / sizeof(p_dev_cap->ccc_supported_pulseshape_combo[0]);
			if ((length == 0) ||
			    (p_dev_cap->num_ccc_supported_pulseshape_combo >
			     CONFIG_UWB_NUM_SUPPORTED_CCC_PULSESHAPE_COMBO) ||
			    ((length % sizeof(p_dev_cap->ccc_supported_pulseshape_combo[0])) !=
			     0)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			for (uint8_t iter = 0; iter < p_dev_cap->num_ccc_supported_pulseshape_combo;
			     iter++) {
				UWB_STREAM_TO_UINT8(p_dev_cap->ccc_supported_pulseshape_combo[iter],
						    caps_info_data, offset);
			}
		} break;
		case UWB_CAPABILITY_PARAM_CCC_MINIMUM_RAN_MULTIPLIER: {
			if (length != sizeof(p_dev_cap->ccc_minimum_ran_multiplier)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->ccc_minimum_ran_multiplier, caps_info_data,
					    offset);
		} break;
		case UWB_CAPABILITY_PARAM_ALIRO_SUPPORTED_MAC_MODE: {
			if (length != sizeof(p_dev_cap->aliro_supported_mac_mode)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			UWB_STREAM_TO_UINT8(p_dev_cap->aliro_supported_mac_mode, caps_info_data,
					    offset);
		} break;
		case UWB_CAPABILITY_PARAM_ALIRO_SUPPORTED_PROTOCOL_VERSIONS: {
			p_dev_cap->num_aliro_supported_protocol_versions =
				length / sizeof(p_dev_cap->aliro_supported_protocol_versions[0]);
			if ((length == 0) ||
			    (p_dev_cap->num_aliro_supported_protocol_versions >
			     CONFIG_UWB_NUM_SUPPORTED_ALIRO_PROTOCOL_VERSIONS) ||
			    ((length % sizeof(p_dev_cap->aliro_supported_protocol_versions[0])) !=
			     0)) {
				LOG_ERR("Invalid length");
				status = UWB_STATUS_CODE_CORRUPTED;
				break;
			}
			for (uint8_t iter = 0;
			     iter < p_dev_cap->num_aliro_supported_protocol_versions; iter++) {
				UWB_BE_STREAM_TO_UINT16(
					p_dev_cap->aliro_supported_protocol_versions[iter],
					caps_info_data, offset);
			}
		} break;

		default: {
			/* Unknown tag - add to raw buffer */
			uint16_t extended_tag = (tag << 8) | (length);

			if ((extended_tag >= UWB_CONFIG_EXTENDED_PARAMS_START) &&
			    (extended_tag < UWB_CONFIG_EXTENDED_PARAMS_END)) {
				/* It is an extended tag */
				length = caps_info_data[offset++]; /* Parse length */
				if ((length + offset) > caps_info_len) {
					LOG_ERR("%s: Invalid length for extended capability info",
						__func__);
					status = UWB_STATUS_CODE_CORRUPTED;
					break;
				} else if ((p_dev_cap->extraCapsLength + length +
					    3 /* Tag + Length */) <= rawTlvBufferLength) {
					/* Insufficient memory */
					LOG_WRN("Insufficient buffer, skipping");
					offset += length;
				} else if (pRawTlvBuffer) {
					*pRawTlvBuffer++ = tag;                        /* Tag LSB */
					*pRawTlvBuffer++ = (extended_tag >> 8) & 0xFF; /* Tag MSB */
					*pRawTlvBuffer++ = length; /* Add length */
					UWB_STREAM_TO_ARRAY(pRawTlvBuffer, caps_info_data, length,
							    offset);
					pRawTlvBuffer += length;
					p_dev_cap->extraCapsLength +=
						(length + 3); /* 2b Tag + 1b Length */
				} else {
					offset += length;
				}
			} else {
				if ((length + offset) > caps_info_len) {
					LOG_ERR("%s: Invalid length for extended capability info",
						__func__);
					status = UWB_STATUS_CODE_CORRUPTED;
					break;
				} else if (((p_dev_cap->extraCapsLength + length +
					     2 /* Tag + Length */) > rawTlvBufferLength)) {
					/* Insufficient memory */
					LOG_WRN("Insufficient buffer, skipping");
					offset += length;
				} else if (pRawTlvBuffer) {
					*pRawTlvBuffer++ = tag;
					*pRawTlvBuffer++ = length;
					UWB_STREAM_TO_ARRAY(pRawTlvBuffer, caps_info_data, length,
							    offset);
					pRawTlvBuffer += length;
					p_dev_cap->extraCapsLength +=
						(length + 2); /* Tag + Length */
				} else {
					offset += length;
				}
			}
		} break;
		}
	}

	return status;
}

uint16_t uwb_calculate_controller_hus_session_payload_length(
	const uwb_hus_controller_secondary_session_config_t *const p_hus_secondary_session_config,
	const uint8_t num_phases)
{
	uint16_t total_length = 0;

	total_length += 4; /* Size of session handle */
	total_length += 1; /* Number of phases */
	for (int i = 0; i < num_phases; i++) {
		if (total_length > (UINT16_MAX - 4 - 2 - 2 - 1 /* Fixed length fields */)) {
			return 0;
		}
		total_length += 4; /* Size of Session ID */
		total_length += 2; /* Size of start_slot_index */
		total_length += 2; /* Size of end_slot_index */
		total_length += 1; /* Size of control */
		if (p_hus_secondary_session_config[i].control & 0x01) {
			/* Extended MAC address */
			if (total_length > (UINT16_MAX - UWB_EXTENDED_MAC_ADDRESS_LEN)) {
				return 0;
			}
			total_length += UWB_EXTENDED_MAC_ADDRESS_LEN; /* size of mac_addr */
		} else {
			if (total_length > (UINT16_MAX - UWB_SHORT_MAC_ADDRESS_LEN)) {
				return 0;
			}
			total_length += UWB_SHORT_MAC_ADDRESS_LEN; /* size of mac_addr */
		}
	}
	return total_length;
}

uwb_status_code_t uwb_serialize_controller_hus_session_payload(
	const uint32_t session_handle, const uint8_t num_phases,
	const uwb_hus_controller_secondary_session_config_t *const p_hus_secondary_session_config,
	uint8_t *const p_cmd_buf, uint16_t *const p_cmd_buf_len)
{
	uint16_t offset = 0;

	if (p_cmd_buf == NULL || p_cmd_buf_len == NULL) {
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}

	if ((offset + sizeof(uint32_t)) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT32_TO_STREAM(p_cmd_buf, session_handle, offset);
	if ((offset + sizeof(uint8_t)) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, num_phases, offset);

	for (int i = 0; i < num_phases; i++) {
		if ((offset + sizeof(uint32_t)) > *p_cmd_buf_len) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT32_TO_STREAM(p_cmd_buf, p_hus_secondary_session_config[i].session_id,
				     offset);
		if ((offset + sizeof(uint16_t)) > *p_cmd_buf_len) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT16_TO_STREAM(p_cmd_buf, p_hus_secondary_session_config[i].start_slot_index,
				     offset);
		if ((offset + sizeof(uint16_t)) > *p_cmd_buf_len) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT16_TO_STREAM(p_cmd_buf, p_hus_secondary_session_config[i].end_slot_index,
				     offset);
		if ((offset + sizeof(uint8_t)) > *p_cmd_buf_len) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT8_TO_STREAM(p_cmd_buf, p_hus_secondary_session_config[i].control, offset);
		if (p_hus_secondary_session_config[i].control & 0x01) {
			if ((offset + UWB_EXTENDED_MAC_ADDRESS_LEN) > *p_cmd_buf_len) {
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			UWB_ARRAY_TO_STREAM(p_cmd_buf, p_hus_secondary_session_config[i].mac_addr,
					    UWB_EXTENDED_MAC_ADDRESS_LEN, offset);
		} else {
			if ((offset + UWB_SHORT_MAC_ADDRESS_LEN) > *p_cmd_buf_len) {
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			UWB_ARRAY_TO_STREAM(p_cmd_buf, p_hus_secondary_session_config[i].mac_addr,
					    UWB_SHORT_MAC_ADDRESS_LEN, offset);
		}
	}

	*p_cmd_buf_len = offset;
	return UWB_STATUS_CODE_SUCCESS;
}

uint16_t uwb_calculate_set_controlee_hus_session_payload_length(const uint8_t num_phases)
{
	uint16_t total_length = 0;

	total_length += 4; /* Size of session handle */
	total_length += 1; /* Size of num_phases */
	total_length += (num_phases * sizeof(uint32_t));
	return total_length;
}

uwb_status_code_t
uwb_serialize_controlee_hus_session_payload(const uint32_t session_handle, const uint8_t num_phases,
					    const uint32_t *const p_hus_secondary_session_handles,
					    uint8_t *const p_cmd_buf, uint16_t *const p_cmd_buf_len)
{
	uint32_t offset = 0;

	if ((offset + sizeof(uint32_t)) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT32_TO_STREAM(p_cmd_buf, session_handle, offset);
	if ((offset + sizeof(uint8_t)) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, num_phases, offset);

	for (int i = 0; i < num_phases; i++) {
		if ((offset + sizeof(uint32_t)) > *p_cmd_buf_len) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT32_TO_STREAM(p_cmd_buf, p_hus_secondary_session_handles[i], offset);
	}

	*p_cmd_buf_len = (uint16_t)offset;
	return UWB_STATUS_CODE_SUCCESS;
}

uint16_t uwb_calculate_dtpcm_payload_length(const uint8_t data_transfer_control,
					    const uint8_t dtpml_size)
{
	uint32_t total_length = 0;
	uint8_t mac_address_len = 0;
	uint8_t slot_bitmap_len = 0;

	total_length += sizeof(uint32_t); /* Size of sessionHandle */
	total_length += sizeof(uint8_t);  /* Size of dtpcmRepetition */
	total_length += sizeof(uint8_t);  /* Size of data_transfer_control */
	total_length += sizeof(uint8_t);  /* Size of dtpml_size */
	if (data_transfer_control & 0x01) {
		mac_address_len = UWB_EXTENDED_MAC_ADDRESS_LEN;
	} else {
		mac_address_len = UWB_SHORT_MAC_ADDRESS_LEN;
	}

	uint8_t slot_bitmap_size =
		UWB_DTPCM_DATA_TRANSFER_GET_SLOT_BITMAP_SIZE(data_transfer_control);
	if (UWB_DATA_TX_SLOT_BITMAP_SIZE_7 == slot_bitmap_size) {
		slot_bitmap_len = 0;
	} else {
		slot_bitmap_len = (1 << slot_bitmap_size);
	}

	total_length += (mac_address_len * dtpml_size) + (slot_bitmap_len * dtpml_size) +
			(dtpml_size) /* Stop Data transfer */;
	if (total_length > UINT16_MAX) {
		return 0;
	}
	return total_length;
}

uwb_status_code_t uwb_serialize_dtpcm_payload(const uint32_t session_handle,
					      const uint8_t dtpcm_repetition,
					      const uint8_t data_transfer_control,
					      const uwb_data_tx_phase_mng_list_t *const dtpml,
					      const uint8_t dtpml_size, uint8_t *const p_cmd_buf,
					      uint16_t *const p_cmd_buf_len)
{
	uint32_t offset = 0;
	uint8_t slot_bitmap_len = 0;

	if (offset + sizeof(uint32_t) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT32_TO_STREAM(p_cmd_buf, session_handle, offset);
	if (offset + sizeof(uint8_t) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, dtpcm_repetition, offset);
	if (offset + sizeof(uint8_t) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, data_transfer_control, offset);
	if (offset + sizeof(uint8_t) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, dtpml_size, offset);

	uint8_t mac_address_len = 0;

	if (data_transfer_control & 0x01) {
		mac_address_len = UWB_EXTENDED_MAC_ADDRESS_LEN;
	} else {
		mac_address_len = UWB_SHORT_MAC_ADDRESS_LEN;
	}

	uint8_t slot_bitmap_size =
		UWB_DTPCM_DATA_TRANSFER_GET_SLOT_BITMAP_SIZE(data_transfer_control);

	if (UWB_DATA_TX_SLOT_BITMAP_SIZE_7 == slot_bitmap_size) {
		slot_bitmap_len = 0;
	} else {
		slot_bitmap_len = (1 << slot_bitmap_size);
	}

	for (uint8_t dtpml_index = 0; dtpml_index < dtpml_size; dtpml_index++) {
		if ((offset + mac_address_len) > *p_cmd_buf_len) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_ARRAY_TO_STREAM(p_cmd_buf, dtpml[dtpml_index].mac_addr, mac_address_len,
				    offset);
		if ((offset + slot_bitmap_len) > *p_cmd_buf_len) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_ARRAY_TO_STREAM(p_cmd_buf, dtpml[dtpml_index].slot_bitmap, slot_bitmap_len,
				    offset);
		if ((offset + sizeof(uint8_t)) > *p_cmd_buf_len) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT8_TO_STREAM(p_cmd_buf, dtpml[dtpml_index].stop_data_transfer, offset);
	}

	*p_cmd_buf_len = offset;
	return UWB_STATUS_CODE_SUCCESS;
}

uint16_t uwb_calculate_update_controller_multicast_list_payload_length(const uint8_t action,
								       const uint8_t num_controlees)
{
	uint32_t total_length = 0;

	total_length += 4; /* Size of session handle */
	total_length += 1; /* Size of action */
	total_length += 1; /* Size of number of controlees */
	uint16_t controlee_list_len = 2 /* Short address */ + 4 /* Subsession ID */;

	if (action == UWB_MULTICAST_CONTROLLER_ACTION_ADD_CONTROLEE_SESSION_KEY16) {
		controlee_list_len += 16;
	} else if (action == UWB_MULTICAST_CONTROLLER_ACTION_ADD_CONTROLEE_SESSION_KEY32) {
		controlee_list_len += 32;
	}
	if ((uint16_t)(controlee_list_len * num_controlees) >
	    (uint16_t)(UINT16_MAX - total_length)) {
		return 0;
	}
	total_length += (controlee_list_len * num_controlees);
	return total_length;
}

uint16_t
uwb_calculate_update_controller_multicast_list_response_length(const uint8_t num_controlees)
{
	uint32_t total_length = 0;

	total_length += 1; /* Status */
	total_length += 1; /* Number of controlees */
	total_length += (3 /* 2Byte address + 1Byte status */ * num_controlees);
	return total_length;
}

uwb_status_code_t uwb_serialize_update_controller_multicast_list_payload(
	const uint32_t session_handle, const uint8_t action,
	uwb_multicast_controlee_list_context_t *p_controlee_context, const uint8_t num_controlees,
	uint8_t *p_cmd_buf, uint16_t *const length)
{
	uint16_t serialized_length = 0;

	UWB_UINT32_TO_STREAM(p_cmd_buf, session_handle, serialized_length);
	UWB_UINT8_TO_STREAM(p_cmd_buf, action, serialized_length);
	UWB_UINT8_TO_STREAM(p_cmd_buf, num_controlees, serialized_length);

	for (uint8_t i = 0; i < num_controlees; i++) {
		if (serialized_length > (*length - 2)) {
			*length = 0;
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT16_TO_STREAM(p_cmd_buf, p_controlee_context[i].short_address,
				     serialized_length);
		if (serialized_length > (*length - 4)) {
			*length = 0;
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT32_TO_STREAM(p_cmd_buf, p_controlee_context[i].subsession_id,
				     serialized_length);

		if (action == UWB_MULTICAST_CONTROLLER_ACTION_ADD_CONTROLEE_SESSION_KEY16) {
			if ((((uint32_t)serialized_length) + UWB_SUB_SESSION_KEY_16_LEN) >
			    *length) {
				*length = 0;
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			UWB_ARRAY_TO_STREAM(p_cmd_buf, p_controlee_context[i].subsession_key,
					    UWB_SUB_SESSION_KEY_16_LEN, serialized_length);
		} else if (action == UWB_MULTICAST_CONTROLLER_ACTION_ADD_CONTROLEE_SESSION_KEY32) {
			if ((((uint32_t)serialized_length) + UWB_SUB_SESSION_KEY_32_LEN) >
			    *length) {
				*length = 0;
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			UWB_ARRAY_TO_STREAM(p_cmd_buf, p_controlee_context[i].subsession_key,
					    UWB_SUB_SESSION_KEY_32_LEN, serialized_length);
		}
	}

	*length = serialized_length;
	return UWB_STATUS_CODE_SUCCESS;
}

static uwb_multicast_controlee_list_context_t *
uwb_find_mac_addr_in_controlee_list(const uint16_t address,
				    uwb_multicast_controlee_list_context_t *const p_controlee_list,
				    const uint8_t num_controlees)
{
	for (uint8_t controlee_num = 0; controlee_num < num_controlees; controlee_num++) {
		if (address == p_controlee_list[controlee_num].short_address) {
			return &p_controlee_list[controlee_num];
		}
	}
	return NULL;
}

uwb_status_code_t uwb_deserialize_update_controller_multicast_list_resp(
	uwb_multicast_controlee_list_context_t *const p_controlee_list,
	const uint8_t num_controlees, const uint8_t *const response, const uint16_t response_length)
{
	uwb_status_code_t status = UWB_STATUS_CODE_FAILED;
	uint32_t offset = 0;
	uint8_t num_controlees_rsp = 0;
	const uint8_t *pResponse = response;

	UWB_STREAM_TO_UINT8(num_controlees_rsp, pResponse, offset);
	if (num_controlees_rsp > num_controlees) {
		LOG_ERR("Invalid response");
		return status;
	}

	for (size_t i = 0; i < num_controlees_rsp; i++) {
		uint16_t mac_address = 0;

		UWB_STREAM_TO_UINT16(mac_address, response, offset);
		UWB_STREAM_TO_UINT8(status, response, offset);
		uwb_multicast_controlee_list_context_t *controlee =
			uwb_find_mac_addr_in_controlee_list(mac_address, p_controlee_list,
							    num_controlees);

		if (!controlee) {
			LOG_ERR("Unknown MAC Address returned");
			return status;
		}
		controlee->status = status;
	}

	return UWB_STATUS_CODE_SUCCESS;
}

uint16_t uwb_calculate_dt_anchor_ranging_round_command_length(
	const uint8_t n_active_rounds, const uwb_mac_addr_mode_t mac_addressing_mode,
	const uwb_active_rounds_config_t *const round_config_list)
{
	uint16_t total_length = 0;

	total_length += 4; /* Size of session handle */
	total_length += 1; /* Size of n_active_rounds */
	for (uint8_t i = 0; i < n_active_rounds; i++) {
		if (total_length > (UINT16_MAX - 1)) {
			return 0;
		}
		total_length += 1; /* Size of roundIndex */
		if (total_length > (UINT16_MAX - 1)) {
			return 0;
		}
		total_length += 1; /* Size of rangingRole */
		/* if ranging role is Initiator adding Subsequent fields */
		if (round_config_list[i].rangingRole == UWB_DT_ANCHOR_ROLE_INITIATOR) {
			if (total_length > (UINT16_MAX - 1)) {
				return 0;
			}
			total_length += 1; /* Size of noOfResponders */
			if (mac_addressing_mode == UWB_MAC_ADDR_MODE_2BYTES) {
				if (total_length >
				    (UINT16_MAX - (UWB_SHORT_MAC_ADDRESS_LEN *
						   round_config_list[i].num_responders))) {
					return 0;
				}
				total_length += UWB_SHORT_MAC_ADDRESS_LEN *
						round_config_list[i].num_responders;
			} else {
				if (total_length >
				    (UINT16_MAX - (UWB_EXTENDED_MAC_ADDRESS_LEN *
						   round_config_list[i].num_responders))) {
					return 0;
				}
				total_length += UWB_EXTENDED_MAC_ADDRESS_LEN *
						round_config_list[i].num_responders;
			}
			if (total_length > (UINT16_MAX - 1)) {
				return 0;
			}
			total_length += 1; /*size of responderSlotScheduling*/
			if (round_config_list[i].responderSlotScheduling ==
			    UWB_DT_ANCHOR_RESPONDER_SCHEDULING_SPECIFIED) {
				if (total_length >
				    (UINT16_MAX - round_config_list[i].num_responders)) {
					return 0;
				}
				total_length += round_config_list[i].num_responders;
			}
		}
	}
	return total_length;
}

uwb_status_code_t uwb_serialize_update_active_rounds_anchor_payload(
	uint32_t session_handle, const uint8_t n_active_rounds,
	const uwb_mac_addr_mode_t mac_addressing_mode,
	const uwb_active_rounds_config_t *const round_config_list, uint8_t *const p_cmd_buf,
	uint16_t *const p_payload_length)
{
	uint32_t offset = 0;

	/* adding parameter session ID */
	if ((offset + sizeof(session_handle)) > *p_payload_length) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT32_TO_STREAM(p_cmd_buf, session_handle, offset);
	/* adding parameter n_active_rounds */
	if ((offset + sizeof(n_active_rounds)) > *p_payload_length) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, n_active_rounds, offset);

	for (int i = 0; i < n_active_rounds; i++) {
		/* adding parameter n_active_rounds */
		if ((offset + sizeof(round_config_list[i].roundIndex)) > *p_payload_length) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT8_TO_STREAM(p_cmd_buf, round_config_list[i].roundIndex, offset);
		/* adding parameter rangingRole */
		if ((offset + sizeof(round_config_list[i].rangingRole)) > *p_payload_length) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT8_TO_STREAM(p_cmd_buf, round_config_list[i].rangingRole, offset);

		/* if ranging role is Initiator adding Subsequent fields */
		if (round_config_list[i].rangingRole == UWB_DT_ANCHOR_ROLE_INITIATOR) {
			/* adding parameter num_responders */
			if ((offset + sizeof(round_config_list[i].num_responders)) >
			    *p_payload_length) {
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			UWB_UINT8_TO_STREAM(p_cmd_buf, round_config_list[i].num_responders, offset);

			uint16_t dst_mac_add_len = 0;
			/* depending upon the mac_addressing_mode adding  responderMacAddressList*/
			if (mac_addressing_mode == UWB_MAC_ADDR_MODE_2BYTES) {
				dst_mac_add_len = UWB_SHORT_MAC_ADDRESS_LEN *
						  round_config_list[i].num_responders;
			} else {
				dst_mac_add_len = UWB_EXTENDED_MAC_ADDRESS_LEN *
						  round_config_list[i].num_responders;
			}

			if ((offset + dst_mac_add_len) > *p_payload_length) {
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			if (!round_config_list[i].responderMacAddressList) {
				offset = 0;
				break;
			}
			UWB_ARRAY_TO_STREAM(p_cmd_buf, round_config_list[i].responderMacAddressList,
					    dst_mac_add_len, offset);

			/* adding parameter responderSlotScheduling
			 *  depending upon responderSlotScheduling *responderSlots will be added
			 */
			if ((offset + sizeof(round_config_list[i].responderSlotScheduling)) >
			    *p_payload_length) {
				return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
			}
			UWB_UINT8_TO_STREAM(p_cmd_buf, round_config_list[i].responderSlotScheduling,
					    offset);
			if (round_config_list[i].responderSlotScheduling ==
			    UWB_DT_ANCHOR_RESPONDER_SCHEDULING_IMPLICIT) {
			} else if (round_config_list[i].responderSlots) {
				if ((offset + round_config_list[i].num_responders) >
				    *p_payload_length) {
					return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
				}
				UWB_ARRAY_TO_STREAM(p_cmd_buf, round_config_list[i].responderSlots,
						    round_config_list[i].num_responders, offset);
			} else {
				offset = 0;
				*p_payload_length = 0;
				return UWB_STATUS_CODE_INVALID_ARGUMENT;
			}
		}
	}

	*p_payload_length = offset;
	return UWB_STATUS_CODE_SUCCESS;
}

uint16_t uwb_calculate_update_active_rounds_receiver_command_length(const uint8_t n_active_rounds)
{
	uint16_t total_length = 0;

	total_length += 4; /* Size of session handle */
	total_length += 1; /* Size of n_active_rounds */
	total_length += n_active_rounds;
	return total_length;
}

uwb_status_code_t uwb_serialize_update_tag_active_rounds_payload(
	const uint32_t session_handle, const uint8_t n_active_rounds,
	const uint8_t *const round_index_list, uint8_t *const p_cmd_buf,
	uint16_t *const p_cmd_buf_len)
{
	uint16_t offset = 0;

	if ((offset + sizeof(uint32_t)) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT32_TO_STREAM(p_cmd_buf, session_handle, offset);
	if ((offset + sizeof(uint8_t)) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, n_active_rounds, offset);

	for (int i = 0; i < n_active_rounds; i++) {
		if ((offset + sizeof(uint8_t)) > *p_cmd_buf_len) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT8_TO_STREAM(p_cmd_buf, round_index_list[i], offset);
	}

	*p_cmd_buf_len = offset;
	return UWB_STATUS_CODE_SUCCESS;
}

uint16_t uwb_calculate_logical_link_create_payload_length(void)
{
	uint16_t total_length = 0;

	total_length += sizeof(uint32_t);             /* Size of session_handle */
	total_length += sizeof(uint8_t);              /* Size of llm_selector */
	total_length += UWB_EXTENDED_MAC_ADDRESS_LEN; /* Size of dst_address */
	total_length += sizeof(uint8_t);              /* Size of ll_class_length */
	total_length += sizeof(uint8_t);              /* Size of max_sdu_size_length */
	total_length += sizeof(uint8_t);              /* Size of max_sdu_size_value */

	return total_length;
}

uwb_status_code_t uwb_serialize_create_logical_link_cmd(
	const uint32_t session_handle,
	const uwb_link_layer_mode_selector_t link_layer_mode_selector,
	const uint8_t *const destination_address, const uint8_t logical_link_class,
	const uint8_t max_sdu_size_length, const uint8_t max_sdu_size_value,
	uint8_t *const p_cmd_buf, uint16_t *const p_cmd_buf_len)
{
	uint32_t offset = 0;

	if (p_cmd_buf == NULL || p_cmd_buf_len == NULL || destination_address == NULL) {
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}

	if ((offset + sizeof(uint32_t)) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT32_TO_STREAM(p_cmd_buf, session_handle, offset);
	if ((offset + sizeof(uint8_t)) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, link_layer_mode_selector, offset);
	if ((offset + UWB_EXTENDED_MAC_ADDRESS_LEN) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_ARRAY_TO_STREAM(p_cmd_buf, destination_address, UWB_EXTENDED_MAC_ADDRESS_LEN, offset);
	if ((offset + sizeof(uint8_t)) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, logical_link_class, offset);
	/** MAX SDU SIZE Value field not present if MAX SDU SIZE Length is 0. */
	if ((offset + sizeof(uint8_t)) > *p_cmd_buf_len) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT8_TO_STREAM(p_cmd_buf, max_sdu_size_length, offset);
	if (max_sdu_size_length != 0x00) {
		if ((offset + sizeof(uint8_t)) > *p_cmd_buf_len) {
			return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
		}
		UWB_UINT8_TO_STREAM(p_cmd_buf, max_sdu_size_value, offset);
	}

	*p_cmd_buf_len = offset;
	return UWB_STATUS_CODE_SUCCESS;
}

uwb_status_code_t uwb_deserialize_link_get_params_payload(
	const uint8_t *const p_response,
	uwb_logical_link_get_params_rsp_t *const p_logical_link_get_params_rsp)
{
	uint16_t offset = 0;
	uint16_t controlField;

	UWB_STREAM_TO_UINT16(p_logical_link_get_params_rsp->control_field, p_response, offset);

	controlField =
		p_logical_link_get_params_rsp->control_field & UWB_LL_GET_PARAM_CONTROL_FIELD_MASK;
	LOG_DBG("Control Field : 0x%X", controlField);
	/* Maximum LL SDU size */
	if (controlField & UWB_LL_GET_PARAM_CONTROL_FIELD_SDU_SIZE_BITMASK) {
		UWB_STREAM_TO_UINT16(p_logical_link_get_params_rsp->max_ll_sdu_size, p_response,
				     offset);
	}
	/* Maximum LL PDU size */
	if (controlField & UWB_LL_GET_PARAM_CONTROL_FIELD_PDU_SIZE_BITMASK) {
		UWB_STREAM_TO_UINT16(p_logical_link_get_params_rsp->max_ll_pdu_size, p_response,
				     offset);
	}
	/* Transmit Window Size, TxW */
	if (controlField & UWB_LL_GET_PARAM_CONTROL_FIELD_TxW_BITMASK) {
		UWB_STREAM_TO_UINT8(p_logical_link_get_params_rsp->tx_window_size, p_response,
				    offset);
	}
	/* Receive Window Size, RxW */
	if (controlField & UWB_LL_GET_PARAM_CONTROL_FIELD_RxW_BITMASK) {
		UWB_STREAM_TO_UINT8(p_logical_link_get_params_rsp->rx_window_size, p_response,
				    offset);
	}
	/* Repetition count Max */
	if (controlField & UWB_LL_GET_PARAM_CONTROL_FIELD_REP_CNT_MAX_BITMASK) {
		UWB_STREAM_TO_UINT8(p_logical_link_get_params_rsp->repetition_count_max, p_response,
				    offset);
	}
	/* Link TO */
	if (controlField & UWB_LL_GET_PARAM_CONTROL_FIELD_LINK_TO_BITMASK) {
		UWB_STREAM_TO_UINT8(p_logical_link_get_params_rsp->link_to, p_response, offset);
	}
	/* PORT */
	if (controlField & UWB_LL_GET_PARAM_CONTROL_FIELD_PORT_BITMASK) {
		UWB_STREAM_TO_UINT8(p_logical_link_get_params_rsp->port, p_response, offset);
	}
	/* Maximum Transceiver LL SDU size */
	if (controlField & UWB_LL_GET_PARAM_CONTROL_FIELD_MAX_TRANSCEIVER_SDU_BITMASK) {
		UWB_STREAM_TO_UINT8(p_logical_link_get_params_rsp->max_transceiver_ll_sdu_size,
				    p_response, offset);
	}

	return UWB_STATUS_CODE_SUCCESS;
}

uint16_t uwb_calculate_send_data_payload_length(const uint16_t data_size)
{
	uint32_t payload_length = 0;

	if (data_size == 0) {
		return payload_length;
	}
	payload_length += sizeof(uint32_t);             /* connectionIdentifier */
	payload_length += UWB_EXTENDED_MAC_ADDRESS_LEN; /* Destination address */
	payload_length += sizeof(uint16_t);             /* Sequence number */
	payload_length += sizeof(uint16_t);             /* Data size */
	payload_length += data_size;
	if (payload_length > UINT16_MAX) {
		return 0;
	}
	return payload_length;
}

uwb_status_code_t uwb_serialize_send_data_payload(const uint32_t connection_identifier,
						  const uint8_t *const destination_address,
						  const uint16_t sequence_number,
						  const uint8_t *const data,
						  const uint16_t data_size, uint8_t *const payload,
						  uint16_t *const payload_length)
{
	uint32_t buffer_size = *payload_length;
	uint32_t offset = 0;

	if (data_size == 0 || !data) {
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}
	if ((offset + sizeof(connection_identifier)) > buffer_size) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT32_TO_STREAM(payload, connection_identifier, offset);
	if ((offset + UWB_EXTENDED_MAC_ADDRESS_LEN) > buffer_size) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_ARRAY_TO_STREAM(payload, destination_address, UWB_EXTENDED_MAC_ADDRESS_LEN, offset);
	if ((offset + sizeof(sequence_number)) > buffer_size) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT16_TO_STREAM(payload, sequence_number, offset);
	if ((offset + sizeof(data_size)) > buffer_size) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT16_TO_STREAM(payload, data_size, offset);
	if ((offset + data_size) > buffer_size) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_ARRAY_TO_STREAM(payload, data, data_size, offset);
	*payload_length = offset;

	return UWB_STATUS_CODE_SUCCESS;
}

uint16_t uwb_calculate_ll_send_data_payload_length(const uint16_t data_size)
{
	uint32_t payload_length = 0;

	payload_length += sizeof(uint32_t); /* connectionIdentifier */
	payload_length += sizeof(uint16_t); /* Sequence number */
	payload_length += sizeof(uint16_t); /* Data size */
	payload_length += data_size;
	if (payload_length > UINT16_MAX) {
		LOG_DBG("Could not calculate payload length");
		return 0;
	}
	return payload_length;
}

uwb_status_code_t uwb_serialize_ll_send_data_payload(const uint32_t connection_identifier,
						     const uint16_t sequence_number,
						     const uint8_t *const data,
						     const uint16_t data_size,
						     uint8_t *const payload,
						     uint16_t *const payload_length)
{
	uint32_t buffer_size = *payload_length;
	uint32_t offset = 0;

	if (data_size > 0 && !data) {
		return UWB_STATUS_CODE_INVALID_ARGUMENT;
	}
	if ((offset + sizeof(connection_identifier)) > buffer_size) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT32_TO_STREAM(payload, connection_identifier, offset);
	if ((offset + sizeof(sequence_number)) > buffer_size) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT16_TO_STREAM(payload, sequence_number, offset);
	if ((offset + sizeof(data_size)) > buffer_size) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_UINT16_TO_STREAM(payload, data_size, offset);
	if ((offset + data_size) > buffer_size) {
		return UWB_STATUS_CODE_INSUFFICIENT_BUFFER;
	}
	UWB_ARRAY_TO_STREAM(payload, data, data_size, offset);
	*payload_length = offset;

	return UWB_STATUS_CODE_SUCCESS;
}
