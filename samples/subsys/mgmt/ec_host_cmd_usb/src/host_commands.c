/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/version.h>

#include "host_commands.h"

LOG_MODULE_REGISTER(host_commands, LOG_LEVEL_INF);

#define USBHC_MAX_REQUEST_SIZE CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE
#define USBHC_MAX_RESPONSE_SIZE CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_SIZE

static uint8_t row0;
#define MKBP_COLS 8

static void mod_input_cb(struct input_event *evt, void *user_data)
{
	uint8_t bit;

	if (!evt->sync) {
		return;
	}

	switch (evt->code) {
	case INPUT_KEY_0:
	case INPUT_KEY_A:
		bit = 0;
		break;
	case INPUT_KEY_1:
	case INPUT_KEY_B:
		bit = 1;
		break;
	case INPUT_KEY_2:
	case INPUT_KEY_C:
		bit = 2;
		break;
	case INPUT_KEY_3:
	case INPUT_KEY_D:
		bit = 3;
		break;
	default:
		return;
	}

	if (evt->value) {
		row0 |= BIT(bit);
	} else {
		row0 &= ~BIT(bit);
	}

	ec_host_cmd_backend_usb_trigger_event();
}

INPUT_CALLBACK_DEFINE(NULL, mod_input_cb, NULL);

static enum ec_host_cmd_status ec_cmd_version(struct ec_host_cmd_handler_args *args)
{
	struct ec_response_get_version_v1 *const response = args->output_buf;

	strncpy(response->version_string_ro,
		STRINGIFY(BUILD_VERSION), sizeof(response->version_string_ro));

	if (args->version == 0) {
		args->output_buf_size = sizeof(struct ec_response_get_version);
	} else {
		args->output_buf_size = sizeof(*response);
	}

	return EC_HOST_CMD_SUCCESS;
}

EC_HOST_CMD_HANDLER_RESP_ONLY(EC_CMD_GET_VERSION, ec_cmd_version, BIT(1) | BIT(0),
			      struct ec_response_get_version_v1);


static enum ec_host_cmd_status host_command_protocol_info(struct ec_host_cmd_handler_args *args)
{
	struct ec_response_get_protocol_info *r = args->output_buf;

	memset(r, 0, sizeof(*r));
	r->protocol_versions |= BIT(3);
	r->max_request_packet_size = USBHC_MAX_REQUEST_SIZE;
	r->max_response_packet_size = USBHC_MAX_RESPONSE_SIZE;
	r->flags = EC_PROTOCOL_INFO_IN_PROGRESS_SUPPORTED;

	args->output_buf_size = sizeof(*r);

	return EC_HOST_CMD_SUCCESS;
}

EC_HOST_CMD_HANDLER_RESP_ONLY(EC_CMD_GET_PROTOCOL_INFO, host_command_protocol_info, BIT(0),
			      struct ec_response_get_protocol_info);

static enum ec_host_cmd_status hc_get_features(struct ec_host_cmd_handler_args *args)
{
	struct ec_response_get_features *r = args->output_buf;

	memset(r, 0, sizeof(*r));
	r->flags[0] = 0;
	r->flags[1] = 0;

	args->output_buf_size = sizeof(*r);

	return EC_HOST_CMD_SUCCESS;
}

EC_HOST_CMD_HANDLER_RESP_ONLY(EC_CMD_GET_FEATURES, hc_get_features, BIT(0),
			      struct ec_response_get_features);

static enum ec_host_cmd_status ec_mkbp_info(struct ec_host_cmd_handler_args *args)
{
	const struct ec_params_mkbp_info *p = args->input_buf;

	switch (p->info_type) {
	case EC_MKBP_INFO_SUPPORTED:
		union ec_response_get_next_data *r = args->output_buf;

		switch (p->event_type) {
		case EC_MKBP_EVENT_BUTTON:
			r->buttons = 0;
			args->output_buf_size = sizeof(r->buttons);
			return EC_HOST_CMD_SUCCESS;
		case EC_MKBP_EVENT_SWITCH:
			r->switches = 0;
			args->output_buf_size = sizeof(r->switches);
			return EC_HOST_CMD_SUCCESS;
		default:
			return EC_HOST_CMD_INVALID_PARAM;
		}
	default:
		return EC_HOST_CMD_INVALID_PARAM;
	}
}

EC_HOST_CMD_HANDLER(EC_CMD_MKBP_INFO, ec_mkbp_info, BIT(1),
		    struct ec_params_mkbp_info, struct ec_response_mkbp_info);

static enum ec_host_cmd_status mkbp_get_next_event(struct ec_host_cmd_handler_args *args)
{
	struct ec_response_get_next_event_v3 *r = args->output_buf;

	memset(r, 0, sizeof(*r));

	r->event_type = EC_MKBP_EVENT_KEY_MATRIX;
	r->data.key_matrix[0] = row0;

	args->output_buf_size = MKBP_COLS + 1;

	return EC_HOST_CMD_SUCCESS;
}

EC_HOST_CMD_HANDLER_RESP_ONLY(EC_CMD_GET_NEXT_EVENT, mkbp_get_next_event, BIT(3),
			      struct ec_response_get_next_event_v3);

static enum ec_host_cmd_status ec_get_cmd_versions(struct ec_host_cmd_handler_args *args)
{
	const struct ec_params_get_cmd_versions *p = args->input_buf;
	struct ec_response_get_cmd_versions *r = args->output_buf;

	STRUCT_SECTION_FOREACH(ec_host_cmd_handler, handler) {
		if (p->cmd == handler->id) {
			r->version_mask = handler->version_mask;
			args->output_buf_size = sizeof(*r);
			return EC_HOST_CMD_SUCCESS;
		}
	}

	return EC_HOST_CMD_INVALID_PARAM;
}

EC_HOST_CMD_HANDLER(EC_CMD_GET_CMD_VERSIONS, ec_get_cmd_versions, BIT(0),
		    struct ec_params_get_cmd_versions,
		    struct ec_response_get_cmd_versions);

static enum ec_host_cmd_status get_uptime_info(struct ec_host_cmd_handler_args *args)
{
	struct ec_response_uptime_info *r = args->output_buf;

	memset(r, 0, sizeof(*r));

	r->time_since_ec_boot_ms = k_uptime_get_32();
	args->output_buf_size = sizeof(*r);

	return EC_HOST_CMD_SUCCESS;
}

EC_HOST_CMD_HANDLER_RESP_ONLY(EC_CMD_GET_UPTIME_INFO, get_uptime_info, BIT(0),
			      struct ec_response_uptime_info);
