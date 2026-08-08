/*
 * Redfish ComputerSystem resources describing the host managed by the BMC.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/config.h>
#include <zephyr/mgmt/bmc/host.h>
#include <zephyr/mgmt/bmc/redfish.h>

#include "redfish_internal.h"

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

/*** /redfish/v1/Systems ***/
static int systems_collection_get(struct bmc_redfish_ctx *ctx)
{
	int ret;

	ret = redfish_collection_open(ctx, REDFISH_URI_SYSTEMS,
				      "#ComputerSystemCollection.ComputerSystemCollection",
				      "Computer System Collection", 1);
	if (ret == 0) {
		ret = redfish_collection_add(ctx, true, REDFISH_URI_SYSTEM);
	}

	if (ret == 0) {
		ret = redfish_collection_close(ctx);
	}

	return (ret < 0) ? HTTP_500_INTERNAL_SERVER_ERROR : 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_systems, REDFISH_URI_SYSTEMS, true, systems_collection_get,
			    NULL, NULL);

/*** /redfish/v1/Systems/system ***/
#define REDFISH_RESET_TYPES_MAX 3

struct redfish_reset_action {
	const char *target;
	const char *reset_type_values[REDFISH_RESET_TYPES_MAX];
	size_t reset_type_values_len;
};

static const struct json_obj_descr reset_action_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct redfish_reset_action, target, JSON_TOK_STRING),
	JSON_OBJ_DESCR_ARRAY_NAMED(struct redfish_reset_action,
				   "ResetType@Redfish.AllowableValues", reset_type_values,
				   REDFISH_RESET_TYPES_MAX, reset_type_values_len,
				   JSON_TOK_STRING),
};

struct redfish_actions {
	struct redfish_reset_action reset_action;
};

static const struct json_obj_descr actions_descr[] = {
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_actions, "#ComputerSystem.Reset", reset_action,
				    reset_action_descr),
};

struct redfish_processor_summary {
	int32_t count;
	const char *model;
};

static const struct json_obj_descr processor_summary_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_processor_summary, "Count", count,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_processor_summary, "Model", model,
				  JSON_TOK_STRING),
};

struct redfish_memory_summary {
	int32_t total_system_gib;
};

static const struct json_obj_descr memory_summary_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_memory_summary, "TotalSystemMemoryGiB",
				  total_system_gib, JSON_TOK_NUMBER),
};

struct redfish_system_links {
	size_t chassis_len;
	struct redfish_link chassis[1];
	size_t managed_by_len;
	struct redfish_link managed_by[1];
};

static const struct json_obj_descr system_links_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct redfish_system_links, "Chassis", chassis, 1,
				       chassis_len, redfish_link_descr,
				       ARRAY_SIZE(redfish_link_descr)),
	JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct redfish_system_links, "ManagedBy", managed_by, 1,
				       managed_by_len, redfish_link_descr,
				       ARRAY_SIZE(redfish_link_descr)),
};

struct redfish_computer_system {
	const char *odata_id;
	const char *odata_type;
	const char *id;
	const char *uuid;
	const char *name;
	const char *system_type;
	const char *manufacturer;
	const char *model;
	const char *serial_number;
	const char *part_number;
	const char *power_restore_policy;
	const char *power_state;
	struct redfish_processor_summary processor_summary;
	struct redfish_memory_summary memory_summary;
	struct redfish_system_links links;
	struct redfish_actions actions;
};

static const struct json_obj_descr computer_system_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "@odata.id", odata_id,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "@odata.type", odata_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "Id", id, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "UUID", uuid, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "Name", name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "SystemType", system_type,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "Manufacturer", manufacturer,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "Model", model,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "SerialNumber", serial_number,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "PartNumber", part_number,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "PowerRestorePolicy",
				  power_restore_policy, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_computer_system, "PowerState", power_state,
				  JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_computer_system, "ProcessorSummary",
				    processor_summary, processor_summary_descr),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_computer_system, "MemorySummary",
				    memory_summary, memory_summary_descr),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_computer_system, "Links", links,
				    system_links_descr),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct redfish_computer_system, "Actions", actions,
				    actions_descr),
};

static int system_patch(struct bmc_redfish_ctx *ctx)
{
	struct redfish_computer_system payload;
	int ret;

	memset(&payload, 0, sizeof(payload));

	ret = bmc_redfish_request_parse(ctx, computer_system_descr,
					ARRAY_SIZE(computer_system_descr), &payload);
	if (ret < 0) {
		LOG_ERR("ComputerSystem: malformed JSON (err=%d)", ret);
		return HTTP_400_BAD_REQUEST;
	}

	if (payload.power_restore_policy != NULL) {
		if (strcmp(payload.power_restore_policy, "AlwaysOn") == 0) {
			ret = bmc_config_host_auto_poweron_set(true);
		} else if (strcmp(payload.power_restore_policy, "AlwaysOff") == 0) {
			ret = bmc_config_host_auto_poweron_set(false);
		} else {
			LOG_ERR("ComputerSystem: unsupported PowerRestorePolicy \"%s\"",
				payload.power_restore_policy);
			return HTTP_400_BAD_REQUEST;
		}

		if (ret < 0) {
			LOG_ERR("Could not set the host auto power-on policy (err=%d)", ret);
			return HTTP_500_INTERNAL_SERVER_ERROR;
		}
	}

	return 0;
}

static int system_get(struct bmc_redfish_ctx *ctx)
{
	const struct bmc_redfish_identity *identity = bmc_redfish_identity_get();
	const struct redfish_computer_system computer_system = {
		.odata_id = REDFISH_URI_SYSTEM,
		.odata_type = "#ComputerSystem.v1_22_0.ComputerSystem",
		.id = "system",
		.uuid = bmc_uuid_get(),
		.name = identity->product_name,
		.system_type = "Physical",
		.manufacturer = identity->manufacturer,
		.model = identity->model,
		.serial_number = identity->serial_number,
		.part_number = identity->part_number,
		.power_restore_policy = bmc_config_host_auto_poweron() ? "AlwaysOn" : "AlwaysOff",
		.power_state = bmc_host_power_get() ? "On" : "Off",
		.processor_summary = {
			.count = identity->processor_count,
			.model = identity->processor_model,
		},
		.memory_summary = {
			.total_system_gib = identity->memory_gib,
		},
		.links = {
			.chassis_len = 1,
			.chassis = {{.odata_id = REDFISH_URI_CHASSIS}},
			.managed_by_len = 1,
			.managed_by = {{.odata_id = REDFISH_URI_MANAGER}},
		},
		.actions = {
			.reset_action = {
				.target = REDFISH_URI_SYSTEM_RESET,
				.reset_type_values = {"On", "ForceOff", "PowerCycle"},
				.reset_type_values_len = REDFISH_RESET_TYPES_MAX,
			},
		},
	};

	if (redfish_encode_with_oem(ctx, computer_system_descr, ARRAY_SIZE(computer_system_descr),
				    &computer_system, BMC_REDFISH_OEM_SYSTEM) < 0) {
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_system, REDFISH_URI_SYSTEM, true, system_get, system_patch,
			    NULL);

/*** /redfish/v1/Systems/system/Actions/ComputerSystem.Reset ***/
struct redfish_reset_payload {
	const char *reset_type;
};

static const struct json_obj_descr reset_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct redfish_reset_payload, "ResetType", reset_type,
				  JSON_TOK_STRING),
};

static int system_reset_post(struct bmc_redfish_ctx *ctx)
{
	struct redfish_reset_payload payload;
	int ret;

	memset(&payload, 0, sizeof(payload));

	ret = bmc_redfish_request_parse(ctx, reset_descr, ARRAY_SIZE(reset_descr), &payload);
	if (ret < 0 || payload.reset_type == NULL) {
		LOG_ERR("ComputerSystem.Reset: malformed JSON (err=%d)", ret);
		return HTTP_400_BAD_REQUEST;
	}

	LOG_INF("ComputerSystem.Reset: %s", payload.reset_type);

	if (strcmp(payload.reset_type, "On") == 0) {
		ret = bmc_host_power_set(true);
	} else if (strcmp(payload.reset_type, "ForceOff") == 0) {
		ret = bmc_host_power_set(false);
	} else if (strcmp(payload.reset_type, "PowerCycle") == 0) {
		ret = bmc_host_reset();
	} else {
		LOG_ERR("ComputerSystem.Reset: unsupported ResetType \"%s\"", payload.reset_type);
		return HTTP_400_BAD_REQUEST;
	}

	if (ret == -ENOTSUP) {
		return HTTP_501_NOT_IMPLEMENTED;
	}

	if (ret < 0) {
		LOG_ERR("ComputerSystem.Reset failed (err=%d)", ret);
		return HTTP_500_INTERNAL_SERVER_ERROR;
	}

	return 0;
}

BMC_REDFISH_RESOURCE_DEFINE(redfish_system_reset, REDFISH_URI_SYSTEM_RESET, true, NULL, NULL,
			    system_reset_post);
