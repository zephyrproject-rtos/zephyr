/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <libmctp.h>
#include <libmctp-cmds.h>
#include <zephyr/pmci/mctp/mctp_i3c_controller.h>
#include <zephyr/logging/log.h>

#include "mctp_ssd_power.h"
LOG_MODULE_REGISTER(mctp_get_eid, CONFIG_APP_LOG_LEVEL);

#define MCTP_TAG_OWNER 1
#define MCTP_INSTANCE_ID 0

#define LINK_SETUP_DELAY_MS		500U
#define FSM_DELAY_MS			100U

/* How long the FSM waits for a response before advancing to the next state. */
#define MCTP_RESP_TIMEOUT_MS		1000U

/*
 * Get MCTP Version Support, "MCTP version number entry" selector.
 */
#define MCTP_GET_VERSION_BASE_SPEC	0xFF
#define MCTP_GET_VERSION_CONTROL	0x00
#define MCTP_GET_VERSION_PLDM		0x01

MCTP_I3C_CONTROLLER_DT_DEFINE(mctp_i3c_ctrl, DT_NODELABEL(mctp_i3c_ctrl));

static struct mctp *mctp_ctx;

/* Signalled by rx_message when the response the FSM is waiting for arrives. */
static K_SEM_DEFINE(resp_sem, 0, 1);

/* Control command code the FSM currently expects a response for. */
static uint8_t expected_cmd;

/* Get Version msg_type currently being queried (gives the response context). */
static uint8_t expected_version_msg_type;

/* Message types the endpoint advertised via Get Message Type Support. */
#define MAX_SUPPORTED_MSG_TYPES 16
static uint8_t supported_msg_types[MAX_SUPPORTED_MSG_TYPES];
static uint8_t num_supported_msg_types;

static bool msg_type_supported(uint8_t msg_type)
{
	for (uint8_t i = 0; i < num_supported_msg_types; i++) {
		if (supported_msg_types[i] == msg_type) {
			return true;
		}
	}
	return false;
}

/* Build the rq/d/inst byte for a new request, bumping the instance ID. */
static uint8_t next_rq_dgram_inst(void)
{
	static uint8_t instance_id;
	uint8_t rq_dgram_inst = MCTP_CTRL_HDR_FLAG_REQUEST |
		(instance_id & MCTP_CTRL_HDR_INSTANCE_ID_MASK);

	instance_id = (instance_id + 1U) & MCTP_CTRL_HDR_INSTANCE_ID_MASK;
	return rq_dgram_inst;
}

static void handle_get_endpoint_id_resp(uint8_t src_eid, const void *msg, size_t len)
{
	const struct mctp_ctrl_cmd_get_endpoint_id_resp *resp = msg;

	if (len < sizeof(*resp)) {
		LOG_ERR("Short Get Endpoint ID response from EID %u, len %zu",
			src_eid, len);
		return;
	}

	if (resp->completion_code != MCTP_CTRL_CC_SUCCESS) {
		LOG_ERR("Get Endpoint ID error from EID %u, cc 0x%02x",
			src_eid, resp->completion_code);
		return;
	}

	LOG_INF("EID %u: endpoint_id=%u, endpoint_type=0x%02x, medium=0x%02x",
		src_eid, resp->endpoint_id, resp->endpoint_type,
		resp->medium_specific);
}

/* Handles a header + completion-code only response (e.g. Discovery Notify). */
static void handle_empty_resp(const char *what, uint8_t src_eid, const void *msg, size_t len)
{
	const struct mctp_ctrl_cmd_empty_resp *resp = msg;

	if (len < sizeof(*resp)) {
		LOG_ERR("Short %s response from EID %u, len %zu", what, src_eid, len);
		return;
	}

	if (resp->completion_code != MCTP_CTRL_CC_SUCCESS) {
		LOG_ERR("%s error from EID %u, cc 0x%02x", what, src_eid,
			resp->completion_code);
		return;
	}

	LOG_INF("EID %u: %s success", src_eid, what);
}

static void handle_get_msg_type_resp(uint8_t src_eid, const void *msg, size_t len)
{
	const struct mctp_ctrl_cmd_get_types_resp *resp = msg;

	if (len < sizeof(*resp)) {
		LOG_ERR("Short Get Message Type response from EID %u, len %zu",
			src_eid, len);
		return;
	}

	if (resp->completion_code != MCTP_CTRL_CC_SUCCESS) {
		LOG_ERR("Get Message Type error from EID %u, cc 0x%02x",
			src_eid, resp->completion_code);
		return;
	}

	if (len < sizeof(*resp) + resp->type_count) {
		LOG_ERR("Truncated Get Message Type response from EID %u: %u types, len %zu",
			src_eid, resp->type_count, len);
		return;
	}

	num_supported_msg_types = 0;
	LOG_INF("EID %u: %u supported message type(s)", src_eid, resp->type_count);
	for (uint8_t i = 0; i < resp->type_count; i++) {
		LOG_INF("  msg_type[%u] = 0x%02x", i, resp->types[i]);
		if (num_supported_msg_types < MAX_SUPPORTED_MSG_TYPES) {
			supported_msg_types[num_supported_msg_types++] = resp->types[i];
		}
	}
}

static void handle_get_version_resp(uint8_t src_eid, const void *msg, size_t len)
{
	const struct mctp_ctrl_cmd_get_version_resp *resp = msg;

	/*
	 * An error response (e.g. "versions not supported") carries only the
	 * header + completion code, so validate that minimum before inspecting
	 * the completion code, and only require version_count on success.
	 */
	if (len < sizeof(resp->hdr) + sizeof(resp->completion_code)) {
		LOG_ERR("Short Get Version response from EID %u, len %zu",
			src_eid, len);
		return;
	}

	if (resp->completion_code == MCTP_CTRL_VERSIONS_NOT_SUPPORTED) {
		/*
		 * "Not supported" is expected when the queried message type was not advertised
		 * by Get Message Type Support
		 * (e.g. PLDM on an endpoint that only reports NVMe-MI)
		 */
		if (expected_version_msg_type != MCTP_GET_VERSION_BASE_SPEC &&
		    !msg_type_supported(expected_version_msg_type)) {
			LOG_WRN("EID %u: no version for msg_type 0x%02x not advertised",
				src_eid, expected_version_msg_type);
		} else {
			LOG_ERR("EID %u: msg_type 0x%02x reports no supported versions",
				src_eid, expected_version_msg_type);
		}
		return;
	}

	if (resp->completion_code != MCTP_CTRL_CC_SUCCESS) {
		LOG_ERR("Get Version error from EID %u, cc 0x%02x",
			src_eid, resp->completion_code);
		return;
	}

	if (len < sizeof(*resp) + (size_t)resp->version_count * sizeof(uint32_t)) {
		LOG_ERR("Truncated Get Version response from EID %u: %u versions, len %zu",
			src_eid, resp->version_count, len);
		return;
	}

	LOG_INF("EID %u: %u supported version(s)", src_eid, resp->version_count);
	for (uint8_t i = 0; i < resp->version_count; i++) {
		const uint8_t *v = (const uint8_t *)&resp->versions[i];

		/* Wire order: major, minor, update, alpha (BCD). DSP0236 sec. 12.6. */
		LOG_INF("  version[%u] = 0x%02x 0x%02x 0x%02x 0x%02x (major.minor.update.alpha)",
			i, v[0], v[1], v[2], v[3]);
	}
}

static void rx_message(uint8_t eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	const struct mctp_ctrl_msg_hdr *hdr = msg;

	LOG_HEXDUMP_DBG(msg, len, "MCTP response: ");

	if (len >= sizeof(*hdr) &&
	    hdr->ic_msg_type == MCTP_CTRL_HDR_MSG_TYPE &&
	    !(hdr->rq_dgram_inst & MCTP_CTRL_HDR_FLAG_REQUEST)) {
		switch (hdr->command_code) {
		case MCTP_CTRL_CMD_DISCOVERY_NOTIFY:
			handle_empty_resp("Discovery Notify", eid, msg, len);
			break;
		case MCTP_CTRL_CMD_GET_ENDPOINT_ID:
			handle_get_endpoint_id_resp(eid, msg, len);
			break;
		case MCTP_CTRL_CMD_GET_MESSAGE_TYPE_SUPPORT:
			handle_get_msg_type_resp(eid, msg, len);
			break;
		case MCTP_CTRL_CMD_GET_VERSION_SUPPORT:
			handle_get_version_resp(eid, msg, len);
			break;
		default:
			LOG_WRN("Unhandled control response cmd 0x%02x from EID %u",
				hdr->command_code, eid);
			break;
		}

		/* Wake the FSM if this is the response it is waiting for. */
		if (hdr->command_code == expected_cmd) {
			k_sem_give(&resp_sem);
		}
		return;
	}

	LOG_INF("Received message \"%s\" from endpoint %d to %d, msg_tag %d, len %zu",
		(char *)msg, eid, MCTP_INSTANCE_ID, msg_tag, len);
}

/* Sends a control request that carries only the 3-byte control header. */
static int send_ctrl_header_only(struct mctp *ctx, mctp_eid_t dest_eid,
				 uint8_t command_code, const char *what)
{
	struct mctp_ctrl_msg_hdr req = {
		.ic_msg_type = MCTP_CTRL_HDR_MSG_TYPE,
		.rq_dgram_inst = next_rq_dgram_inst(),
		.command_code = command_code,
	};
	int rc;

	rc = mctp_message_tx(ctx, dest_eid, true, 0U, &req, sizeof(req));
	if (rc != 0) {
		LOG_ERR("Failed to send %s to EID %u: %d", what, dest_eid, rc);
		return rc;
	}

	LOG_DBG("Sent %s to EID %u", what, dest_eid);
	return 0;
}

static int mctp_send_discovery_notify(struct mctp *ctx, mctp_eid_t dest_eid)
{
	return send_ctrl_header_only(ctx, dest_eid, MCTP_CTRL_CMD_DISCOVERY_NOTIFY,
				     "Discovery Notify");
}

static int mctp_send_get_endpoint_id(struct mctp *ctx, mctp_eid_t dest_eid)
{
	return send_ctrl_header_only(ctx, dest_eid, MCTP_CTRL_CMD_GET_ENDPOINT_ID,
				     "Get Endpoint ID");
}

static int mctp_send_get_msg_type_support(struct mctp *ctx, mctp_eid_t dest_eid)
{
	return send_ctrl_header_only(ctx, dest_eid,
				     MCTP_CTRL_CMD_GET_MESSAGE_TYPE_SUPPORT,
				     "Get Message Type Support");
}

static int mctp_send_get_version(struct mctp *ctx, mctp_eid_t dest_eid, uint8_t msg_type)
{
	struct mctp_ctrl_cmd_get_version_req req = {
		.hdr = {
			.ic_msg_type = MCTP_CTRL_HDR_MSG_TYPE,
			.rq_dgram_inst = next_rq_dgram_inst(),
			.command_code = MCTP_CTRL_CMD_GET_VERSION_SUPPORT,
		},
		.msg_type = msg_type,
	};
	int rc;

	/* Remember which type we asked about for the response handler. */
	expected_version_msg_type = msg_type;

	rc = mctp_message_tx(ctx, dest_eid, true, 0U, &req, sizeof(req));
	if (rc != 0) {
		LOG_ERR("Failed to send Get Version (msg_type 0x%02x) to EID %u: %d",
			msg_type, dest_eid, rc);
		return rc;
	}

	LOG_DBG("Sent Get Version (msg_type 0x%02x) to EID %u", msg_type, dest_eid);
	return 0;
}

/*
 * Discovery / enumeration FSM. Each state issues one MCTP control request and
 * waits (response-driven, with timeout) before advancing to the next.
 */
enum mctp_fsm_state {
	FSM_DISCOVERY_NOTIFY,
	FSM_GET_ENDPOINT_ID,
	FSM_GET_MSG_TYPE,
	FSM_GET_VERSION_BASE,
	FSM_GET_VERSION_CONTROL,
	FSM_GET_VERSION_PLDM,
	FSM_DONE,
};

static const char *fsm_state_name(enum mctp_fsm_state state)
{
	switch (state) {
	case FSM_DISCOVERY_NOTIFY:    return "Discovery Notify";
	case FSM_GET_ENDPOINT_ID:     return "Get Endpoint ID";
	case FSM_GET_MSG_TYPE:        return "Get Message Type Support";
	case FSM_GET_VERSION_BASE:    return "Get Version (base spec)";
	case FSM_GET_VERSION_CONTROL: return "Get Version (control)";
	case FSM_GET_VERSION_PLDM:    return "Get Version (PLDM)";
	default:                      return "Done";
	}
}

/* Issues the request for the given state and records the expected response. */
static int fsm_send(enum mctp_fsm_state state, struct mctp *ctx)
{
	switch (state) {
	case FSM_DISCOVERY_NOTIFY:
		expected_cmd = MCTP_CTRL_CMD_DISCOVERY_NOTIFY;
		/* Discovery Notify is always directed at the bus owner. */
		return mctp_send_discovery_notify(ctx, MCTP_EID_NULL);
	case FSM_GET_ENDPOINT_ID:
		expected_cmd = MCTP_CTRL_CMD_GET_ENDPOINT_ID;
		return mctp_send_get_endpoint_id(ctx, MCTP_EID_NULL);
	case FSM_GET_MSG_TYPE:
		expected_cmd = MCTP_CTRL_CMD_GET_MESSAGE_TYPE_SUPPORT;
		return mctp_send_get_msg_type_support(ctx, MCTP_EID_NULL);
	case FSM_GET_VERSION_BASE:
		expected_cmd = MCTP_CTRL_CMD_GET_VERSION_SUPPORT;
		return mctp_send_get_version(ctx, MCTP_EID_NULL, MCTP_GET_VERSION_BASE_SPEC);
	case FSM_GET_VERSION_CONTROL:
		expected_cmd = MCTP_CTRL_CMD_GET_VERSION_SUPPORT;
		return mctp_send_get_version(ctx, MCTP_EID_NULL, MCTP_GET_VERSION_CONTROL);
	case FSM_GET_VERSION_PLDM:
		expected_cmd = MCTP_CTRL_CMD_GET_VERSION_SUPPORT;
		return mctp_send_get_version(ctx, MCTP_EID_NULL, MCTP_GET_VERSION_PLDM);
	default:
		return -EINVAL;
	}
}

static void run_mctp_fsm(struct mctp *ctx)
{
	for (enum mctp_fsm_state state = FSM_GET_ENDPOINT_ID;
	     state != FSM_DONE; state++) {
		int rc;

		LOG_INF("FSM -> %s", fsm_state_name(state));

		/* Drop any stale signal before issuing the next request. */
		k_sem_reset(&resp_sem);

		k_msleep(FSM_DELAY_MS);
		rc = fsm_send(state, ctx);
		if (rc != 0) {
			LOG_WRN("%s: send failed (%d), advancing",
				fsm_state_name(state), rc);
			continue;
		}

		if (k_sem_take(&resp_sem, K_MSEC(MCTP_RESP_TIMEOUT_MS)) != 0) {
			LOG_WRN("%s: no response within %u ms, advancing",
				fsm_state_name(state), MCTP_RESP_TIMEOUT_MS);
		}
	}

	LOG_INF("MCTP FSM complete");
}

int main(void)
{
	LOG_INF("MCTP Get Endpoint ID Sample");

	mctp_ssd_power_init();
	mctp_i3c_initialization();

	/* Initialize MCTP instance (transport-specific init needed here) */
	mctp_ctx = mctp_init();
	if (!mctp_ctx) {
		LOG_ERR("mctp_init() failed");
		return -1;
	}

	__ASSERT_NO_MSG(mctp_ctx != NULL);
	mctp_register_bus(mctp_ctx, &mctp_i3c_ctrl.binding, MCTP_INSTANCE_ID);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);

	/*
	 * Discovery requests are addressed to the null EID: the endpoints have
	 * not been assigned an EID yet, and the I3C binding routes a null-EID
	 * packet to the first endpoint descriptor.
	 */
	LOG_INF("Running MCTP discovery FSM (requests addressed to null EID)");
	run_mctp_fsm(mctp_ctx);

	k_msleep(5000);
	return 0;
}
