/*
 * Copyright (c) 2018 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *
 *  @brief This file contains structures representing JSON messages
 *  exchanged with a UpdateHub
 */

#ifndef __UPDATEHUB_PRIV_H__
#define __UPDATEHUB_PRIV_H__

#define UPDATEHUB_API_HEADER \
	"Api-Content-Type: application/vnd.updatehub-v1+json"

enum updatehub_uri_path {
	UPDATEHUB_PROBE = 0,
	UPDATEHUB_REPORT,
	UPDATEHUB_DOWNLOAD,
};

enum updatehub_state {
	UPDATEHUB_STATE_DOWNLOADING = 0,
	UPDATEHUB_STATE_DOWNLOADED,
	UPDATEHUB_STATE_INSTALLING,
	UPDATEHUB_STATE_INSTALLED,
	UPDATEHUB_STATE_REBOOTING,
	UPDATEHUB_STATE_ERROR,
};

static char *updatehub_response(enum updatehub_response response)
{
	switch (response) {
	case UPDATEHUB_NETWORKING_ERROR:
		return "Fail to connect to the UpdateHub server";
	case UPDATEHUB_INCOMPATIBLE_HARDWARE:
		return "Incompatible hardware";
	case UPDATEHUB_METADATA_ERROR:
		return "Fail to parse or to encode the metadata";
	case UPDATEHUB_DOWNLOAD_ERROR:
		return "Fail while downloading the update package";
	case UPDATEHUB_INSTALL_ERROR:
		return "Fail while installing the update package";
	case UPDATEHUB_FLASH_INIT_ERROR:
		return "Fail to initialize the flash";
	case UPDATEHUB_NO_UPDATE:
		return "No update available";
	default:
		return NULL;
	}
}

static const char *uri_path(enum updatehub_uri_path type)
{
	switch (type) {
	case UPDATEHUB_PROBE:
		return "upgrades";
	case UPDATEHUB_REPORT:
		return "report";
	case UPDATEHUB_DOWNLOAD:
		return "products";
	default:
		return NULL;
	}
}

static const char *state_name(enum updatehub_state state)
{
	switch (state) {
	case UPDATEHUB_STATE_DOWNLOADING:
		return "downloading";
	case UPDATEHUB_STATE_DOWNLOADED:
		return "downloaded";
	case UPDATEHUB_STATE_INSTALLING:
		return "installing";
	case UPDATEHUB_STATE_INSTALLED:
		return "installed";
	case UPDATEHUB_STATE_REBOOTING:
		return "rebooting";
	case UPDATEHUB_STATE_ERROR:
		return "error";
	default:
		return NULL;
	}
}

struct resp_probe_objects {
	const char *mode;
	const char *sha256sum;
	int size;
};

struct resp_probe_objects_array {
	struct resp_probe_objects objects;
};

struct resp_probe_any_boards {
	struct resp_probe_objects_array objects[2];
	size_t objects_len;
	const char *product;
	const char *supported_hardware;
};

struct resp_probe_some_boards {
	struct resp_probe_objects_array objects[2];
	size_t objects_len;
	const char *product;
	const char *supported_hardware[CONFIG_UPDATEHUB_SUPPORTED_HARDWARE_MAX];
	size_t supported_hardware_len;
};

struct updatehub_config_device_identity {
	const char *id;
};

struct report {
	const char *product_uid;
	const char *hardware;
	const char *version;
	struct updatehub_config_device_identity device_identity;
	const char *status;
	const char *package_uid;
	const char *error_message;
	const char *previous_state;
};

struct probe {
	const char *product_uid;
	const char *hardware;
	const char *version;
	struct updatehub_config_device_identity device_identity;
};

static const struct json_obj_descr recv_probe_objects_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct resp_probe_objects,
			    mode, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct resp_probe_objects,
			    sha256sum, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct resp_probe_objects,
			    size, JSON_TOK_NUMBER),
};

static const struct json_obj_descr recv_probe_objects_descr_array[] = {
	JSON_OBJ_DESCR_OBJECT(struct resp_probe_objects_array,
			      objects, recv_probe_objects_descr),
};

static const struct json_obj_descr recv_probe_sh_string_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct resp_probe_any_boards,
			    product, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct resp_probe_any_boards,
				  "supported-hardware", supported_hardware,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_ARRAY_ARRAY(struct resp_probe_any_boards,
				   objects, 2, objects_len,
				   recv_probe_objects_descr_array,
				   ARRAY_SIZE(recv_probe_objects_descr_array)),
};

static const struct json_obj_descr recv_probe_sh_array_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct resp_probe_some_boards,
			    product, JSON_TOK_STRING),
	JSON_OBJ_DESCR_ARRAY_NAMED(struct resp_probe_some_boards,
				   "supported-hardware", supported_hardware,
				   CONFIG_UPDATEHUB_SUPPORTED_HARDWARE_MAX,
				   supported_hardware_len, JSON_TOK_STRING),
	JSON_OBJ_DESCR_ARRAY_ARRAY(struct resp_probe_some_boards,
				   objects, 2, objects_len,
				   recv_probe_objects_descr_array,
				   ARRAY_SIZE(recv_probe_objects_descr_array)),
};

static const struct json_obj_descr device_identity_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct updatehub_config_device_identity,
			    id, JSON_TOK_STRING),
};

static const struct json_obj_descr send_report_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct report,
				  "product-uid", product_uid,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct report,
				    "device-identity", device_identity,
				    device_identity_descr),
	JSON_OBJ_DESCR_PRIM_NAMED(struct report,
				  "error-message", error_message,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct report,
				  "previous-state", previous_state,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct report,
			    version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct report,
			    hardware, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct report,
				  "package-uid", package_uid,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct report,
			    status, JSON_TOK_STRING),
};

static const struct json_obj_descr send_probe_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct probe,
				  "product-uid", product_uid,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct probe,
				    "device-identity", device_identity,
				    device_identity_descr),
	JSON_OBJ_DESCR_PRIM(struct probe,
			    version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct probe,
			    hardware, JSON_TOK_STRING),
};

/**
 * @}
 */

#endif /* __UPDATEHUB_PRIV_H__ */
