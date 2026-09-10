/*
 * Copyright (c) 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/http/client.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <zephyr/sys/util.h>
#include <openthread/thread.h>
#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>
#include <openthread/instance.h>
#include <openthread/cli.h>
#include <openthread/thread.h>
#include <openthread/thread_ftd.h>
#include <openthread/netdiag.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rest_api.h"

LOG_MODULE_REGISTER(rest_api, CONFIG_OTBR_LOG_LEVEL);

/* Buffer for receiving POST data */
static char post_data_buffer[1024];
static size_t post_data_len;

/* Helper function to get OpenThread instance */
static otInstance *get_ot_instance(void)
{
	return openthread_get_default_instance();
}

/* Helper function to parse URL-encoded parameter */
static bool get_url_param(const char *data, const char *param_name,
			  char *output, size_t output_size)
{
	char search_str[64];
	const char *start;
	const char *end;
	size_t len;

	snprintf(search_str, sizeof(search_str), "%s=", param_name);
	start = strstr(data, search_str);

	if (!start) {
		return false;
	}

	start += strlen(search_str);
	end = strchr(start, '&');

	if (end) {
		len = end - start;
	} else {
		len = strlen(start);
	}

	if (len >= output_size) {
		len = output_size - 1;
	}

	memcpy(output, start, len);
	output[len] = '\0';

	return true;
}

/* Helper function to parse JSON string value */
static bool get_json_string_value(const char *json,
				  const char *key,
				  char *output,
				  size_t output_size)
{
	char search_str[64];
	const char *key_pos;
	const char *colon;
	const char *start;
	const char *end;
	size_t len;

	snprintf(search_str, sizeof(search_str), "\"%s\"", key);
	key_pos = strstr(json, search_str);
	if (!key_pos) {
		LOG_DBG("Key '%s' not found in JSON", key);

		return false;
	}

	colon = strchr(key_pos, ':');

	if (!colon) {
		LOG_DBG("No colon found after key '%s'", key);
		return false;
	}

	start = colon + 1;

	while (*start == ' ' || *start == '\t' ||
		   *start == '\n' || *start == '\r') {
		start++;
	}

	if (*start != '"') {
		LOG_DBG("No opening quote found for key '%s'", key);
		return false;
	}
	start++;

	end = start;

	while (*end && *end != '"') {
		if (*end == '\\' && *(end + 1) == '"') {
			end += 2;
		} else {
			end++;
		}
	}

	if (*end != '"') {
		LOG_DBG("No closing quote found for key '%s'", key);
		return false;
	}

	len = end - start;

	if (len >= output_size) {
		len = output_size - 1;
	}

	memcpy(output, start, len);
	output[len] = '\0';

	LOG_DBG("Extracted '%s' = '%s'", key, output);
	return true;
}

/* REST API: GET /api/status */
int rest_api_status_handler(struct http_client_ctx *client,
				 enum http_transaction_status status,
				 const struct http_request_ctx *request_ctx,
				 struct http_response_ctx *response_ctx,
				 void *user_data)
{
	static char json_response[1024];
	otInstance *instance;
	otDeviceRole role;
	const char *role_str;
	uint16_t rloc16;
	uint32_t partition_id;
	uint8_t router_id;
	bool thread_enabled;

	LOG_INF("Status API called, status: %d", status);

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		instance = get_ot_instance();

		if (!instance) {
			snprintf(json_response, sizeof(json_response),
				"{\"error\":"
				"\"OpenThread instance not available\"}");
		} else {
			openthread_mutex_lock();
			role = otThreadGetDeviceRole(instance);
			role_str = "unknown";

			openthread_mutex_unlock();

			switch (role) {
			case OT_DEVICE_ROLE_DISABLED:
				role_str = "disabled";
				break;
			case OT_DEVICE_ROLE_DETACHED:
				role_str = "detached";
				break;
			case OT_DEVICE_ROLE_CHILD:
				role_str = "child";
				break;
			case OT_DEVICE_ROLE_ROUTER:
				role_str = "router";
				break;
			case OT_DEVICE_ROLE_LEADER:
				role_str = "leader";
				break;
			default:
				break;
			}

			openthread_mutex_lock();
			rloc16 = otThreadGetRloc16(instance);
			partition_id = otThreadGetPartitionId(instance);
			router_id = otThreadGetLeaderRouterId(instance);
			thread_enabled =
				otThreadGetDeviceRole(instance) !=
				OT_DEVICE_ROLE_DISABLED;
			openthread_mutex_unlock();

			snprintf(json_response, sizeof(json_response),
				"{\"status\":\"ok\","
				"\"role\":\"%s\","
				"\"rloc16\":\"0x%04x\","
				"\"partition_id\":%u,"
				"\"router_id\":%u,"
				"\"thread_enabled\":%s}",
				 role_str, rloc16, partition_id, router_id,
				 thread_enabled ? "true" : "false");
		}

		response_ctx->body = (const uint8_t *)json_response;
		response_ctx->body_len = strlen(json_response);
		response_ctx->final_chunk = true;
		response_ctx->status = HTTP_200_OK;
	}

	return 0;
}

struct dataset_fields {
	char network_name[OT_NETWORK_NAME_MAX_SIZE + 1];
	char pan_id_str[16];
	char extpanid_str[128];
	char channel_str[16];
	char network_key_str[128];
};

static void extract_dataset_fields(const otOperationalDataset *dataset,
				    struct dataset_fields *fields)
{
	char extpanid_hex[OT_EXT_PAN_ID_SIZE*2 + 1];
	uint8_t byte;
	char key_hex[OT_NETWORK_KEY_SIZE*2 + 1];

	/* Initialize all fields to "null" */
	strncpy(fields->network_name, "", sizeof(fields->network_name));
	strncpy(fields->pan_id_str, "null", sizeof(fields->pan_id_str));
	strncpy(fields->extpanid_str, "null", sizeof(fields->extpanid_str));
	strncpy(fields->channel_str, "null", sizeof(fields->channel_str));
	strncpy(fields->network_key_str, "null", sizeof(fields->network_key_str));

	/* Extract network name */
	if (dataset->mComponents.mIsNetworkNamePresent) {
		size_t name_len = strnlen((const char *)dataset->mNetworkName.m8,
					  OT_NETWORK_NAME_MAX_SIZE);

		memcpy(fields->network_name, dataset->mNetworkName.m8, name_len);
		fields->network_name[name_len] = '\0';
	}

	/* Extract PAN ID */
	if (dataset->mComponents.mIsPanIdPresent) {
		snprintf(fields->pan_id_str, sizeof(fields->pan_id_str),
			 "\"0x%04x\"", dataset->mPanId);
	}

	/* Extract Extended PAN ID */
	if (dataset->mComponents.mIsExtendedPanIdPresent) {

		for (int i = 0; i < OT_EXT_PAN_ID_SIZE; i++) {
			byte = dataset->mExtendedPanId.m8[i];
			snprintf(&extpanid_hex[i * 2], 3, "%02x", byte);
		}
		snprintf(fields->extpanid_str, sizeof(fields->extpanid_str),
			 "\"%s\"", extpanid_hex);
	}

	/* Extract channel */
	if (dataset->mComponents.mIsChannelPresent) {
		snprintf(fields->channel_str, sizeof(fields->channel_str),
			 "%u", dataset->mChannel);
	}

	/* Extract network key */
	if (dataset->mComponents.mIsNetworkKeyPresent) {
		for (int i = 0; i < OT_NETWORK_KEY_SIZE; i++) {
			byte = dataset->mNetworkKey.m8[i];
			snprintf(&key_hex[i * 2], 3, "%02x", byte);
		}
		snprintf(fields->network_key_str, sizeof(fields->network_key_str),
			 "\"%s\"", key_hex);
	}
}

/* REST API: GET /api/dataset */
int rest_api_dataset_get_handler(struct http_client_ctx *client,
				  enum http_transaction_status status,
				  const struct http_request_ctx *request_ctx,
				  struct http_response_ctx *response_ctx,
				  void *user_data)
{
	static char json_response[2048];
	otInstance *instance;
	otOperationalDataset dataset;
	otError error;
	struct dataset_fields fields;

	LOG_INF("Dataset GET API called, status: %d", status);

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		instance = get_ot_instance();
		if (!instance) {
			snprintf(json_response, sizeof(json_response),
				 "{\"error\":"
				 "\"OpenThread instance not available\"}");
		} else {
			openthread_mutex_lock();
			error = otDatasetGetActive(instance, &dataset);
			openthread_mutex_unlock();

			if (error != OT_ERROR_NONE) {
				snprintf(json_response, sizeof(json_response),
					 "{\"error\":"
					 "\"Failed to get dataset\","
					 "\"error_code\":%d}", error);
			} else {
				extract_dataset_fields(&dataset, &fields);
				snprintf(json_response, sizeof(json_response),
					 "{\"status\":\"ok\","
					 "\"network_name\":\"%s\","
					 "\"pan_id\":%s,"
					 "\"extpanid\":%s,"
					 "\"channel\":%s,"
					 "\"network_key\":%s}",
					 fields.network_name,
					 fields.pan_id_str,
					 fields.extpanid_str,
					 fields.channel_str,
					 fields.network_key_str);
			}
		}

		response_ctx->body = (const uint8_t *)json_response;
		response_ctx->body_len = strlen(json_response);
		response_ctx->final_chunk = true;
		response_ctx->status = HTTP_200_OK;
	}

	return 0;
}

/* REST API: GET /api/dataset/hex */
int rest_api_dataset_hex_handler(struct http_client_ctx *client,
				  enum http_transaction_status status,
				  const struct http_request_ctx *request_ctx,
				  struct http_response_ctx *response_ctx,
				  void *user_data)
{
	static char json_response[2048];
	otInstance *instance;
	otOperationalDatasetTlvs dataset_tlvs;
	otError error;
	char hex_string[OT_OPERATIONAL_DATASET_MAX_LENGTH * 2 + 1];

	LOG_INF("Dataset HEX API called, status: %d", status);

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		instance = get_ot_instance();
		if (!instance) {
			snprintf(json_response, sizeof(json_response),
				"{\"error\":"
				"\"OpenThread instance not available\"}");
		} else {
			openthread_mutex_lock();
			error = otDatasetGetActiveTlvs(instance, &dataset_tlvs);
			openthread_mutex_unlock();

			if (error != OT_ERROR_NONE) {
				snprintf(json_response,
					 sizeof(json_response),
					"{\"error\":"
					"\"Failed to get dataset\","
					"\"error_code\":%d}", error);
			} else {
				/* Convert TLVs to hex string */
				for (uint8_t i = 0; i < dataset_tlvs.mLength; i++) {
					snprintf(&hex_string[i * 2], 3,
						"%02x",
						 dataset_tlvs.mTlvs[i]);
				}
				hex_string[dataset_tlvs.mLength * 2] = '\0';

				snprintf(json_response,
					 sizeof(json_response),
					"{\"status\":\"ok\","
					"\"dataset_hex\":\"%s\","
					"\"length\":%d}",
					 hex_string, dataset_tlvs.mLength);
			}
		}

		response_ctx->body = (const uint8_t *)json_response;
		response_ctx->body_len = strlen(json_response);
		response_ctx->final_chunk = true;
		response_ctx->status = HTTP_200_OK;
	}

	return 0;
}

/* Structure to hold node display properties */
struct node_display_info {
	const uint8_t *ext_addr;
	uint16_t rloc16;
	const char *label;
	const char *color;
	const char *shape;
	int size;
};

static void add_node_to_json_direct(char **ptr, size_t *remaining,
				     const struct node_display_info *info,
				     bool *first_node)
{
	int written;

	if (!*first_node) {
		written = snprintf(*ptr, *remaining, ",");
		*ptr += written;
		*remaining -= written;
	}

	written = snprintf(*ptr, *remaining,
			   "{\"id\":\"%02x%02x%02x%02x%02x%02x%02x%02x\","
			   "\"label\":\"%s\","
			   "\"color\":\"%s\","
			   "\"shape\":\"%s\","
			   "\"size\":%d,"
			   "\"rloc16\":\"0x%04x\"}",
			   info->ext_addr[0], info->ext_addr[1],
			   info->ext_addr[2], info->ext_addr[3],
			   info->ext_addr[4], info->ext_addr[5],
			   info->ext_addr[6], info->ext_addr[7],
			   info->label, info->color, info->shape,
			   info->size, info->rloc16);
	*ptr += written;
	*remaining -= written;
	*first_node = false;
}

static void add_edge_to_json_direct(char **ptr, size_t *remaining,
				     const uint8_t *from_addr,
				     const uint8_t *to_addr,
				     int lqi, bool *first_edge)
{
	int written;

	if (!*first_edge) {
		written = snprintf(*ptr, *remaining, ",");
		*ptr += written;
		*remaining -= written;
	}

	written = snprintf(*ptr, *remaining,
			   "{\"from\":\"%02x%02x%02x%02x%02x%02x%02x%02x\","
			   "\"to\":\"%02x%02x%02x%02x%02x%02x%02x%02x\","
			   "\"label\":\"LQI:%d\","
			   "\"arrows\":\"to\","
			   "\"width\":2}",
			   from_addr[0], from_addr[1], from_addr[2], from_addr[3],
			   from_addr[4], from_addr[5], from_addr[6], from_addr[7],
			   to_addr[0], to_addr[1], to_addr[2], to_addr[3],
			   to_addr[4], to_addr[5], to_addr[6], to_addr[7],
			   lqi);
	*ptr += written;
	*remaining -= written;
	*first_edge = false;
}

/* REST API: GET /api/topology */
int rest_api_topology_handler(struct http_client_ctx *client,
			       enum http_transaction_status status,
			       const struct http_request_ctx *request_ctx,
			       struct http_response_ctx *response_ctx,
			       void *user_data)
{
	static char json_response[8192];
	char *ptr;
	size_t remaining;
	int written;
	otInstance *instance;
	otDeviceRole role;
	uint16_t rloc16;
	bool first_node;
	otNeighborInfoIterator iterator;
	otNeighborInfo neighbor_info;
	bool first_edge;
	const char *role_str;
	const char *color;
	const char *shape;
	char self_label[64];
	const char *neighbor_role;
	const char *neighbor_color;
	const char *neighbor_shape;
	char neighbor_label[64];

	const otExtAddress *ext_addr;

	LOG_INF("Topology API called, status: %d", status);

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		ptr = json_response;
		remaining = sizeof(json_response);
		instance = get_ot_instance();

		if (!instance) {
			snprintf(json_response, sizeof(json_response),
				 "{\"error\":"
				 "\"OpenThread instance not available\"}");
			goto send_response;
		}

		openthread_mutex_lock();
		role = otThreadGetDeviceRole(instance);
		rloc16 = otThreadGetRloc16(instance);
		ext_addr = otLinkGetExtendedAddress(instance);
		openthread_mutex_unlock();

		written = snprintf(ptr, remaining, "{\"status\":\"ok\",\"nodes\":[");
		ptr += written;
		remaining -= written;

		first_node = true;
		/* Add self node */
		role_str = "unknown";
		color = "#cccccc";
		shape = "box";
		switch (role) {
		case OT_DEVICE_ROLE_DISABLED:
			role_str = "disabled";
			color = "#999999";
			break;
		case OT_DEVICE_ROLE_DETACHED:
			role_str = "detached";
			color = "#ff9999";
			break;
		case OT_DEVICE_ROLE_CHILD:
			role_str = "child";
			color = "#99ccff";
			shape = "ellipse";
			break;
		case OT_DEVICE_ROLE_ROUTER:
			role_str = "router";
			color = "#99ff99";
			shape = "diamond";
			break;
		case OT_DEVICE_ROLE_LEADER:
			role_str = "leader";
			color = "#ffcc00";
			shape = "star";
			break;
		default:
			break;
		}

		snprintf(self_label, sizeof(self_label),
			 "BR\\n0x%04x\\n%s", rloc16, role_str);
		struct node_display_info self_info = {
			.ext_addr = ext_addr->m8,
			.rloc16 = rloc16,
			.label = self_label,
			.color = color,
			.shape = shape,
			.size = 30
		};

		add_node_to_json_direct(&ptr, &remaining, &self_info, &first_node);

		/* Get neighbor table */
		openthread_mutex_lock();
		iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
		while (otThreadGetNextNeighborInfo(instance, &iterator,
						   &neighbor_info) == OT_ERROR_NONE) {
			neighbor_role = "unknown";
			neighbor_color = "#cccccc";
			neighbor_shape = "ellipse";
			if (neighbor_info.mIsChild) {
				neighbor_role = "child";
				neighbor_color = "#99ccff";
				neighbor_shape = "dot";
			} else {
				neighbor_role = "router";
				neighbor_color = "#99ff99";
				neighbor_shape = "diamond";
			}
			snprintf(neighbor_label, sizeof(neighbor_label),
				 "%s\\n0x%04x\\nLQI:%d",
				 neighbor_role,
				 neighbor_info.mRloc16,
				 neighbor_info.mLinkQualityIn);
			struct node_display_info neighbor_node_info = {
				.ext_addr = neighbor_info.mExtAddress.m8,
				.rloc16 = neighbor_info.mRloc16,
				.label = neighbor_label,
				.color = neighbor_color,
				.shape = neighbor_shape,
				.size = 20
			};

			add_node_to_json_direct(&ptr, &remaining,
						&neighbor_node_info, &first_node);
		}
		openthread_mutex_unlock();
		written = snprintf(ptr, remaining, "],\"edges\":[");
		ptr += written;
		remaining -= written;

		first_edge = true;
		openthread_mutex_lock();
		iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
		while (otThreadGetNextNeighborInfo(instance, &iterator,
						   &neighbor_info) == OT_ERROR_NONE) {
			add_edge_to_json_direct(&ptr, &remaining,
						neighbor_info.mExtAddress.m8,
						ext_addr->m8,
						neighbor_info.mLinkQualityIn,
						&first_edge);
		}
		openthread_mutex_unlock();

		snprintf(ptr, remaining, "],\"role\":\"%s\"}", role_str);

send_response:
		response_ctx->body = (const uint8_t *)json_response;
		response_ctx->body_len = strlen(json_response);
		response_ctx->final_chunk = true;
		response_ctx->status = HTTP_200_OK;
	}

	return 0;
}

/* Structure to hold parsed network parameters */
struct network_params {
	char network_name[OT_NETWORK_NAME_MAX_SIZE + 1];
	char pan_id_str[16];
	char extpanid_str[65];
	char channel_str[16];
	char network_key_str[65];
	bool has_name;
	bool has_pan;
	bool has_extpanid;
	bool has_channel;
	bool has_key;
};

/* Helper to receive POST data chunk */
static void receive_post_data_chunk(const struct http_request_ctx *request_ctx)
{
	size_t remaining = sizeof(post_data_buffer) - post_data_len - 1;
	size_t to_copy = request_ctx->data_len < remaining ?
			 request_ctx->data_len : remaining;

	memcpy(post_data_buffer + post_data_len, request_ctx->data, to_copy);
	post_data_len += to_copy;
	post_data_buffer[post_data_len] = '\0';
}

/* Helper to parse network parameters from POST data */
static void parse_network_params(const char *data, bool is_json,
				  struct network_params *params)
{
	memset(params, 0, sizeof(*params));

	if (is_json) {
		LOG_DBG("Detected JSON format");
		params->has_name = get_json_string_value(data, "network_name",
							 params->network_name,
							 sizeof(params->network_name));
		params->has_pan = get_json_string_value(data, "pan_id",
							params->pan_id_str,
							sizeof(params->pan_id_str));
		params->has_extpanid = get_json_string_value(data, "extpanid",
							     params->extpanid_str,
							     sizeof(params->extpanid_str));
		params->has_channel = get_json_string_value(data, "channel",
							    params->channel_str,
							    sizeof(params->channel_str));
		params->has_key = get_json_string_value(data, "network_key",
							params->network_key_str,
							sizeof(params->network_key_str));
	} else {
		LOG_DBG("Detected URL-encoded format");
		params->has_name = get_url_param(data, "network_name",
						 params->network_name,
						 sizeof(params->network_name));
		params->has_pan = get_url_param(data, "pan_id",
						params->pan_id_str,
						sizeof(params->pan_id_str));
		params->has_extpanid = get_url_param(data, "extpanid",
						     params->extpanid_str,
						     sizeof(params->extpanid_str));
		params->has_channel = get_url_param(data, "channel",
						    params->channel_str,
						    sizeof(params->channel_str));
		params->has_key = get_url_param(data, "network_key",
						params->network_key_str,
						sizeof(params->network_key_str));
	}

	LOG_DBG("===== PARSED PARAMETERS =====");
	LOG_DBG("has_name: %d, value: '%s'", params->has_name, params->network_name);
	LOG_DBG("has_pan: %d, value: '%s'", params->has_pan, params->pan_id_str);
	LOG_DBG("has_extpanid: %d, value: '%s'", params->has_extpanid, params->extpanid_str);
	LOG_DBG("has_channel: %d, value: '%s'", params->has_channel, params->channel_str);
	LOG_DBG("has_key: %d, value: '%s'", params->has_key, params->network_key_str);
}

/* Helper to set network name in dataset */
static void set_network_name(otOperationalDataset *dataset, const char *name, bool has_name)
{
	size_t name_len;

	if (!has_name || strlen(name) == 0) {
		LOG_WRN("Network name NOT set");
		return;
	}
	name_len = strlen(name);
	if (name_len > OT_NETWORK_NAME_MAX_SIZE) {
		name_len = OT_NETWORK_NAME_MAX_SIZE;
	}

	memcpy(dataset->mNetworkName.m8, name, name_len);
	if (name_len < OT_NETWORK_NAME_MAX_SIZE) {
		dataset->mNetworkName.m8[name_len] = '\0';
	}
	dataset->mComponents.mIsNetworkNamePresent = true;
	LOG_DBG("Set network name: '%s' (len=%d)", name, name_len);
}

/* Helper to set PAN ID in dataset */
static void set_pan_id(otOperationalDataset *dataset, const char *pan_str, bool has_pan)
{
	if (!has_pan || strlen(pan_str) == 0) {
		LOG_WRN("PAN ID NOT set");
		return;
	}

	dataset->mPanId = (uint16_t)strtol(pan_str, NULL, 0);
	dataset->mComponents.mIsPanIdPresent = true;
	LOG_DBG("Set PAN ID: 0x%04x (from '%s')", dataset->mPanId, pan_str);
}

/* Helper to set Extended PAN ID in dataset */
static void set_extpanid(otOperationalDataset *dataset, const char *extpanid_str,
			 bool has_extpanid)
{
	size_t hex_len;
	int ret;

	if (!has_extpanid || strlen(extpanid_str) == 0) {
		LOG_WRN("Extended PAN ID NOT set");
		return;
	}
	hex_len = strlen(extpanid_str);

	ret = hex2bin(extpanid_str, hex_len,
					dataset->mExtendedPanId.m8,
					OT_EXT_PAN_ID_SIZE);
	if (ret == OT_EXT_PAN_ID_SIZE) {
		dataset->mComponents.mIsExtendedPanIdPresent = true;
		LOG_DBG("Set Extended PAN ID (from '%s', converted %d bytes)",
			extpanid_str, hex_len);
		LOG_HEXDUMP_DBG(dataset->mExtendedPanId.m8,
				OT_EXT_PAN_ID_SIZE, "  ExtPanID:");
	} else {
		LOG_WRN("Invalid Extended PAN ID length: %d (expected %d)",
			hex_len, OT_EXT_PAN_ID_SIZE);
	}
}

/* Helper to set channel in dataset */
static void set_channel(otOperationalDataset *dataset, const char *channel_str,
			bool has_channel)
{
	if (!has_channel || strlen(channel_str) == 0) {
		LOG_WRN("Channel NOT set");
		return;
	}

	dataset->mChannel = (uint16_t)atoi(channel_str);
	dataset->mComponents.mIsChannelPresent = true;
	LOG_DBG("Set channel: %u (from '%s')", dataset->mChannel, channel_str);
}

/* Helper to set network key in dataset */
static void set_network_key(otOperationalDataset *dataset, const char *key_str,
			    bool has_key)
{
	size_t key_len;
	int ret;

	if (!has_key || strlen(key_str) == 0) {
		LOG_WRN("Network key NOT set");
		return;
	}
	key_len = strlen(key_str);

	ret = hex2bin(key_str, key_len,
				   dataset->mNetworkKey.m8,
				   OT_NETWORK_KEY_SIZE);
	if (ret == OT_NETWORK_KEY_SIZE) {
		dataset->mComponents.mIsNetworkKeyPresent = true;
		LOG_DBG("Set network key (from '%s', converted %d bytes)",
			key_str, key_len);
		LOG_HEXDUMP_DBG(dataset->mNetworkKey.m8,
				OT_NETWORK_KEY_SIZE, "  Network Key:");
	} else {
		LOG_WRN("Invalid network key length: %d (expected %d)",
			key_len, OT_NETWORK_KEY_SIZE);
	}
}

/* Helper to apply user parameters to dataset */
static void apply_user_params(otOperationalDataset *dataset,
			       const struct network_params *params)
{
	LOG_DBG("Creating dataset with UI values and random generated values");

	set_network_name(dataset, params->network_name, params->has_name);
	set_pan_id(dataset, params->pan_id_str, params->has_pan);
	set_extpanid(dataset, params->extpanid_str, params->has_extpanid);
	set_channel(dataset, params->channel_str, params->has_channel);
	set_network_key(dataset, params->network_key_str, params->has_key);
}

/* Helper to generate random dataset values */
static void generate_random_values(otOperationalDataset *dataset)
{
	LOG_DBG("===== GENERATING RANDOM VALUES =====");

	openthread_mutex_lock();
	otPlatCryptoRandomGet(dataset->mPskc.m8, OT_PSKC_MAX_SIZE);
	openthread_mutex_unlock();
	dataset->mComponents.mIsPskcPresent = true;
	LOG_HEXDUMP_DBG(dataset->mPskc.m8, OT_PSKC_MAX_SIZE, "PSKc:");

	dataset->mMeshLocalPrefix.m8[0] = 0xfd;
	openthread_mutex_lock();
	otPlatCryptoRandomGet(&dataset->mMeshLocalPrefix.m8[1], 7);
	openthread_mutex_unlock();
	dataset->mComponents.mIsMeshLocalPrefixPresent = true;
	LOG_HEXDUMP_DBG(dataset->mMeshLocalPrefix.m8,
			OT_IP6_PREFIX_SIZE, "Mesh Local Prefix:");

	dataset->mSecurityPolicy.mRotationTime = 672;
	dataset->mSecurityPolicy.mObtainNetworkKeyEnabled = true;
	dataset->mSecurityPolicy.mNativeCommissioningEnabled = true;
	dataset->mSecurityPolicy.mRoutersEnabled = true;
	dataset->mSecurityPolicy.mExternalCommissioningEnabled = true;
	dataset->mSecurityPolicy.mCommercialCommissioningEnabled = false;
	dataset->mSecurityPolicy.mAutonomousEnrollmentEnabled = false;
	dataset->mSecurityPolicy.mNetworkKeyProvisioningEnabled = false;
	dataset->mSecurityPolicy.mNonCcmRoutersEnabled = false;
	dataset->mComponents.mIsSecurityPolicyPresent = true;

	dataset->mActiveTimestamp.mSeconds = 1;
	dataset->mActiveTimestamp.mTicks = 0;
	dataset->mActiveTimestamp.mAuthoritative = false;
	dataset->mComponents.mIsActiveTimestampPresent = true;

	dataset->mChannelMask = 0x07FFF800;
	dataset->mComponents.mIsChannelMaskPresent = true;
}

/* Helper to log complete dataset */
static void log_dataset(const otOperationalDataset *dataset)
{
	LOG_DBG("===== COMPLETE DATASET =====");
	LOG_DBG("Network Name: '%s'", dataset->mNetworkName.m8);
	LOG_DBG("Channel: %d", dataset->mChannel);
	LOG_DBG("PAN ID: 0x%04x", dataset->mPanId);
	LOG_HEXDUMP_DBG(dataset->mExtendedPanId.m8,
			OT_EXT_PAN_ID_SIZE, "Extended PAN ID:");
	LOG_HEXDUMP_DBG(dataset->mNetworkKey.m8,
			OT_NETWORK_KEY_SIZE, "Network Key:");
}

/* Helper to process network configuration */
static otError process_network_config(otInstance *instance,
				      const struct network_params *params,
				      char *response, size_t response_size)
{
	otOperationalDataset dataset;
	otOperationalDatasetTlvs dataset_tlvs;
	otError error;

	memset(&dataset, 0, sizeof(dataset));

	apply_user_params(&dataset, params);
	generate_random_values(&dataset);
	log_dataset(&dataset);

	LOG_DBG("Dataset created with UI values and generated random values");

	openthread_mutex_lock();
	otDatasetConvertToTlvs(&dataset, &dataset_tlvs);
	LOG_DBG("otDatasetConvertToTlvs Done");
	error = otDatasetSetActive(instance, &dataset);
	openthread_mutex_unlock();

	if (error != OT_ERROR_NONE) {
		snprintf(response, response_size,
			 "{\"error\":\"Failed to set dataset\","
			 "\"error_code\":%d}",
			 error);
	} else {
		snprintf(response, response_size,
			 "{\"status\":\"success\","
			 "\"message\":"
			 "\"Network configuration created successfully\"}");
		LOG_DBG("Dataset configured and activated with otDatasetSetActive");
	}

	return error;
}

/* Helper to handle final POST data */
static void handle_final_data(const struct http_request_ctx *request_ctx,
			       char *response, size_t response_size)
{
	otInstance *instance;
	struct network_params params;
	bool is_json;

	if (request_ctx->data_len > 0) {
		LOG_DBG("Receiving final data: %d bytes (total before: %d)",
			request_ctx->data_len, post_data_len);
		receive_post_data_chunk(request_ctx);
	}

	instance = get_ot_instance();

	if (!instance) {
		snprintf(response, response_size,
			 "{\"error\":"
			 "\"OpenThread instance not available\"}");
		return;
	}

	LOG_DBG("Received POST data (%d bytes): %s", post_data_len, post_data_buffer);
	LOG_HEXDUMP_DBG(post_data_buffer, post_data_len, "POST data (hex):");

	if (post_data_len == 0) {
		LOG_ERR("No POST data received");
		snprintf(response, response_size,
			 "{\"error\":\"No data received\"}");
		return;
	}

	is_json = (post_data_buffer[0] == '{');
	parse_network_params(post_data_buffer, is_json, &params);
	process_network_config(instance, &params, response, response_size);
}

/* REST API: POST /api/network/config */
int rest_api_network_config_handler(struct http_client_ctx *client,
				     enum http_transaction_status status,
				     const struct http_request_ctx *request_ctx,
				     struct http_response_ctx *response_ctx,
				     void *user_data)
{
	static char json_response[512];

	LOG_INF("Network config API called, status: %d", status);

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		post_data_len = 0;
		memset(post_data_buffer, 0, sizeof(post_data_buffer));
		return 0;
	}

	if (status == HTTP_SERVER_REQUEST_DATA_MORE) {
		LOG_DBG("Receiving more data: %d bytes (total so far: %d)",
			request_ctx->data_len, post_data_len);
		receive_post_data_chunk(request_ctx);
		return 0;
	}

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		handle_final_data(request_ctx, json_response, sizeof(json_response));

		post_data_len = 0;
		memset(post_data_buffer, 0, sizeof(post_data_buffer));
		response_ctx->body = (const uint8_t *)json_response;
		response_ctx->body_len = strlen(json_response);
		response_ctx->final_chunk = true;
		response_ctx->status = HTTP_200_OK;
	}

	return 0;
}

/* Helper to receive POST data chunk for node info */
static void receive_node_info_chunk(const struct http_request_ctx *request_ctx,
				     char *buffer, size_t *len, size_t buffer_size)
{
	size_t remaining = buffer_size - *len - 1;
	size_t to_copy = request_ctx->data_len < remaining ?
			 request_ctx->data_len : remaining;

	memcpy(buffer + *len, request_ctx->data, to_copy);
	*len += to_copy;
	buffer[*len] = '\0';
}

/* Helper to get role string from device role */
static const char *get_role_string(otDeviceRole role)
{
	switch (role) {
	case OT_DEVICE_ROLE_DISABLED:
		return "disabled";
	case OT_DEVICE_ROLE_DETACHED:
		return "detached";
	case OT_DEVICE_ROLE_CHILD:
		return "child";
	case OT_DEVICE_ROLE_ROUTER:
		return "router";
	case OT_DEVICE_ROLE_LEADER:
		return "leader";
	default:
		return "unknown";
	}
}

/* Helper to collect IPv6 addresses */
static void collect_ipv6_addresses(otInstance *instance, char *addresses,
				    size_t addresses_size)
{
	char addr_str[64];
	bool first_addr = true;

	const otNetifAddress *addr;

	strncat(addresses, "[", addresses_size - strlen(addresses) - 1);

	openthread_mutex_lock();
	addr = otIp6GetUnicastAddresses(instance);
	while (addr) {
		snprintf(addr_str, sizeof(addr_str),
			 "%02x%02x:%02x%02x:%02x%02x:"
			 "%02x%02x:%02x%02x:%02x%02x:"
			 "%02x%02x:%02x%02x",
			 addr->mAddress.mFields.m8[0],
			 addr->mAddress.mFields.m8[1],
			 addr->mAddress.mFields.m8[2],
			 addr->mAddress.mFields.m8[3],
			 addr->mAddress.mFields.m8[4],
			 addr->mAddress.mFields.m8[5],
			 addr->mAddress.mFields.m8[6],
			 addr->mAddress.mFields.m8[7],
			 addr->mAddress.mFields.m8[8],
			 addr->mAddress.mFields.m8[9],
			 addr->mAddress.mFields.m8[10],
			 addr->mAddress.mFields.m8[11],
			 addr->mAddress.mFields.m8[12],
			 addr->mAddress.mFields.m8[13],
			 addr->mAddress.mFields.m8[14],
			 addr->mAddress.mFields.m8[15]);

		if (!first_addr) {
			strncat(addresses, ",",
				addresses_size - strlen(addresses) - 1);
		}
		snprintf(addresses + strlen(addresses),
			 addresses_size - strlen(addresses),
			 "\"%s\"", addr_str);
		first_addr = false;
		addr = addr->mNext;
	}
	openthread_mutex_unlock();

	strncat(addresses, "]", addresses_size - strlen(addresses) - 1);
}

/* Helper to build Border Router info response */
static void build_br_info_response(otInstance *instance, uint16_t rloc16,
				    char *response, size_t response_size)
{
	const otExtAddress *ext_addr;
	otDeviceRole role;
	const char *role_str;
	char ipv6_addresses[1024] = {0};

	openthread_mutex_lock();
	ext_addr = otLinkGetExtendedAddress(instance);
	role = otThreadGetDeviceRole(instance);
	openthread_mutex_unlock();

	role_str = get_role_string(role);
	collect_ipv6_addresses(instance, ipv6_addresses, sizeof(ipv6_addresses));

	snprintf(response, response_size,
		 "{\"status\":\"ok\","
		 "\"rloc16\":\"0x%04x\","
		 "\"role\":\"%s\","
		 "\"ext_address\":"
		 "\"%02x%02x%02x%02x%02x%02x%02x%02x\","
		 "\"link_quality_in\":255,"
		 "\"age\":0,"
		 "\"ipv6_addresses\":%s}",
		 rloc16,
		 role_str,
		 ext_addr->m8[0], ext_addr->m8[1],
		 ext_addr->m8[2], ext_addr->m8[3],
		 ext_addr->m8[4], ext_addr->m8[5],
		 ext_addr->m8[6], ext_addr->m8[7],
		 ipv6_addresses);
}

/* Helper to find neighbor by RLOC16 */
static bool find_neighbor(otInstance *instance, uint16_t rloc16,
			  otNeighborInfo *neighbor_info)
{
	otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
	bool found;

	openthread_mutex_lock();
	iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
	found = false;
	while (otThreadGetNextNeighborInfo(instance, &iterator, neighbor_info) ==
	       OT_ERROR_NONE) {
		if (neighbor_info->mRloc16 == rloc16) {
			found = true;
			break;
		}
	}
	openthread_mutex_unlock();

	return found;
}

/* Helper to construct mesh-local address */
static bool construct_mesh_local_addr(otInstance *instance, uint16_t rloc16,
				       char *addr, size_t addr_size)
{
	const otMeshLocalPrefix *ml_prefix;

	openthread_mutex_lock();
	ml_prefix = otThreadGetMeshLocalPrefix(instance);
	openthread_mutex_unlock();

	if (ml_prefix == NULL) {
		return false;
	}

	snprintf(addr, addr_size,
		 "fd%02x:%02x%02x:%02x%02x:0000:0000:00ff:fe00:%04x",
		 ml_prefix->m8[1], ml_prefix->m8[2], ml_prefix->m8[3],
		 ml_prefix->m8[4], ml_prefix->m8[5], rloc16);

	return true;
}

/* Helper to build neighbor info response */
static void build_neighbor_info_response(const otNeighborInfo *neighbor_info,
					  const char *link_local,
					  char *response, size_t response_size)
{
	const char *role = neighbor_info->mIsChild ? "child" : "router";

	snprintf(response, response_size,
		 "{\"status\":\"ok\","
		 "\"rloc16\":\"0x%04x\","
		 "\"role\":\"%s\","
		 "\"ext_address\":"
		 "\"%02x%02x%02x%02x%02x%02x%02x%02x\","
		 "\"link_quality_in\":%d,"
		 "\"age\":%u,"
		 "\"ipv6_addresses\":[\"%s\"]}",
		 neighbor_info->mRloc16,
		 role,
		 neighbor_info->mExtAddress.m8[0],
		 neighbor_info->mExtAddress.m8[1],
		 neighbor_info->mExtAddress.m8[2],
		 neighbor_info->mExtAddress.m8[3],
		 neighbor_info->mExtAddress.m8[4],
		 neighbor_info->mExtAddress.m8[5],
		 neighbor_info->mExtAddress.m8[6],
		 neighbor_info->mExtAddress.m8[7],
		 neighbor_info->mLinkQualityIn,
		 neighbor_info->mAge,
		 link_local);
}

/* Helper to parse RLOC16 from POST data */
static bool parse_rloc16(const char *data, uint16_t *rloc16)
{
	const char *rloc_param = strstr(data, "rloc16=");

	if (!rloc_param) {
		return false;
	}

	*rloc16 = (uint16_t)strtol(rloc_param + 7, NULL, 0);
	return true;
}

/* Helper to handle node info request */
static void handle_node_info_request(otInstance *instance, const char *post_data,
				      char *response, size_t response_size)
{
	uint16_t rloc16;
	uint16_t self_rloc16;
	char link_local[64];
	otNeighborInfo neighbor_info;

	if (!parse_rloc16(post_data, &rloc16)) {
		snprintf(response, response_size,
			 "{\"error\":\"Missing rloc16 parameter\"}");
		return;
	}

	LOG_DBG("Looking for node with RLOC16: 0x%04x", rloc16);

	/* Check if it's the Border Router itself */
	openthread_mutex_lock();
	self_rloc16 = otThreadGetRloc16(instance);
	openthread_mutex_unlock();

	if (rloc16 == self_rloc16) {
		build_br_info_response(instance, rloc16, response, response_size);
		return;
	}

	/* Search in neighbor table */
	if (!find_neighbor(instance, rloc16, &neighbor_info)) {
		snprintf(response, response_size,
			 "{\"error\":\"Node not found\","
			 "\"rloc16\":\"0x%04x\"}", rloc16);
		return;
	}

	/* Construct mesh-local address */
	if (!construct_mesh_local_addr(instance, rloc16, link_local,
				       sizeof(link_local))) {
		snprintf(response, response_size,
			 "{\"error\":"
			 "\"Failed to get mesh-local prefix\"}");
		return;
	}

	build_neighbor_info_response(&neighbor_info, link_local,
				      response, response_size);
}

/* Helper to handle final node info data */
static void handle_final_node_info(const struct http_request_ctx *request_ctx,
				    char *buffer, size_t *len,
				    char *response, size_t response_size)
{
	otInstance *instance;

	if (request_ctx->data_len > 0) {
		receive_node_info_chunk(request_ctx, buffer, len, 256);
	}

	instance = get_ot_instance();
	if (!instance) {
		snprintf(response, response_size,
			 "{\"error\":"
			 "\"OpenThread instance not available\"}");
		return;
	}

	LOG_DBG("Received POST data: %s", buffer);
	handle_node_info_request(instance, buffer, response, response_size);
}

/* REST API: POST /api/node/info: rloc16=0xXXXX */
int rest_api_node_info_handler(struct http_client_ctx *client,
				enum http_transaction_status status,
				const struct http_request_ctx *request_ctx,
				struct http_response_ctx *response_ctx,
				void *user_data)
{
	static char json_response[2048];
	static char post_data_buffer_local[256];
	static size_t post_data_len_local;

	LOG_INF("Node info API called, status: %d", status);

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		post_data_len_local = 0;
		memset(post_data_buffer_local, 0,
		       sizeof(post_data_buffer_local));
		return 0;
	}

	if (status == HTTP_SERVER_REQUEST_DATA_MORE) {
		receive_node_info_chunk(request_ctx, post_data_buffer_local,
					&post_data_len_local,
					sizeof(post_data_buffer_local));
		return 0;
	}

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		handle_final_node_info(request_ctx, post_data_buffer_local,
					&post_data_len_local, json_response,
					sizeof(json_response));

		post_data_len_local = 0;
		memset(post_data_buffer_local, 0,
		       sizeof(post_data_buffer_local));
		response_ctx->body = (const uint8_t *)json_response;
		response_ctx->body_len = strlen(json_response);
		response_ctx->final_chunk = true;
		response_ctx->status = HTTP_200_OK;
	}

	return 0;
}

/* REST API: GET /api/thread/start */
int rest_api_thread_start_handler(struct http_client_ctx *client,
				   enum http_transaction_status status,
				   const struct http_request_ctx *request_ctx,
				   struct http_response_ctx *response_ctx,
				   void *user_data)
{
	static char json_response[256];
	otInstance *instance;
	otError error;

	LOG_INF("Thread start API called, status: %d", status);
	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		instance = get_ot_instance();
		if (!instance) {
			snprintf(json_response, sizeof(json_response),
				"{\"error\":"
				"\"OpenThread instance not available\"}");
		} else {
			/* First, enable the IPv6 interface */
			openthread_mutex_lock();
			error = otIp6SetEnabled(instance, true);
			openthread_mutex_unlock();
			if (error != OT_ERROR_NONE) {
				LOG_ERR("Failed to enable IPv6 interface: %d",
					error);
				snprintf(json_response,
					 sizeof(json_response),
					"{\"error\":"
					"\"Failed to enable IPv6 "
					"interface\","
					"\"error_code\":%d}",
					 error);
				goto send_response;
			}

			LOG_DBG("IPv6 interface enabled");

			/* Small delay to allow interface to stabilize */
			k_sleep(K_MSEC(100));

			/* Then enable Thread */
			openthread_mutex_lock();
			error = otThreadSetEnabled(instance, true);
			openthread_mutex_unlock();
			if (error != OT_ERROR_NONE) {
				LOG_ERR("Failed to start Thread: %d", error);
				snprintf(json_response,
					 sizeof(json_response),
					"{\"error\":"
					"\"Failed to start Thread\","
					"\"error_code\":%d}", error);
			} else {
				LOG_DBG("Thread network started successfully");
				snprintf(json_response,
					 sizeof(json_response),
					"{\"status\":\"success\","
					"\"message\":"
					"\"Thread network started\"}");
			}
		}

send_response:
		response_ctx->body = (const uint8_t *)json_response;
		response_ctx->body_len = strlen(json_response);
		response_ctx->final_chunk = true;
		response_ctx->status = HTTP_200_OK;
	}

	return 0;
}

/* REST API: GET /api/thread/stop */
int rest_api_thread_stop_handler(struct http_client_ctx *client,
				  enum http_transaction_status status,
				  const struct http_request_ctx *request_ctx,
				  struct http_response_ctx *response_ctx,
				  void *user_data)
{
	static char json_response[256];
	otInstance *instance;
	otError error;

	LOG_INF("Thread stop API called, status: %d", status);

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		instance = get_ot_instance();
		if (!instance) {
			snprintf(json_response, sizeof(json_response),
				"{\"error\":"
				"\"OpenThread instance not available\"}");
		} else {
			/* First disable Thread */
			openthread_mutex_lock();
			error = otThreadSetEnabled(instance, false);
			openthread_mutex_unlock();
			if (error != OT_ERROR_NONE) {
				LOG_ERR("Failed to stop Thread: %d", error);
				snprintf(json_response,
					 sizeof(json_response),
					"{\"error\":"
					"\"Failed to stop Thread\","
					"\"error_code\":%d}", error);
				goto send_response;
			}

			LOG_DBG("Thread network stopped");

			/* Small delay */
			k_sleep(K_MSEC(100));

			/* Then disable IPv6 interface */
			openthread_mutex_lock();
			error = otIp6SetEnabled(instance, false);
			openthread_mutex_unlock();
			if (error != OT_ERROR_NONE) {
				LOG_ERR("Failed to disable IPv6 interface: %d",
					error);
				snprintf(json_response,
					 sizeof(json_response),
					"{\"error\":"
					"\"Failed to disable IPv6 "
					"interface\","
					"\"error_code\":%d}",
					 error);
			} else {
				LOG_DBG("IPv6 interface disabled "
					"successfully");
				snprintf(json_response,
					 sizeof(json_response),
					"{\"status\":\"success\","
					"\"message\":"
					"\"Thread network stopped\"}");
			}
		}

send_response:
		response_ctx->body = (const uint8_t *)json_response;
		response_ctx->body_len = strlen(json_response);
		response_ctx->final_chunk = true;
		response_ctx->status = HTTP_200_OK;
	}

	return 0;
}

struct diag_result {
	char response[4096];
	bool received;
	k_timeout_t timeout;
};

static struct diag_result current_diag_result = {0};
static K_MUTEX_DEFINE(diag_mutex);

/* Structure to pass diagnostic context */
struct diag_context {
	uint8_t tlvs[32];
	uint8_t count;
};

/* Helper to format diagnostic mode TLV */
static int format_diag_mode(char *ptr, size_t remaining,
				const otLinkModeConfig *mode)
{
	return snprintf(ptr, remaining,
			"\"mode\":{\"rx_on_idle\":%s,"
			"\"device_type\":%s,"
			"\"network_data\":%s}",
			mode->mRxOnWhenIdle ? "true" : "false",
			mode->mDeviceType ? "\"ftd\"" : "\"mtd\"",
			mode->mNetworkData ? "\"full\"" : "\"stable\"");
}

/* Helper to format diagnostic connectivity TLV */
static int format_diag_connectivity(char *ptr, size_t remaining,
					 const otNetworkDiagConnectivity *conn)
{
	return snprintf(ptr, remaining,
			"\"connectivity\":{"
			"\"parent_priority\":%d,"
			"\"link_quality_3\":%d,"
			"\"link_quality_2\":%d,"
			"\"link_quality_1\":%d,"
			"\"leader_cost\":%d,"
			"\"id_sequence\":%d,"
			"\"active_routers\":%d,"
			"\"sed_buffer_size\":%d,"
			"\"sed_datagram_count\":%d}",
			conn->mParentPriority,
			conn->mLinkQuality3,
			conn->mLinkQuality2,
			conn->mLinkQuality1,
			conn->mLeaderCost,
			conn->mIdSequence,
			conn->mActiveRouters,
			conn->mSedBufferSize,
			conn->mSedDatagramCount);
}

/* Helper to format diagnostic route TLV */
static int format_diag_route(char *ptr, size_t remaining,
				  const otNetworkDiagRoute *route)
{
	int written;
	char *start_ptr = ptr;

	const otNetworkDiagRouteData *rd;

	written = snprintf(ptr, remaining,
			   "\"route\":{\"id_sequence\":%d,"
			   "\"route_data\":[",
			   route->mIdSequence);
	ptr += written;
	remaining -= written;

	for (uint16_t i = 0; i < route->mRouteCount; i++) {
		rd = &route->mRouteData[i];

		if (i > 0) {
			written = snprintf(ptr, remaining, ",");
			ptr += written;
			remaining -= written;
		}
		written = snprintf(ptr, remaining,
				   "{\"router_id\":%d,"
				   "\"link_quality_out\":%d,"
				   "\"link_quality_in\":%d,"
				   "\"route_cost\":%d}",
				   rd->mRouterId,
				   rd->mLinkQualityOut,
				   rd->mLinkQualityIn,
				   rd->mRouteCost);
		ptr += written;
		remaining -= written;
	}
	written = snprintf(ptr, remaining, "]}");
	ptr += written;

	return ptr - start_ptr;
}

/* Helper to format diagnostic leader data TLV */
static int format_diag_leader_data(char *ptr, size_t remaining,
					const otLeaderData *leader)
{
	return snprintf(ptr, remaining,
			"\"leader_data\":{"
			"\"partition_id\":%u,"
			"\"weighting\":%d,"
			"\"data_version\":%d,"
			"\"stable_data_version\":%d,"
			"\"leader_router_id\":%d}",
			leader->mPartitionId,
			leader->mWeighting,
			leader->mDataVersion,
			leader->mStableDataVersion,
			leader->mLeaderRouterId);
}

/* Helper to format diagnostic child table TLV */
static int format_diag_child_table(char *ptr, size_t remaining,
					const otNetworkDiagChildEntry *table,
					uint16_t count)
{
	char *start_ptr = ptr;
	int written;

	const otNetworkDiagChildEntry *child;

	written = snprintf(ptr, remaining, "\"child_table\":[");
	ptr += written;
	remaining -= written;

	for (uint16_t i = 0; i < count; i++) {
		child = &table[i];

		if (i > 0) {
			written = snprintf(ptr, remaining, ",");
			ptr += written;
			remaining -= written;
		}
		written = snprintf(ptr, remaining,
				   "{\"timeout\":%u,"
				   "\"link_quality\":%d,"
				   "\"child_id\":%d,"
				   "\"rx_on_idle\":%s,"
				   "\"ftd\":%s}",
				   child->mTimeout,
				   child->mLinkQuality,
				   child->mChildId,
				   child->mMode.mRxOnWhenIdle ?
				   "true" : "false",
				   child->mMode.mDeviceType ?
				   "true" : "false");
		ptr += written;
		remaining -= written;
	}
	written = snprintf(ptr, remaining, "]");
	ptr += written;

	return ptr - start_ptr;
}

/* Helper to format diagnostic MAC counters TLV */
static int format_diag_mac_counters(char *ptr, size_t remaining,
					 const otNetworkDiagMacCounters *mac)
{
	return snprintf(ptr, remaining,
			"\"mac_counters\":{"
			"\"if_in_errors\":%u,"
			"\"if_out_errors\":%u,"
			"\"if_in_ucast_pkts\":%u,"
			"\"if_in_broadcast_pkts\":%u,"
			"\"if_out_ucast_pkts\":%u,"
			"\"if_out_broadcast_pkts\":%u}",
			mac->mIfInErrors,
			mac->mIfOutErrors,
			mac->mIfInUcastPkts,
			mac->mIfInBroadcastPkts,
			mac->mIfOutUcastPkts,
			mac->mIfOutBroadcastPkts);
}

/* Helper to format diagnostic MLE counters TLV */
static int format_diag_mle_counters(char *ptr, size_t remaining,
					 const otNetworkDiagMleCounters *mle)
{
	return snprintf(ptr, remaining,
			"\"mle_counters\":{"
			"\"disabled_role\":%u,"
			"\"detached_role\":%u,"
			"\"child_role\":%u,"
			"\"router_role\":%u,"
			"\"leader_role\":%u,"
			"\"attach_attempts\":%u,"
			"\"partition_id_changes\":%u,"
			"\"better_partition_attach_attempts\":%u,"
			"\"parent_changes\":%u}",
			mle->mDisabledRole,
			mle->mDetachedRole,
			mle->mChildRole,
			mle->mRouterRole,
			mle->mLeaderRole,
			mle->mAttachAttempts,
			mle->mPartitionIdChanges,
			mle->mBetterPartitionAttachAttempts,
			mle->mParentChanges);
}

/* Helper to format diagnostic IPv6 address list TLV */
static int format_diag_ip6_addr_list(char *ptr, size_t remaining,
					  const otIp6Address *list,
					  uint16_t count)
{
	char *start_ptr = ptr;
	int written;
	const otIp6Address *addr;

	written = snprintf(ptr, remaining, "\"ipv6_addresses\":[");
	ptr += written;
	remaining -= written;

	for (uint16_t i = 0; i < count; i++) {
		addr = &list[i];

		if (i > 0) {
			written = snprintf(ptr, remaining, ",");
			ptr += written;
			remaining -= written;
		}
		written = snprintf(ptr, remaining,
				   "\"%02x%02x:%02x%02x:%02x%02x:"
				   "%02x%02x:%02x%02x:%02x%02x:"
				   "%02x%02x:%02x%02x\"",
				   addr->mFields.m8[0],
				   addr->mFields.m8[1],
				   addr->mFields.m8[2],
				   addr->mFields.m8[3],
				   addr->mFields.m8[4],
				   addr->mFields.m8[5],
				   addr->mFields.m8[6],
				   addr->mFields.m8[7],
				   addr->mFields.m8[8],
				   addr->mFields.m8[9],
				   addr->mFields.m8[10],
				   addr->mFields.m8[11],
				   addr->mFields.m8[12],
				   addr->mFields.m8[13],
				   addr->mFields.m8[14],
				   addr->mFields.m8[15]);
		ptr += written;
		remaining -= written;
	}
	written = snprintf(ptr, remaining, "]");
	ptr += written;

	return ptr - start_ptr;
}
/* Helper to add missing TLVs by category */
static void add_missing_tlvs(char **ptr, size_t *remaining, bool *first,
				 const uint8_t *requested_tlvs,
				 uint8_t requested_count,
				 const uint8_t *received_tlvs,
				 uint8_t received_count)
{
	for (uint8_t i = 0; i < requested_count; i++) {
		bool found = false;

		for (uint8_t j = 0; j < received_count; j++) {
			if (requested_tlvs[i] == received_tlvs[j]) {
				found = true;
				break;
			}
		}

		if (!found) {
			if (!*first) {
				int written = snprintf(*ptr, *remaining, ",");
				*ptr += written;
				*remaining -= written;
			}
			*first = false;

			int written = 0;

			switch (requested_tlvs[i]) {
			/* Basic Information */
			case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:
				written = snprintf(*ptr, *remaining,
						   "\"ext_address\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS:
				written = snprintf(*ptr, *remaining,
						   "\"rloc16\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_MODE:
				written = snprintf(*ptr, *remaining,
						   "\"mode\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY:
				written = snprintf(*ptr, *remaining,
						   "\"connectivity\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_ROUTE:
				written = snprintf(*ptr, *remaining,
						   "\"route\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA:
				written = snprintf(*ptr, *remaining,
						   "\"leader_data\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL:
				written = snprintf(*ptr, *remaining,
						   "\"battery_level\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE:
				written = snprintf(*ptr, *remaining,
						   "\"child_table\":\"N/A\"");
				break;

			/* Vendor Information */
			case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME:
				written = snprintf(*ptr, *remaining,
						   "\"vendor_name\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL:
				written = snprintf(*ptr, *remaining,
						   "\"vendor_model\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION:
				written = snprintf(*ptr, *remaining,
						   "\"vendor_sw_version\""
						   ":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION:
				written = snprintf(*ptr, *remaining,
						   "\"thread_stack_version\""
						   ":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_APP_URL:
				written = snprintf(*ptr, *remaining,
						   "\"vendor_app_url\""
						   ":\"N/A\"");
				break;

			/* Network Data */
			case OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA:
				written = snprintf(*ptr, *remaining,
						   "\"network_data\":\"N/A\"");
				break;

			/* Counters & Statistics */
			case OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS:
				written = snprintf(*ptr, *remaining,
						   "\"mac_counters\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS:
				written = snprintf(*ptr, *remaining,
						   "\"mle_counters\":\"N/A\"");
				break;
			case OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST:
				written = snprintf(*ptr, *remaining,
						   "\"ipv6_addresses\""
						   ":\"N/A\"");
				break;

			default:
				written = snprintf(*ptr, *remaining,
						   "\"tlv_%d\":\"N/A\"",
						   requested_tlvs[i]);
				break;
			}

			*ptr += written;
			*remaining -= written;
		}
	}
}

/* Helper to format string TLV, skipping first byte if it's a length */
static const char *get_tlv_string(const char *str)
{
	if (str[0] < 32 || str[0] > 126) {
		return &str[1];  /* Skip length byte */
	}
	return str;
}

/* Helper to format vendor name TLV */
static int format_vendor_name(char *ptr, size_t remaining,
			       const otNetworkDiagTlv *diag_tlv)
{
	const char *str = get_tlv_string((const char *)diag_tlv->mData.mVendorName);

	return snprintf(ptr, remaining, "\"vendor_name\":\"%s\"", str);
}

/* Helper to format vendor model TLV */
static int format_vendor_model(char *ptr, size_t remaining,
				const otNetworkDiagTlv *diag_tlv)
{
	const char *str = get_tlv_string((const char *)diag_tlv->mData.mVendorModel);

	return snprintf(ptr, remaining, "\"vendor_model\":\"%s\"", str);
}

/* Helper to format vendor SW version TLV */
static int format_vendor_sw_version(char *ptr, size_t remaining,
				     const otNetworkDiagTlv *diag_tlv)
{
	const char *str = get_tlv_string((const char *)diag_tlv->mData.mVendorSwVersion);

	return snprintf(ptr, remaining, "\"vendor_sw_version\":\"%s\"", str);
}

/* Helper to format thread stack version TLV */
static int format_thread_stack_version(char *ptr, size_t remaining,
					const otNetworkDiagTlv *diag_tlv)
{
	const char *str = get_tlv_string((const char *)diag_tlv->mData.mThreadStackVersion);

	return snprintf(ptr, remaining, "\"thread_stack_version\":\"%s\"", str);
}

/* Helper to format vendor app URL TLV */
static int format_vendor_app_url(char *ptr, size_t remaining,
				  const otNetworkDiagTlv *diag_tlv)
{
	const char *str = get_tlv_string((const char *)diag_tlv->mData.mVendorAppUrl);

	return snprintf(ptr, remaining, "\"vendor_app_url\":\"%s\"", str);
}

/* Helper to format ext address TLV */
static int format_ext_address(char *ptr, size_t remaining,
			       const otNetworkDiagTlv *diag_tlv)
{
	return snprintf(ptr, remaining,
			"\"ext_address\":"
			"\"%02x%02x%02x%02x%02x%02x%02x%02x\"",
			diag_tlv->mData.mExtAddress.m8[0],
			diag_tlv->mData.mExtAddress.m8[1],
			diag_tlv->mData.mExtAddress.m8[2],
			diag_tlv->mData.mExtAddress.m8[3],
			diag_tlv->mData.mExtAddress.m8[4],
			diag_tlv->mData.mExtAddress.m8[5],
			diag_tlv->mData.mExtAddress.m8[6],
			diag_tlv->mData.mExtAddress.m8[7]);
}

/* Helper to format short address TLV */
static int format_short_address(char *ptr, size_t remaining,
				 const otNetworkDiagTlv *diag_tlv)
{
	return snprintf(ptr, remaining,
			"\"rloc16\":\"0x%04x\"",
			diag_tlv->mData.mAddr16);
}

/* Helper to format battery level TLV */
static int format_battery_level(char *ptr, size_t remaining,
				 const otNetworkDiagTlv *diag_tlv)
{
	return snprintf(ptr, remaining,
			"\"battery_level\":%d",
			diag_tlv->mData.mBatteryLevel);
}

/* Helper to format network data TLV */
static int format_network_data(char *ptr, size_t remaining,
				const uint8_t *buffer, uint16_t data_length)
{
	int written = snprintf(ptr, remaining, "\"network_data\":\"");

	ptr += written;
	remaining -= written;

	/* Search for TLV type 7 in the raw buffer */
	for (uint16_t i = 0; i < data_length - 2; i++) {
		if (buffer[i] == 7) {  /* Type = 7 (Network Data) */
			uint8_t len = buffer[i + 1];

			for (uint8_t j = 0; j < len && j < 128; j++) {
				int w = snprintf(ptr, remaining, "%02x",
						 buffer[i + 2 + j]);
				ptr += w;
				remaining -= w;
				written += w;
			}
			break;
		}
	}

	written += snprintf(ptr, remaining, "\"");
	return written;
}

/* Helper to format unsupported TLV */
static int format_unsupported_tlv(char *ptr, size_t remaining, uint8_t type)
{
	return snprintf(ptr, remaining,
			"\"tlv_%d\":\"unsupported\"", type);
}

/* Helper to format a single diagnostic TLV */
static int format_diagnostic_tlv(char *ptr, size_t remaining,
				  const otNetworkDiagTlv *diag_tlv,
				  const uint8_t *buffer, uint16_t data_length)
{
	switch (diag_tlv->mType) {
	case OT_NETWORK_DIAGNOSTIC_TLV_EXT_ADDRESS:
		return format_ext_address(ptr, remaining, diag_tlv);

	case OT_NETWORK_DIAGNOSTIC_TLV_SHORT_ADDRESS:
		return format_short_address(ptr, remaining, diag_tlv);

	case OT_NETWORK_DIAGNOSTIC_TLV_MODE:
		return format_diag_mode(ptr, remaining, &diag_tlv->mData.mMode);

	case OT_NETWORK_DIAGNOSTIC_TLV_CONNECTIVITY:
		return format_diag_connectivity(ptr, remaining,
						&diag_tlv->mData.mConnectivity);

	case OT_NETWORK_DIAGNOSTIC_TLV_NETWORK_DATA:
		return format_network_data(ptr, remaining, buffer, data_length);

	case OT_NETWORK_DIAGNOSTIC_TLV_ROUTE:
		return format_diag_route(ptr, remaining, &diag_tlv->mData.mRoute);

	case OT_NETWORK_DIAGNOSTIC_TLV_LEADER_DATA:
		return format_diag_leader_data(ptr, remaining,
					       &diag_tlv->mData.mLeaderData);

	case OT_NETWORK_DIAGNOSTIC_TLV_BATTERY_LEVEL:
		return format_battery_level(ptr, remaining, diag_tlv);

	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_NAME:
		return format_vendor_name(ptr, remaining, diag_tlv);

	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_MODEL:
		return format_vendor_model(ptr, remaining, diag_tlv);

	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_SW_VERSION:
		return format_vendor_sw_version(ptr, remaining, diag_tlv);

	case OT_NETWORK_DIAGNOSTIC_TLV_THREAD_STACK_VERSION:
		return format_thread_stack_version(ptr, remaining, diag_tlv);

	case OT_NETWORK_DIAGNOSTIC_TLV_VENDOR_APP_URL:
		return format_vendor_app_url(ptr, remaining, diag_tlv);

	case OT_NETWORK_DIAGNOSTIC_TLV_CHILD_TABLE:
		return format_diag_child_table(ptr, remaining,
					       diag_tlv->mData.mChildTable.mTable,
					       diag_tlv->mData.mChildTable.mCount);

	case OT_NETWORK_DIAGNOSTIC_TLV_MAC_COUNTERS:
		return format_diag_mac_counters(ptr, remaining,
						&diag_tlv->mData.mMacCounters);

	case OT_NETWORK_DIAGNOSTIC_TLV_MLE_COUNTERS:
		return format_diag_mle_counters(ptr, remaining,
						&diag_tlv->mData.mMleCounters);

	case OT_NETWORK_DIAGNOSTIC_TLV_IP6_ADDR_LIST:
		return format_diag_ip6_addr_list(ptr, remaining,
						 diag_tlv->mData.mIp6AddrList.mList,
						 diag_tlv->mData.mIp6AddrList.mCount);

	default:
		return format_unsupported_tlv(ptr, remaining, diag_tlv->mType);
	}
}

/* Helper to track received TLVs */
static void track_received_tlv(uint8_t *received_tlvs, uint8_t *received_count,
			       uint8_t tlv_type)
{
	if (*received_count < 32) {
		received_tlvs[(*received_count)++] = tlv_type;
	}
}

/* Helper to add separator if needed */
static void add_separator_if_needed(char **ptr, size_t *remaining, bool *first)
{
	int written = 0;

	if (!*first) {
		written = snprintf(*ptr, *remaining, ",");
		*ptr += written;
		*remaining -= written;
	}
	*first = false;
}

/* Helper function to parse and format diagnostic TLVs */
static bool parse_diagnostic_tlvs(otMessage *message,
				   const uint8_t *buffer,
				   uint16_t data_length,
				   char *response,
				   size_t response_size,
				   const uint8_t *requested_tlvs,
				   uint8_t requested_count)
{
	char *ptr = response;
	size_t remaining = response_size;
	int written = 0;
	otNetworkDiagIterator iterator = OT_NETWORK_DIAGNOSTIC_ITERATOR_INIT;
	otNetworkDiagTlv diag_tlv;
	bool first = true;
	uint8_t received_tlvs[32] = {0};
	uint8_t received_count = 0;

	written = snprintf(ptr, remaining, "{\"status\":\"ok\",\"data\":{");
	ptr += written;
	remaining -= written;

	while (otThreadGetNextDiagnosticTlv(message, &iterator, &diag_tlv) ==
	       OT_ERROR_NONE) {
		add_separator_if_needed(&ptr, &remaining, &first);
		track_received_tlv(received_tlvs, &received_count, diag_tlv.mType);

		written = format_diagnostic_tlv(ptr, remaining, &diag_tlv,
						buffer, data_length);
		ptr += written;
		remaining -= written;
	}

	if (requested_tlvs && requested_count > 0) {
		add_missing_tlvs(&ptr, &remaining, &first,
				 requested_tlvs, requested_count,
				 received_tlvs, received_count);
	}

	snprintf(ptr, remaining, "}}");
	return true;
}

static void diagnostic_callback(otError error,
				otMessage *message,
				const otMessageInfo *message_info,
				void *context)
{
	uint16_t length;
	uint16_t offset;
	uint16_t data_length;
	uint8_t buffer[4096];
	struct diag_context *ctx;

	k_mutex_lock(&diag_mutex, K_FOREVER);

	if (error == OT_ERROR_NONE && message != NULL) {
		length = otMessageGetLength(message);
		offset = otMessageGetOffset(message);
		data_length = length - offset;

		if (data_length > 0 &&
			data_length < sizeof(current_diag_result.response)) {

			otMessageRead(message, offset, buffer, data_length);

			/* Get requested TLVs from context */
			ctx = (struct diag_context *)context;

			/* Parse the diagnostic TLVs */
			parse_diagnostic_tlvs(message, buffer, data_length,
						  current_diag_result.response,
						  sizeof(current_diag_result.response),
						  ctx ? ctx->tlvs : NULL,
						  ctx ? ctx->count : 0);
			current_diag_result.received = true;
		}
	} else {
		snprintf(current_diag_result.response,
			 sizeof(current_diag_result.response),
			"{\"error\":\"Diagnostic request failed\","
			"\"error_code\":%d}", error);
		current_diag_result.received = true;
	}

	k_mutex_unlock(&diag_mutex);
}

/* Helper function to URL-decode a string */
static void url_decode(char *dst, const char *src, size_t dst_size)
{
	size_t i = 0, j = 0;

	while (src[i] && j < dst_size - 1) {
		if (src[i] == '%' && src[i+1] && src[i+2]) {
			/* Decode %XX */
			char hex[3] = {src[i+1], src[i+2], '\0'};

			dst[j++] = (char)strtol(hex, NULL, 16);
			i += 3;
		} else if (src[i] == '+') {
			/* '+' is space in URL encoding */
			dst[j++] = ' ';
			i++;
		} else {
			dst[j++] = src[i++];
		}
	}
	dst[j] = '\0';
}

/* Structure to hold diagnostic request parameters */
struct diag_request_params {
	uint16_t rloc16;
	uint8_t tlv_types[32];
	uint8_t tlv_count;
};

/* Helper to receive diagnostic POST data chunk */
static void receive_diag_chunk(const struct http_request_ctx *request_ctx,
				char *buffer, size_t *len, size_t buffer_size)
{
	size_t remaining = buffer_size - *len - 1;
	size_t to_copy = request_ctx->data_len < remaining ?
			 request_ctx->data_len : remaining;

	memcpy(buffer + *len, request_ctx->data, to_copy);
	*len += to_copy;
	buffer[*len] = '\0';
}

/* Helper to parse RLOC16 from diagnostic request */
static bool parse_diag_rloc16(const char *data, uint16_t *rloc16)
{
	const char *rloc_param = strstr(data, "rloc16=");

	if (!rloc_param) {
		return false;
	}

	*rloc16 = (uint16_t)strtol(rloc_param + 7, NULL, 0);
	return true;
}

/* Helper to decode and extract TLV value string */
static bool extract_tlv_value(const char *tlvs_param, char *decoded_tlvs,
			      size_t decoded_size)
{
	const char *tlv_value_start = tlvs_param + 5;  /* Skip "tlvs=" */
	const char *tlv_value_end = strchr(tlv_value_start, '&');
	size_t tlv_value_len;
	char encoded_tlvs[128];

	if (tlv_value_end) {
		tlv_value_len = tlv_value_end - tlv_value_start;
	} else {
		tlv_value_len = strlen(tlv_value_start);
	}

	if (tlv_value_len >= decoded_size) {
		tlv_value_len = decoded_size - 1;
	}

	memcpy(encoded_tlvs, tlv_value_start, tlv_value_len);
	encoded_tlvs[tlv_value_len] = '\0';

	url_decode(decoded_tlvs, encoded_tlvs, decoded_size);
	LOG_DBG("Decoded TLVs string: '%s'", decoded_tlvs);

	return true;
}

/* Helper to parse comma-separated TLV types */
static uint8_t parse_tlv_types(const char *tlv_string, uint8_t *tlv_types,
				uint8_t max_count)
{
	const char *tlv_start = tlv_string;
	char *tlv_end;
	uint8_t count = 0;

	while (count < max_count && *tlv_start) {
		tlv_types[count] = (uint8_t)strtol(tlv_start, &tlv_end, 10);
		LOG_DBG("Parsed TLV[%d] = %d", count, tlv_types[count]);
		count++;

		if (*tlv_end == ',') {
			tlv_start = tlv_end + 1;
		} else {
			break;
		}
	}

	return count;
}

/* Helper to parse TLVs parameter */
static uint8_t parse_tlvs_param(const char *data, uint8_t *tlv_types,
				uint8_t max_count)
{
	const char *tlvs_param = strstr(data, "tlvs=");
	char decoded_tlvs[128];

	if (!tlvs_param) {
		return 0;
	}

	if (!extract_tlv_value(tlvs_param, decoded_tlvs, sizeof(decoded_tlvs))) {
		return 0;
	}

	return parse_tlv_types(decoded_tlvs, tlv_types, max_count);
}

/* Helper to parse diagnostic request parameters */
static bool parse_diag_params(const char *data, struct diag_request_params *params,
			       char *error_msg, size_t error_size)
{
	if (!parse_diag_rloc16(data, &params->rloc16)) {
		snprintf(error_msg, error_size,
			 "{\"error\":\"Missing rloc16 parameter\"}");
		return false;
	}

	params->tlv_count = parse_tlvs_param(data, params->tlv_types, 32);

	LOG_DBG("Requesting diagnostics for RLOC16: 0x%04x with %d TLVs",
		params->rloc16, params->tlv_count);

	return true;
}

/* Helper to construct diagnostic destination address */
static bool construct_diag_dest_addr(otInstance *instance, uint16_t rloc16,
				      otIp6Address *dest_addr,
				      char *error_msg, size_t error_size)
{
	const otMeshLocalPrefix *ml_prefix = otThreadGetMeshLocalPrefix(instance);

	openthread_mutex_lock();
	ml_prefix = otThreadGetMeshLocalPrefix(instance);
	openthread_mutex_unlock();

	if (ml_prefix == NULL) {
		snprintf(error_msg, error_size,
			 "{\"error\":"
			 "\"Failed to get mesh-local prefix\"}");
		return false;
	}

	memset(dest_addr, 0, sizeof(*dest_addr));
	memcpy(dest_addr->mFields.m8, ml_prefix->m8, 8);
	dest_addr->mFields.m8[8] = 0x00;
	dest_addr->mFields.m8[9] = 0x00;
	dest_addr->mFields.m8[10] = 0x00;
	dest_addr->mFields.m8[11] = 0xff;
	dest_addr->mFields.m8[12] = 0xfe;
	dest_addr->mFields.m8[13] = 0x00;
	dest_addr->mFields.m8[14] = (uint8_t)(rloc16 >> 8);
	dest_addr->mFields.m8[15] = (uint8_t)(rloc16 & 0xFF);

	return true;
}

/* Helper to send diagnostic request */
static bool send_diag_request(otInstance *instance,
			       const struct diag_request_params *params,
			       const otIp6Address *dest_addr,
			       char *error_msg, size_t error_size)
{
	otError error;

	/* Reset diagnostic result */
	k_mutex_lock(&diag_mutex, K_FOREVER);
	memset(&current_diag_result, 0, sizeof(current_diag_result));
	k_mutex_unlock(&diag_mutex);

	/* Request network diagnostics with callback */
	openthread_mutex_lock();
	error = otThreadSendDiagnosticGet(instance,
						  dest_addr,
						  params->tlv_types,
						  params->tlv_count,
						  diagnostic_callback,
						  NULL);
	openthread_mutex_unlock();

	if (error != OT_ERROR_NONE) {
		snprintf(error_msg, error_size,
			 "{\"error\":"
			 "\"Failed to send diagnostic request\","
			 "\"error_code\":%d}",
			 error);
		return false;
	}

	return true;
}

/* Helper to wait for diagnostic response */
static bool wait_for_diag_response(char *response, size_t response_size)
{
	const int max_wait = 20; /* 2 seconds (20 * 100ms) */
	int wait_count = 0;
	bool received = false;

	while (wait_count < max_wait) {
		k_sleep(K_MSEC(100));
		k_mutex_lock(&diag_mutex, K_FOREVER);
		received = current_diag_result.received;
		k_mutex_unlock(&diag_mutex);

		if (received) {
			LOG_DBG("Diagnostic response received after %d ms",
				wait_count * 100);
			break;
		}
		wait_count++;
	}

	k_mutex_lock(&diag_mutex, K_FOREVER);
	if (current_diag_result.received) {
		strncpy(response, current_diag_result.response, response_size - 1);
		response[response_size - 1] = '\0';
		k_mutex_unlock(&diag_mutex);
		return true;
	}

	k_mutex_unlock(&diag_mutex);
	LOG_WRN("Diagnostic request timeout after %d ms", wait_count * 100);
	snprintf(response, response_size,
		 "{\"error\":"
		 "\"Diagnostic request timeout\"}");
	return false;
}

/* Helper to process diagnostic request */
static void process_diag_request(otInstance *instance, const char *post_data,
				  char *response, size_t response_size)
{
	struct diag_request_params params;
	otIp6Address dest_addr;

	if (!parse_diag_params(post_data, &params, response, response_size)) {
		return;
	}

	if (!construct_diag_dest_addr(instance, params.rloc16, &dest_addr,
				       response, response_size)) {
		return;
	}

	if (!send_diag_request(instance, &params, &dest_addr,
			       response, response_size)) {
		return;
	}

	wait_for_diag_response(response, response_size);
}

/* Helper to handle final diagnostic data */
static void handle_final_diag_data(const struct http_request_ctx *request_ctx,
				    char *buffer, size_t *len,
				    char *response, size_t response_size)
{
	otInstance *instance;

	if (request_ctx->data_len > 0) {
		receive_diag_chunk(request_ctx, buffer, len, 256);
	}

	instance = get_ot_instance();

	if (!instance) {
		snprintf(response, response_size,
			 "{\"error\":"
			 "\"OpenThread instance not available\"}");
		return;
	}

	LOG_DBG("Received POST data: %s", buffer);
	process_diag_request(instance, buffer, response, response_size);
}

/* REST API: POST /api/node/diagnostics: rloc16=0xXXXX&tlvs=0,1,2,3 */
int rest_api_node_diagnostics_handler(struct http_client_ctx *client,
					enum http_transaction_status status,
					const struct http_request_ctx *request_ctx,
					struct http_response_ctx *response_ctx,
					void *user_data)
{
	static char json_response[4096];
	static char post_data_buffer_diag[256];
	static size_t post_data_len_diag;

	LOG_INF("Node diagnostics API called, status: %d", status);

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		post_data_len_diag = 0;
		memset(post_data_buffer_diag, 0,
		       sizeof(post_data_buffer_diag));
		return 0;
	}

	if (status == HTTP_SERVER_REQUEST_DATA_MORE) {
		receive_diag_chunk(request_ctx, post_data_buffer_diag,
				   &post_data_len_diag,
				   sizeof(post_data_buffer_diag));
		return 0;
	}

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		handle_final_diag_data(request_ctx, post_data_buffer_diag,
					&post_data_len_diag, json_response,
					sizeof(json_response));

		post_data_len_diag = 0;
		memset(post_data_buffer_diag, 0,
				sizeof(post_data_buffer_diag));
		response_ctx->body = (const uint8_t *)json_response;
		response_ctx->body_len = strlen(json_response);
		response_ctx->final_chunk = true;
		response_ctx->status = HTTP_200_OK;
	}

	return 0;
}
