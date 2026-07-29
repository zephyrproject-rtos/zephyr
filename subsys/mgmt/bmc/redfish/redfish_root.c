/*
 * Redfish service root, version document, OData service document and the
 * $metadata schema.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/redfish.h>

#include "redfish_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

/*** /redfish/ ***/
struct redfish_version {
	const char *v1;
};

static const struct json_obj_descr version_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_version, "v1", v1, JSON_TOK_STRING),
};

static int version_get(struct bmc_redfish_ctx *ctx)
{
	const struct redfish_version version = {
		.v1 = REDFISH_URI_ROOT,
	};

	if (bmc_redfish_reply_encode(ctx, version_descr, ARRAY_SIZE(version_descr), &version) <
	    0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_version, "/redfish/", false, version_get, NULL, NULL);

/*** /redfish/v1/ ***/
struct redfish_service_root {
	const char *odata_type;
	const char *odata_id;
	const char *id;
	const char *name;
	const char *redfish_version;
	const char *uuid;
	struct redfish_link account_service;
	struct redfish_link managers;
	struct redfish_link systems;
	struct redfish_link chassis;
};

static const struct json_obj_descr service_root_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_service_root, "@odata.type", odata_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_service_root, "@odata.id", odata_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_service_root, "Id", id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_service_root, "Name", name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_service_root, "RedfishVersion", redfish_version,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_service_root, "UUID", uuid, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_service_root, "AccountService",
				    account_service, redfish_link_descr),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_service_root, "Managers", managers,
				    redfish_link_descr),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_service_root, "Systems", systems,
				    redfish_link_descr),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_service_root, "Chassis", chassis,
				    redfish_link_descr),
};

static int service_root_get(struct bmc_redfish_ctx *ctx)
{
	const struct redfish_service_root service_root = {
		.odata_type = "#ServiceRoot.v1_16_1.ServiceRoot",
		.odata_id = REDFISH_URI_ROOT,
		.id = "RootService",
		.name = "Root Service",
		.redfish_version = "1.15.0",
		.uuid = bmc_uuid_get(),
		.account_service = {.odata_id = REDFISH_URI_ACCOUNT_SERVICE},
		.managers = {.odata_id = REDFISH_URI_MANAGERS},
		.systems = {.odata_id = REDFISH_URI_SYSTEMS},
		.chassis = {.odata_id = REDFISH_URI_CHASSIS_COLL},
	};

	if (bmc_redfish_reply_encode(ctx, service_root_descr, ARRAY_SIZE(service_root_descr),
				     &service_root) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_service_root, REDFISH_URI_ROOT, false, service_root_get, NULL,
			    NULL);

/*** /redfish/v1/odata ***/
struct redfish_odata_value {
	const char *name;
	const char *kind;
	const char *url;
};

static const struct json_obj_descr odata_value_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct redfish_odata_value, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct redfish_odata_value, kind, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct redfish_odata_value, url, JSON_TOK_STRING),
};

#define REDFISH_ODATA_VALUES_MAX 5

struct redfish_odata {
	const char *odata_context;
	size_t value_count;
	struct redfish_odata_value value[REDFISH_ODATA_VALUES_MAX];
};

static const struct json_obj_descr odata_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_odata, "@odata.context", odata_context,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct redfish_odata, "value", value,
				       REDFISH_ODATA_VALUES_MAX, value_count, odata_value_descr,
				       ARRAY_SIZE(odata_value_descr)),
};

static int odata_get(struct bmc_redfish_ctx *ctx)
{
	const struct redfish_odata odata = {
		.odata_context = "/redfish/v1/$metadata",
		.value_count = REDFISH_ODATA_VALUES_MAX,
		.value = {
			{.name = "Service", .kind = "Singleton", .url = REDFISH_URI_ROOT},
			{.name = "Systems", .kind = "Singleton", .url = REDFISH_URI_SYSTEMS},
			{.name = "Managers", .kind = "Singleton", .url = REDFISH_URI_MANAGERS},
			{.name = "AccountService", .kind = "Singleton",
			 .url = REDFISH_URI_ACCOUNT_SERVICE},
			{.name = "Chassis", .kind = "Singleton",
			 .url = REDFISH_URI_CHASSIS_COLL},
		},
	};

	if (bmc_redfish_reply_encode(ctx, odata_descr, ARRAY_SIZE(odata_descr), &odata) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_odata, "/redfish/v1/odata", false, odata_get, NULL, NULL);

/*** /redfish/v1/$metadata ***/
static const uint8_t redfish_metadata_xml_gz[] = {
#include "redfish_metadata.xml.gz.inc"
};

static struct http_resource_detail_static redfish_metadata_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "application/xml",
	},
	.static_data = redfish_metadata_xml_gz,
	.static_data_len = sizeof(redfish_metadata_xml_gz),
};

BMC_HTTP_RESOURCE_DEFINE(redfish_metadata, "/redfish/v1/$metadata", &redfish_metadata_detail);
